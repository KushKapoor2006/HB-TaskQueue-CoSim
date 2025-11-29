# bug_report.md — HammerBlade_validation

**Author:** Kush Kapoor
**Target doc:** Reproducible bug report for the MMIO-driven FIFO correctness divergence discovered during co-simulation.

---

## Summary (TL;DR)

During HW↔SW co-simulation of the HammerBlade task-queue accelerator, the Python golden FIFO model reported **3164 pop mismatches** (every pop recorded in the SW trace disagreed with the expected FIFO value). The hardware-only testbench (Verilator TB) reported **0 mismatches** in its internal verification. The failure is therefore localized to the **MMIO-driven interaction** (bridge/handshake semantics, sampling timing, or host sampling order).

This document provides: reproduction steps, minimal reproducer guidance, waveform/VCD analysis checklist, root-cause hypotheses, concrete patch proposals, validation testcases, and CI/automation suggestions.

---

## A. Artifacts & evidence (what we have)

Files produced by the experiments (included in repository):

* `sw/logs/trace.csv` — chronological record of successful push/pop events recorded by the C host (format: `op,0xXXXXXXXX`).
* `sw/logs/results.json` — aggregated counts and basic metrics from the SW host run.
* `sw/logs/golden_results.json` — Python golden model output (mismatches count).
* `hw/run.log` and `hw/results.json` — HW-only TB logs and summary (internal verification shows 0 mismatches).
* `sw/sw_hw/sim.vcd` — Verilator waveform (large). Use for cycle-level debugging.

These artifacts provide a reproducible failing scenario: run Verilator harness in `sw/sw_hw`, run `./test_task_queue_host` in `sw/`, then run `python3 model/fifo_model.py sw/logs/trace.csv` to reproduce `mismatches == 3164`.

---

## B. Reproduction steps (minimal, exact)

1. **Start HW harness (Terminal A)**

```bash
cd sw/sw_hw
make sim
# run from sw/sw_hw so mmio_region.bin is created in that directory
../obj_dir/Vtb_task_queue
# keep this running; the process exposes mmio_region.bin and listens to MMIO ops
```

2. **Run SW host (Terminal B)**

```bash
cd sw
make        # builds ./test_task_queue_host
./test_task_queue_host
# This runs deterministic + randomized campaign and writes sw/logs/trace.csv
```

3. **Run Python golden checker**

```bash
python3 model/fifo_model.py sw/logs/trace.csv
# This writes sw/logs/golden_results.json which should show mismatches
```

4. **Inspect outputs**

* Confirm SW `results.json` and `golden_results.json` show mismatches.
* Open `sw/sw_hw/sim.vcd` in a waveform viewer (e.g., GTKWave) and locate the cycles corresponding to failing ops.

---

## C. Minimal reproducer (scripted sequence)

To reduce noise and isolate the failing behavior, create a minimal push/pop sequence that replicates the mismatch pattern deterministically. The host currently runs randomized operations; instead, script this deterministic sequence:

* Steps:

  1. Push A
  2. Push B
  3. Pop -> expect A
  4. Pop -> expect B

If this fails reproducibly when driven through the MMIO bridge (but passes in the TB), then it proves the mismatch occurs at the bridge sampling boundary rather than exotic concurrency.

**Minimal TB idea:** Modify or add a small testbench `tb_mmio_min.v` that pulses `host_push_req`/`host_pop_req` signals at the exact cycles matching the host script. Run the Verilator harness with this TB and compare outputs.

---

## D. VCD / waveform analysis checklist (what to look for)

Open `sw/sw_hw/sim.vcd` in GTKWave or a similar viewer. Add these signals to the view (names may vary — search in `Vtb_task_queue.h` or generated headers for exact signal names):

* `clk` (clock)
* `reset`
* `mmio_host_push_req` / `host_push_req`
* `mmio_host_pop_req` / `host_pop_req`
* `DATA_IN` / `DATA_OUT`
* `valid_out` / `host_valid`
* `full` / `host_full`
* `push_ack` / `push_resp` / `pop_ack` (any ack/ready bits)
* `read_ptr`, `write_ptr` (if available in core)
* distributor / arbiter internal signals (bank selects)

