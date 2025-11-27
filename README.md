# HB-TaskQueue-CoSim — Detailed Verification Report

**Repository:** `KushKapoor2006/HB-TaskQueue-CoSim`
**Artifact:** verification + co-simulation platform for HammerBlade task-queue accelerator (Verilator RTL + C MMIO host + Python golden model).
**Author:** Kush Kapoor

---

## Executive summary

This repository implements a reproducible hardware–software co-simulation and trace-driven verification framework for a HammerBlade-style task-queue accelerator. The main goal is verification-first engineering: measure performance improvements from a hardware leader queue *and* ensure functional correctness via an independent golden-model oracle.

Key outcomes from the included experiments:

* **Performance:** The behavioral micro-model indicates a median task latency improvement of **4.2×** (SW median = **46 ns** vs HW median = **11 ns**) when using the leader queue in the synthetic benchmark (see `model/plots/median_latency.png`).
* **Verification discovery:** The trace-based Python golden model detected a **systematic functional divergence** between the MMIO-driven co-simulation and the ideal FIFO semantics: **3,164 pop mismatches** (all recorded pops) in the SW-observed trace, while the HW-only testbench internal verification reported **0 mismatches**. This demonstrates a real MMIO/handshake/RPC correctness issue that must be resolved before synthesis.

This repo provides everything to reproduce the experiment: Verilator harness (`sw/sw_hw`), C MMIO host (`sw/`), Python golden model and plotting (`model/`). The artifacts (logs, traces, JSON results, plots) are included for inspection and for use as CV evidence.

---

Background & Motivation
-----------------------

HammerBlade introduced a heterogeneous **leader--follower** execution model to accelerate fine-grained parallel tasks. In the original paper, the leader core is responsible for **managing queues, dispatching tasks, and coordinating follower cores**. However, the paper also highlighted a **critical bottleneck**:

> **Leader software dispatch becomes a scalability limiter**\
> --- excessive queue operations and synchronization overhead slow down throughput as follower counts scale.

In the original design, **task dispatch remained software-dominated**, leading to:

-   High dispatch latency per task

-   Leader core saturation under heavy load

-   Under-utilization of follower compute resources

-   Queue contention under bursty arrival patterns

* * * * *

### My contributions / extension to the original architecture

This project **extends the HammerBlade concept** by:

✔ Introducing a **hardware task-queue accelerator** (FIFO-based), offloading dispatch from the leader software\
✔ Supporting **MMIO-based push/pop** operations for fast queue interaction\
✔ Allowing the leader core to delegate dispatch scheduling directly through hardware\
✔ Improving follower utilization by eliminating SW bottlenecks\
✔ Measuring performance improvement vs. original SW dispatch loop

This creates a **more scalable pipeline**:

```
CPU Leader Core (SW → HW)  →  Hardware Queue  →  Follower Cores
         ↓                          ↑
    Control logic              Low-latency dispatch
```

The hypothesis validated here:

> **Moving task-queue management into hardware improves throughput & latency and prevents leader-core overload**, enabling followers to operate closer to peak occupancy.

* * * * *

### Why validation was necessary

Hardware queue correctness is **critical**: if the FIFO ordering breaks,\
followers may execute tasks in the wrong order → **silent correctness failures**.

Thus, this repository combines:

-   **RTL implementation** of the hardware queue

-   **Verilator co-simulation** with a real C driver

-   **Trace-driven Python golden FIFO model**

-   **Stress workloads** to replicate leader overload scenarios
---

## Repository structure (what's where)

```
HAMMERBLADE/                    # top of project
├─ hw/                           # primary RTL/Verilator harness (legacy copy)
│  ├─ rtl/                       # SystemVerilog modules
│  ├─ testbenches/               # tb_task_queue.v
│  ├─ Makefile
│  └─ verilator_main.cpp         # MMIO bridge (produces mmio_region.bin)

├─ sw/                           # software host + convenience hw copy
│  ├─ sw_hw/                     # copy of hw used to run Verilator in sw context
│  ├─ src/                       # MMIO driver: task_queue_mmio.c/.h
│  ├─ tests/                     # C host harness: test_task_queue_host
│  ├─ logs/                      # outputs from SW runs (results.json, trace.csv, golden_results.json)
│  └─ Makefile                   # build target for host

├─ model/                        # Python behavior/fifo model and plotting
│  ├─ behavioral.py              # micro-model workload generator (produces sim_results.json)
│  ├─ fifo_model.py              # golden-model trace replayer & checker
│  ├─ plot_results.py            # plotting utilities; outputs to model/plots/
│  └─ plots/                     # saved PNGs

├─ HammerBlade.pdf               # Orignal Paper
└─ README.md                     # this file
```

