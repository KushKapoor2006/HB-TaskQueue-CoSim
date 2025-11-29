# bug_report.md — Bug Analysis

**Project:** HammerBlade_validation (HB-TaskQueue-CoSim)

---

## Executive summary

During hardware–software co-simulation of the HammerBlade task-queue accelerator, an independent Python golden FIFO model reported a **systematic correctness failure**: **3164 pop mismatches** in the host-observed trace (every recorded pop disagreed with the expected FIFO value). The hardware-only testbench (internal Verilator TB) reported **0 mismatches** when exercising the RTL in isolation. The divergence is localized to the **MMIO-driven interaction** (bridge and handshake semantics) between the host driver and the Verilator harness.

This document presents a public, reproducible bug analysis: exact reproduction steps, collected evidence, cycle-level inspection checklist, root-cause findings and rationale, concrete remediation actions (remediation validated in steps), and a validation plan to demonstrate the fix.

---

## Affected components

* `sw/sw_hw/verilator_main.cpp` — MMIO bridge used to expose `mmio_region.bin` to host software.
* `sw/src/task_queue_mmio.c` / host driver — records `sw/logs/trace.csv` and interprets MMIO handshake bits.
* `hw/rtl/*` — hb_task_queue_core.sv, hb_task_distributor.sv, hb_arbiter_banked.sv (the FIFO and distribution logic).

Failure observed when the RTL is exercised via the MMIO bridge. The RTL internal TB (without the MMIO bridge) shows no internal mismatches.

---

## Reproduction (exact, reproducible steps)

Run the following in two terminals (A for HW, B for SW). These commands reproduce the failing trace included in this repo.

**Terminal A — HW harness (Verilator)**

```bash
cd sw/sw_hw
make sim
# Launch the Verilator binary from sw/sw_hw so mmio_region.bin appears in this directory
../obj_dir/Vtb_task_queue
# leave running
```

**Terminal B — SW host**

```bash
cd sw
make                # builds test_task_queue_host
./test_task_queue_host
# After completion, check sw/logs/
```

**Offline oracle**

```bash
python3 model/fifo_model.py sw/logs/trace.csv
# Outputs: sw/logs/golden_results.json (contains mismatches count)
```

Expected result in this repository (pre-fix): `sw/logs/golden_results.json` reports `mismatches = 3164`.

---

## Evidence & artifacts

The repository contains the following evidence used in the analysis and for public inspection:

* `sw/logs/trace.csv` — chronological record of host-observed successful push/pop events.
* `sw/logs/results.json` — aggregated SW counts (attempts, successes, refusals).
* `sw/logs/golden_results.json` — golden-model output (3164 mismatches).
* `hw/run.log` and `hw/results.json` — HW-only TB outputs (0 internal mismatches; sim_cycles = 9640).
* `sw/sw_hw/sim.vcd` — Verilator waveform capture (available; recommended to inspect cycles around failing ops).

These artifacts are included verbatim in the repository to enable external reviewers to reproduce and inspect the failure.

---

## Observed behavior (precise)

* The SW host successfully performed 3164 pushes and 3164 pops according to `sw/logs/results.json`.
* The Python golden model (ideal FIFO) replay of `sw/logs/trace.csv` indicates that **every pop value returned to the host did not match the expected FIFO output** (3164 mismatches recorded in `sw/logs/golden_results.json`).
* The HW-only testbench (internal verification harness running the RTL in a self-contained TB) reported **no mismatches** for the same deterministic and randomized tests, indicating that the core FIFO logic can behave correctly but the MMIO-driven interaction exposes a failure mode.

Consequence: under MMIO-driven operation (realistic host), the accelerator returns incorrect task payloads to the leader — this is a functional correctness bug that invalidates end-to-end results and must be fixed before any synthesis or deployment.

---

## Cycle-level inspection checklist (how to analyze sim.vcd)

Open `sw/sw_hw/sim.vcd` in GTKWave or another waveform viewer and add the following signals to the view (names may vary in generated headers; use `Vtb_task_queue.h` to find exact identifiers):

* `clk`, `reset`
* MMIO interface signals: `host_push_req`, `host_pop_req`, `host_push_ack`, `host_pop_ack`, `host_valid`, `host_full` (or equivalently named signals)
* `DATA_IN`, `DATA_OUT`
* FIFO internal regs: `read_ptr`, `write_ptr`, `count` (if exported)
* Distributor/arbiter select signals (bank index)

For a failing pop event (use `sw/logs/trace.csv` to find an approximate timestamp):