**Inspect sequence around one failing pop**:

1. Identify the timestamp (cycle) of the pop event from `sw/logs/trace.csv`. Convert to simulation cycle if the C host logged cycle numbers.
2. In wave viewer, go to the cycle where the host toggles `pop_req`.
3. Check: is `DATA_OUT` stable before the host samples it? Is `valid_out` asserted during sampling? Is any ack toggled prematurely?
4. Check pointer regs: do `read_ptr` / `write_ptr` values correspond to expected FIFO indices before/after the pop?
5. Check for glitching or broad combinational windows where `DATA_OUT` changes on the same cycle it is sampled.

**Look for patterns:**

* If host reads a value that equals the *previous* pop value (stale data), the bridge likely sampled too early.
* If host reads constant or zeroed value, bridging or mapping may be misaligned.
* If read_ptr increments but DATA_OUT is unchanged, ordering/timing mismatch exists.

---

## E. Root-cause hypotheses (ranked)

1. **MMIO sampling/timing mismatch (highest probability)**

   * The MMIO bridge samples `DATA_OUT` on a clock edge before the DUT's output register is updated. Event ordering differences between TB harness and MMIO bridge cause SW to observe stale values.

2. **ACK/VALID handshake ordering bug**

   * The host clears `VALID` or toggles `ACK` in a way that permits the next pop to read old data. For example: host asserts `pop_req` and immediately reads `DATA_OUT` without waiting for `valid` to settle.

3. **Host-memory mapping race**

   * The memory-mapped file semantics may allow partial writes or caching causing the host to read stale data under certain scheduling.

4. **Distributor/arbiter routing bug under host-mode**

   * Under TB-only mode a different code path triggers correct routing; but under host-mode the arbiter routes from incorrect bank index.

5. **Pointer update ordering (RTL)**

   * The core may increment pointers before finishing data transfer or writing `DATA_OUT`, so the value read corresponds to a pointer not yet filled correctly.

---

## F. Concrete repair suggestions (fix candidates)

These are proposed patches ordered by invasiveness and likelihood of success.

### 1) *Sample-ready* bit (recommended)

* Add a `sample_ready` bit that the DUT asserts once `DATA_OUT` and `valid_out` are stable for a full clock cycle.
* The bridge must wait for `sample_ready == 1` before allowing the host to read `DATA_OUT`.
* Advantage: minimal RTL change, explicit synchronization point.
* Validation: add assertions in TB that `sample_ready` is high whenever `valid_out` is true and `DATA_OUT` matches expected.

### 2) *Two-cycle read handshake* (simple bridge change)

* Modify `verilator_main.cpp` / MMIO bridge to require the host to issue `pop_req`, then wait one extra clock cycle before reading `DATA_OUT` and acknowledging `pop_ack`.
* Advantage: no RTL change, easier to test quickly.
* Drawback: may mask true RTL races; better to add sample-ready eventually.

### 3) *Reorder pointer updates in RTL* (if pointers are suspect)

* Ensure that `DATA_OUT` is written to the output register **before** the read pointer updates. If pointer increment precedes data stable, swap order or add register stage.

### 4) *Tighter VALID/ACK semantics* (protocol redesign)

* Enforce `valid_out` is sticky until host issues an explicit `pop_ack`, ensuring the bridge cannot sample data in between transitions.

### 5) *Bridge-level atomic read* (file-backed MMIO approach)

* Implement atomic read semantics for the MMIO file region (advisory locking around the 64-bit read) to avoid partially-updated views if host and simulator thread race. This is lower priority; more likely the issue is cycle-level.

---

## G. Validation & test plan (post-fix)

After applying a candidate fix, run the following tests in order:

1. **Minimal deterministic sequence** (fast): push A, push B, pop→expect A, pop→expect B. Repeat 100x. Ensure no mismatches.
2. **Deterministic micro-test** (existing host deterministic run) — ensure the deterministic section of the host passes.
3. **Randomized small campaign** (1k ops): verify `golden_results.json` shows 0 mismatches.
4. **Full randomized campaign** (10k ops): verify `golden_results.json` shows 0 mismatches and compare stats with previous runs for regression.
5. **Regression TB**: run HW-only TB harness and verify internal TB mismatches remain 0.
6. **VCD inspection**: for at least 10 failing sequences previously recorded, confirm sampling timing is corrected and `DATA_OUT` is stable when sampled.