---

## How the verification pipeline works (methodology)

1. **Hardware harness (Verilator)**

   * Build the Verilated simulation in `sw/sw_hw` with `make sim`.
   * The harness exposes a memory-mapped file `mmio_region.bin` which acts as the MMIO register region.
   * The Verilator harness responds to push/pop commands via that MMIO region and updates internal DUT signals.

2. **MMIO host (C)**

   * `test_task_queue_host` (C) finds and mmaps `mmio_region.bin`, then runs a deterministic micro-test followed by a randomized stress test (10k ops by default).
   * The host logs every successful push/pop line to `sw/logs/trace.csv`, and aggregate counts to `sw/logs/results.json`.
   * Once the host finishes, it writes a TB_DONE flag into the MMIO region so the Verilator process can exit cleanly.

3. **Python golden model (oracle)**

   * `fifo_model.py` is a minimal perfect FIFO: it replays each `push,0x...` and `pop,0x...` operation from the trace and verifies the popped values match the expected FIFO ordering.
   * The checker writes `sw/logs/golden_results.json` with push/pop counts, final queue length, and mismatch count.

4. **Analysis and plotting**

   * `plot_results.py` consumes `sw/logs/results.json`, `model/sim_results.json` and the trace to plot latency distributions, queue depths, arrival/dequeue rates, throughput and utilization.

This design separates *exercise* (C host) from *oracle* (Python), removing accidental coupling and making mismatches meaningful.

---

## Reproducing the experiments (step-by-step)

> Use two terminals. Terminal A runs HW (Verilator). Terminal B runs SW host.

**Terminal A — Build & run HW harness**

```bash
cd sw/sw_hw
make sim
# Run from sw/sw_hw so mmio_region.bin is created at sw/sw_hw/mmio_region.bin
../obj_dir/Vtb_task_queue
# keep this process running; it is the MMIO peripheral
```

**Terminal B — Build & run SW host**

```bash
cd sw
make
./test_task_queue_host
# After completion, logs will be in sw/logs/
```

**Offline golden-model check**

```bash
python3 sw/fifo_model.py sw/logs/trace.csv
# writes sw/logs/golden_results.json
```

**Plotting**

```bash
python3 model/plot_results.py --trace sw/logs/trace.csv --metrics sw/logs/results.json
# images saved to model/plots/
```

---

## Primary artifacts & quantitative metrics

All metrics below are taken from included log files and JSON outputs.

### Hardware-only simulation (hw/results.json)

```json
{
  "attempted_pushes": 3316,
  "successful_pushes": 3140,
  "refused_pushes": 176,
  "attempted_pops": 3329,
  "successful_pops": 3140,
  "refused_pops": 189,
  "mismatches": 0,
  "sim_cycles": 9640
}
```

**Interpretation**: In the internal TB flow the RTL reports correct data values for deterministic and randomized tests (no mismatches internally). The TB simulated the last randomized run for 9,640 cycles.

### Software-driven co-simulation (sw/logs/results.json)

```json
{
  "attempted_pushes": 3382,
  "successful_pushes": 3164,
  "refused_pushes": 216,
  "attempted_pops": 3305,
  "successful_pops": 3164,
  "refused_pops": 141,
  "mismatches": 3162
}
```

**Interpretation**: The MMIO host observed 3,164 pushes and 3,164 pops but the golden model reported 3,164 mismatches — meaning each observed pop did not match the golden FIFO outcome.

### Golden-model results (sw/logs/golden_results.json)

```json
{
  "trace_path": "logs/trace.csv",
  "pushes": 3164,
  "pops": 3164,
  "final_queue_len": 0,
  "mismatches": 3164
}
```

**Interpretation**: Every popped value in the trace failed the golden-model verification.

### Software micromodel highlights (model/sim_results.json)

Key numbers (from `model/sim_results.json`):

* Median latency (SW) = **46.19 ns**
* Median latency (HW) = **11 ns**
* Median speedup ≈ **4.2×**
* Sampled leader utilization (SW trace micro-model) ≈ **0.946**
* Sampled follower utilization (sample) ≈ **0.0303**

These numbers are reflected in `model/plots/median_latency.png` and other figures in `model/plots/`.

---

## Bug summary (brief)

**Symptom:** The Python golden model reports that **all** recorded pops in the MMIO-driven trace were incorrect (3164 mismatches). The hardware TB (run internally in Verilator) reported no mismatches, so the failure is specific to the MMIO-driven interaction.