1. Verify that `host_pop_req` is asserted and then sampled by the DUT.
2. Confirm that `DATA_OUT` is stable and `host_valid` (the DUT's valid indicator) is asserted at the time the host reads it.
3. If `DATA_OUT` is not stable or `host_valid` is not asserted when the host reads, the bridge is sampling too early.
4. Verify pointer values (`read_ptr`, `write_ptr`) before and after pop: they should correspond to the FIFO ordering expected by the golden model.
5. Note whether `DATA_OUT` changes in the same cycle the host samples — simultaneous changes indicate a race.

If `DATA_OUT` is stale (equal to the previous read) or not matching the expected pushed value while pointers indicate a different element, the issue is timing/order of sampling between bridge and RTL.

---

## Root-cause analysis (conclusions)

After reviewing the evidence and the pattern of failure, the most probable cause is **MMIO sampling/timing mismatch** between the Verilator MMIO bridge and the RTL core. Supporting reasoning:

* The RTL internal TB reports 0 mismatches: core logic is capable of correct behavior when the TB exercises it directly.
* The host-observed trace shows correct counts (push/pop operations occur) but incorrect payload values, implying reads occur but values are wrong/stale rather than ops missing.
* The likely mechanism is that the bridge or host reads `DATA_OUT` before the DUT has registered the correct value onto `DATA_OUT` and asserted `valid`.

Less probable but possible secondary causes:

* ACK/VALID handshake ordering errors allowing host to sample a data bus while it is transient.
* Pointer update/order inversion inside the core under the host-driven timing mode (e.g., pointer increments visible before data write completes).
* File-backed MMIO partial-write visibility or host caching effects (unlikely given counts match and everything is local file-backed).

Thus the working hypothesis is: **bridge samples too early and reads stale/previous values**.

---

## Remediation (concrete changes applied / to be applied)

This section states the remediation plan in clear terms suitable for a public bug report. Two categories of fixes are appropriate: quick mitigation for validation, and a robust RTL fix.

### 1) Short-term mitigation — bridge-side one-cycle delay

**Action:** Modify the Verilator MMIO bridge (in `verilator_main.cpp`) so that when the host issues a `pop` request, the bridge waits one additional clock edge before sampling `DATA_OUT` and returning the value to the host.

**Rationale:** Adds minimal latency but hugely reduces race conditions while preserving the RTL unchanged. This patch is fast to implement and validate.

**Validation:** Run the existing deterministic and randomized host tests. Expectation: `sw/logs/golden_results.json` should show `mismatches == 0` if the issue is purely sampling timing.

### 2) Robust fix — add `sample_ready` register in RTL

**Action:** Add a `sample_ready` output in `hb_task_queue_core.sv` that the DUT asserts when `DATA_OUT` and `valid` have been stable for a full cycle. The bridge will require `sample_ready==1` before allowing the host to read `DATA_OUT`.

**Rationale:** Explicitly documents and synchronizes the data-validity contract between DUT and bridge. This is protocol-level correctness and is preferred for production-quality code.

**Validation:** After integrating `sample_ready`, run the deterministic + randomized campaigns and expect `golden_results.json` to report zero mismatches. Cycle-level waveforms should show `sample_ready` asserted before `DATA_OUT` sampling.

### 3) Pointer-ordering guard (if needed)

**Action:** Reorder RTL operations so that `DATA_OUT` is written by the FIFO stage before `read_ptr` increments, or add an output register stage to ensure `DATA_OUT` is stable for the cycle the pointer changes.

**Rationale:** Ensures the visible data corresponds to the expected FIFO element even under tight timing windows.

**Validation:** As above.

---

## Validation & regression plan

The fix process follows these steps and must be documented in the PR for transparency:

1. Implement bridge-side one-cycle delay and run the deterministic micro-test (fast). If this resolves mismatches, document the result and proceed to the next step.
2. Implement `sample_ready` in RTL, re-run all tests. The expectation is `golden_results.json` shows zero mismatches for both small and randomized campaigns.
3. Add unit deterministic fixtures (small sequences) under `tests/fixtures/` that are run as part of `make verify`.
4. Run full randomized campaign (10k ops) and record `sw/logs/results.json` and `sw/logs/golden_results.json` in the PR artifacts.
5. For final acceptance, include `sw/sw_hw/sim.vcd` excerpts, annotated GTKWave screenshots showing corrected timing.

Automated `make verify` target will be added to the repository; it runs HW build, SW host, Python golden model, and plots; it should fail on any `mismatches > 0`.

---

## CI / automation recommendations

* `make verify` target in `sw/Makefile` that:

  * Builds the Verilator harness (in `sw/sw_hw`),
  * Starts the Verilator binary in background,
  * Runs `./test_task_queue_host`,
  * Runs `python3 model/fifo_model.py sw/logs/trace.csv`,
  * Runs plotting scripts, and
  * Exits with non-zero status if `golden_results.json` reports `mismatches > 0`.

* CI job (GitHub Actions) to run `make verify` on a Linux runner with Verilator installed. Use a larger timeout for the Verilator run.

---

## Minimal reproducible fixture (included)

Add a small fixture under `sw/tests/fixtures/min_repro/` with:

* `trace_min.csv` — a tiny deterministic `push,pop` sequence that previously failed (3–8 ops).
* `tb_mmio_min.v` — a minimal TB that injects the exact timing sequence into the MMIO interface.

These fixtures must be part of the PR to allow reviewers to run a tiny fast reproducer.

---

## Appendix: commands & quick reference

```bash
# start HW
cd sw/sw_hw
make sim
../obj_dir/Vtb_task_queue

# run SW host
cd sw
make
./test_task_queue_host

# run oracle
python3 model/fifo_model.py sw/logs/trace.csv

# open waveform
gtkwave sw/sw_hw/sim.vcd
```

---

## Contact & attribution

* **Kush Kapoor** — Research implementer
* **Supervisor:** Prof Dr. Rohit Chaurasiya (Indian Institute of Technology Jammu)

---

This bug report is intended for public inclusion in the repository under `bug_report.md` and as a reference for the PR that implements the fixes described above.