**CI target:** Implement `make verify` which runs steps 1–4 and fails if `mismatches > 0`.

---

## H. Minimal code diffs (pseudocode)

### Bridge: introduce 1-cycle sample delay (quick patch)

```cpp
// in verilator_main.cpp (host side MMIO read handler)
// before: read DATA_OUT immediately when pop_req observed
// after: when pop_req observed -> tick one cycle -> sample DATA_OUT
```

### RTL: sample_ready register (more robust)

```verilog
// inside hb_task_queue_core.sv
reg sample_ready_reg;
reg [DATA_W-1:0] data_out_reg;

always @(posedge clk) begin
  if (reset) begin
    sample_ready_reg <= 0;
    data_out_reg <= 0;
  end else begin
    data_out_reg <= internal_data_bus; // produced by FIFO stage
    sample_ready_reg <= valid_internal && (data_out_reg == internal_data_bus);
  end
end

// expose sample_ready as read-only MMIO status bit
```

---

## I. Logging and instrumentation recommendations

* **Add host-stamped cycle numbers** to `sw/logs/trace.csv` so every trace line includes a simulation cycle and host timestamp: `op,0x...,cycle`.
* **Add per-op unique IDs** (monotonic counter) to correlate host logs with DUT logs and VCD markers (write request id into a small register field that is visible in VCD).
* **Increase VCD granularity** around MMIO signals for a small time window (use command-line options to dump only a subset of signals to reduce file size).
* **Add assertions** in RTL for pointer sanity: `assert(read_ptr != write_ptr || !valid_out)` under certain conditions and enable them in simulation builds.

---

## J. CI & automation (Make targets)

Add to `sw/Makefile`:

```make
.PHONY: verify
verify: sim_host golden_check
	@python3 model/plot_results.py --trace sw/logs/trace.csv --metrics sw/logs/results.json
	@if [ $$(jq .mismatches sw/logs/golden_results.json) -ne 0 ]; then \
		echo "Verification failed: mismatches > 0"; exit 2; \
	fi

sim_host:
	# run the HW harness in background (or require it running), then run host
	cd sw/sw_hw && ../obj_dir/Vtb_task_queue &
	cd sw && ./test_task_queue_host

golden_check:
	python3 model/fifo_model.py sw/logs/trace.csv
```

CI runner should run `make verify` inside a container with Verilator installed. If backgrounding HW is flaky, prefer to spawn the Verilator binary inside a screen or background process that can be killed by the verify target after completion.

---

## K. Reporting checklist (for PR / issue)

When opening the bug fix PR, provide:

* Small failing trace snippet (3–10 ops) in `tests/fixtures/` plus expected outputs
* VCD snippet showing the timing failure (annotated screenshot or `.wcfg` saved view)
* Proposed patch (bridge or RTL change) with unit/regression tests
* Re-run of `make verify` showing green

---

## L. Appendix: quick reference commands

```bash
# 1. Build HW
cd sw/sw_hw; make sim
# 2. Run HW
../obj_dir/Vtb_task_queue
# 3. Run SW host
cd sw; make; ./test_task_queue_host
# 4. Golden check
python3 model/fifo_model.py sw/logs/trace.csv
# 5. Waveform: open sim.vcd (GTKWave)
gtkwave sw/sw_hw/sim.vcd
```

---

## M. Contact & next actions

If you want I will:

1. Implement the **two-cycle bridge delay** quick patch and run the verification suite (fast); or
2. Implement the **sample_ready** RTL change (cleaner) and create unit tests + CI; or
3. Produce `tests/fixtures/min_repro` with deterministic traces and a TB that reproduces the issue for PR inclusion.

Reply with `bridge_patch`, `rtl_sample_ready`, or `min_repro` (or `all`) and I will implement and run the tests.