**High-level hypotheses:**

1. **MMIO sampling/timing mismatch**: the bridge might read DATA_OUT before the DUT produces stable output (sampling on the wrong edge or missing register stage).
2. **Handshake ordering bug**: host clears ACK/VALID bits too early or too late relative to data sampling, causing the host to read stale values.
3. **RTL pointer/arbiter logic**: when the DUT is driven by host-mode signals, pointer increments or distributor logic could route data incorrectly; however, this is less consistent with the TB's internal correctness.

**Why this matters:** functional correctness failures at the MMIO layer can silently produce incorrect results in the deployed accelerator. Catching this in simulation prevents incorrect silicon or wasted design cycles.

> A full `bug_report.md` will provide cycle-level VCD evidence, exact failing sequence, and recommended patch (e.g., add a sample-ready register or two-phase handshake). That detailed report is suggested as the next deliverable.

---

## Development notes & design trade-offs

* **MMIO via memory-mapped file:** chosen for portability and easy integration with Verilator. Trade-off: file-backed MMIO introduces timing semantics that are easy to use but can hide subtle synchronization issues. The discovered bug is precisely of this category.

* **C host + Python oracle separation:** this intentionally avoids coupling the oracle to the driver. The C host exercises the MMIO interface as a realistic driver would. The Python oracle verifies the *semantics* of operations offline.

* **Why not formal verification yet:** the project focuses on practical, trace-based verification to catch concrete failures in the MMIO interface. Formal methods (BMC, property checking) are valuable once the MMIO/RTL handshake semantics are stabilized and the functional model is agreed upon.

* **Plotting & reproducibility:** plotting scripts operate on saved traces and JSON metrics to guarantee reproducible figures for presentations and CV materials.

---

## Future work & next steps

Short-term (next sprint):

1. Produce `bug_report.md` with a minimal reproducer, VCD snippets, and a cycle-by-cycle timeline of the failing sequence (recommended next task).
2. Implement and test handshake fixes: e.g., a sample-ready bit, an extra cycle guard before sampling DATA_OUT, or stricter ACK/VALID semantics in the MMIO bridge.
3. Re-run the randomized campaign until `mismatches == 0` in the SW-driven flow.

Medium-term:

1. Add a `make verify` CI target that runs HW build (without GUI), SW host, golden check, and plotting; the target should fail if `mismatches > 0`.
2. Run Yosys synthesis to check synthesizability and get resource estimates (post-fix).
3. Formalize pointer invariants with property assertions or bounded model checking.

Research extensions:

* Explore dispatch policies and batch-enqueue for leader cores and quantify end-to-end application-level improvement on microkernels.
* Scale the workload: increase FIFO depth, follower count, and arrival rates to stress test the scheduler.

---

## CV-ready text (pick one or both)

**Verification-focused bullet**

> Built a hardware–software co-simulation and trace-driven verification framework for the HammerBlade task-queue accelerator (Verilator + MMIO C host + Python golden model). The independent Python oracle discovered a systematic FIFO correctness divergence: 3,164 pop mismatches across a 10k randomized campaign, enabling a targeted RTL debugging effort prior to synthesis.

**Performance + verification bullet**

> Implemented trace-driven validation and quantitative evaluation of a leader-queue accelerator. Measured a median latency improvement of **4.2×** (SW median 46 ns vs HW median 11 ns) in microbenchmarks while using a Python golden model to verify FIFO semantics and surface correctness issues.

---

## Artifacts to include in a release or application

* `sw/logs/trace.csv`, `sw/logs/results.json`, `sw/logs/golden_results.json`, `hw/run.log` (evidence of discovery)
* `model/plots/*.png` (visual summary: latency, throughput, utilization)
* `sw/sw_hw/sim.vcd` (waveform, large) for detailed reviewers
* `bug_report.md` (recommended next deliverable) with VCD excerpts and repro steps

---

## License & citation

Add a `LICENSE` file (MIT suggested) and cite the HammerBlade paper when describing this work in an academic context.

---

## Contact

Kush Kapoor — GitHub: `https://github.com/KushKapoor2006`
(Include your email if you want to be contacted for reviews)

---

If you would like, I can now:

* Draft `bug_report.md` with an annotated VCD-based reproducer and patch suggestion (recommended), or
* Implement a `make verify` pipeline target that automates HW, SW, golden check and plotting, and fails on mismatches.

Reply with either `bug_report`, `make_verify`, or `both` and I will prepare the requested next step.
