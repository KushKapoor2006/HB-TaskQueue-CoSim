# HammerBlade — Detailed Verification & Co‑simulation Report

* **Role:** Research assistant / intern project (implementer: Kush Kapoor)
* **Supervisor:** Prof Dr. Rohit Chaurasiya (Indian Institute of Technology Jammu)

---

## TL;DR

Hardware acceleration of HammerBlade’s leader/follower dispatch bottleneck, validated through MMIO-driven co-simulation and independent Python FIFO correctness checking. It combines:

* cycle-accurate RTL simulation (Verilator),
* a native C MMIO host driver that exercises the accelerator via a memory-mapped region,
* an independent Python FIFO golden model (oracle) that replays host traces and validates FIFO semantics, and
* plotting / analysis scripts for quantitative evaluation.

**Key quantitative takeaways (from included runs):**

* **Median latency (micro-model):** SW = **46.19 ns**, HW = **11 ns** → **~4.2×** median improvement.
* **HW-only randomized run (Verilator TB):** 3,140 successful pushes & pops, **0 internal TB mismatches** (sim_cycles = 9640). (`hw/results.json`)
* **SW-driven co-simulation (MMIO + host):** 3,164 successful pushes & pops observed by the host; **3,164 pop mismatches** reported by the Python oracle — systematic functional divergence discovered during verification. (`sw/logs/golden_results.json`)

**Takeaway:** performance improvement is promising, but a correctness failure in the MMIO-driven flow must be resolved before synthesis or deployment. The verification pipeline prevented incorrect hardware from moving forward.

---

## Table of contents

1. [Overview & Motivation](#overview--motivation)
2. [What I extended from HammerBlade (research contribution)](#what-i-extended-from-hammerblade-research-contribution)
3. [Project summary & contributions](#project-summary--contributions)
4. [Repository layout (high-level)](#repository-layout-high-level)
5. [Quickstart & reproducible pipeline](#quickstart--reproducible-pipeline)
6. [Design & RTL modules (brief)](#design--rtl-modules-brief)
7. [Verification methodology](#verification-methodology)
8. [Results & figures (included)](#results--figures-included)
9. [Brief bug analysis & hypotheses](#brief-bug-analysis--hypotheses)
10. [Development notes & design trade-offs](#development-notes--design-trade-offs)
11. [Future work & next steps](#future-work--next-steps)
12. [License & contact](#license--contact)

---

## Overview & Motivation

The HammerBlade architecture separates *leader* and *follower* roles: leaders manage queues and dispatch, while followers execute tasks. Prior work demonstrated large speedups by moving dispatch closer to hardware, but also exposed a **leader-software dispatch bottleneck**: as follower counts scale, the leader’s software dispatch code becomes a throughput and latency limiter (queue operations, synchronization and context overhead). This project aims to:

* implement a hardware task-queue accelerator (FIFO) to offload dispatch from software leaders, and
* build a rigorous HW↔SW verification pipeline to ensure the hardware is functionally correct under realistic driver workloads.

Performance without correctness is meaningless for accelerators — this project makes correctness the first-class objective.

---

## What I extended from HammerBlade (research contribution)

This work is explicitly an extension of the HammerBlade paper (not a replacement). My contributions extend the original leader/follower idea by:

* **Implementing a hardware FIFO task queue** that exposes low-latency MMIO push/pop operations to the leader core, enabling the leader to enqueue tasks with few cycles of overhead.
* **Designing a complete MMIO host driver** and co‑simulation flow so the RTL is exercised using a realistic software stack instead of synthetic TB-only inputs.
* **Building a trace-driven verification pipeline**: the C driver logs successful push/pop events; a Python golden FIFO model replays the trace and checks exact value ordering and underflow/overflow conditions.
* **Identifying functional MMIO-level divergence** (3,164 mismatches) which would otherwise remain hidden if only TB-only tests were used.

Why this extension matters: the original HammerBlade work showed the potential for hardware-assisted dispatch — this project shows how to *safely* realize that potential in practice by pairing performance evaluation with semantic verification.

---

## Project summary & contributions

* **Architecture implementation:** SystemVerilog RTL for a FIFO-based task-queue (core modules: queue core, distributor/arbiter, small MMIO interface). Files live in `hw/rtl/` and `sw/sw_hw/`.
* **Co-simulation harness:** Verilator-based hardware harness (`sw/sw_hw`), C MMIO host driver (`sw/`), and a small MMIO protocol implemented with an mmap-backed file (`mmio_region.bin`) for portability.
* **Oracle & analysis:** `model/fifo_model.py` (golden reference) and `model/plot_results.py` to produce CV-quality plots.
* **Validation campaign:** deterministic micro-tests + large randomized stress (10k ops) and cross-validation of hardware-only TB vs SW-observed behavior.
* **Discovery:** Found a systematic FIFO correctness failure in the MMIO-driven flow; collected reproducible evidence and a debugging roadmap.

---

## Repository layout (high-level)

```
HB-TaskQueue-CoSim/
├─ hw/                    # vanilla RTL + testbench for HW-only TB
│  ├─ rtl/
│  ├─ testbenches/
│  └─ verilator_main.cpp

├─ sw/                    # SW host + convenience copy of hw (sw_hw)
│  ├─ sw_hw/              # copy of hw used to run Verilator from sw/ context
│  ├─ src/                # task_queue_mmio.c / .h
│  ├─ tests/              # test_task_queue_host.c
│  └─ logs/               # run outputs: results.json, trace.csv, golden_results.json

├─ model/                 # Python models and plotting utilities
│  ├─ behavioral.py
│  ├─ fifo_model.py
│  └─ plot_results.py

├─ outputs/               # optional: large artifacts (VCDs, waveforms, images)
├─ Hammerblade.pdf        # Orignal Hammerblade model paper
└─ README.md              # this file
```

---

## Quickstart & reproducible pipeline

### Prerequisites

* Linux or WSL
* Verilator (>= 4.0 recommended)
* GCC / clang
* Python 3.8+ with `numpy`, `matplotlib`, `pandas` (for plotting; optional)

Install Python deps quickly:

```bash
pip3 install numpy matplotlib pandas
```

### 1) Start the hardware (Verilator harness)

Open terminal A:

```bash
cd sw/sw_hw
make sim
# run from sw/sw_hw to create sw/sw_hw/mmio_region.bin
../obj_dir/Vtb_task_queue
# leave running; it behaves like the MMIO peripheral
```

### 2) Run the software host test

Open terminal B:

```bash
cd sw
make
./test_task_queue_host
# writes logs to sw/logs/: trace.csv, results.json
```

### 3) Run the Python golden checker (offline)

```bash
python3 model/fifo_model.py sw/logs/trace.csv
# writes sw/logs/golden_results.json
```

### 4) Produce plots (optional)

```bash
python3 model/plot_results.py --trace sw/logs/trace.csv --metrics sw/logs/results.json
# PNGs written to model/plots/
```

---

## Design & RTL modules (brief)

*(See the source files in `hw/rtl/` for full details.)*

* **hb_task_queue_core.sv** — FIFO core and high‑level push/pop interface.
* **hb_task_distributor.sv** — Handles distribution of tasks among banks/followers.
* **hb_arbiter_banked.sv** — Banked arbiter to service multiple followers.
* **verilator_main.cpp** — MMIO bridge + Verilator harness. Maps `mmio_region.bin` and implements a simple host handshake.

Key interfaces:

* **MMIO region** includes control (push_req / pop_req), DATA_IN, DATA_OUT, STATUS bits (FULL, VALID), and ACK flags for push/pop outcomes.
* Handshake correctness is essential: host should sample `DATA_OUT` only when `VALID` is asserted and stable.

---

## Verification methodology

This project emphasizes trace-based semantic verification:

1. **Exercise layer (C host):** realistic workload generation and mixed push/pop operations; logs each successful push/pop to `sw/logs/trace.csv`.
2. **Oracle layer (Python):** independent FIFO model replays trace and verifies exact LIFO ordering, underflow/overflow and value equality.
3. **Cross-check:** any mismatch is flagged and recorded with reproducible evidence (trace, logs, waveforms).
4. **Comparison:** HW-only TB vs SW-driven co-sim — differences indicate MMIO/bridge-level or surrounding handshake faults.

This separation avoids the common pitfall of coupling driver and oracle and provides strong evidence when mismatches appear.

---

## Results & figures (included)

**Hardware-only (Verilator TB)** — `hw/results.json`

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

**Software-driven co-sim** — `sw/logs/results.json`

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

**Golden-model** — `sw/logs/golden_results.json`

```json
{
  "trace_path": "logs/trace.csv",
  "pushes": 3164,
  "pops": 3164,
  "final_queue_len": 0,
  "mismatches": 3164
}
```

**Behavioral micro-model** — `model/sim_results.json` (highlights)

* Median latency (SW) = **46.19 ns**
* Median latency (HW) = **11 ns**
* Median speedup ≈ **4.2×**
* Leader util (sample) ≈ **0.946**
* Follower util (sample) ≈ **0.0303**

**Plots included** (see `model/plots/`)

* `median_latency.png`, `throughput.png`, `utilization.png`, `queue_arrival_hw.png`, `queue_arrival_sw.png`, `rates_hw.png`, `rates_sw.png`.

Interpretation summary: HW-only TB indicates the RTL *can* behave correctly (0 TB mismatches). However, when exercised via the host MMIO driver we observed a systematic value-ordering failure (3164 mismatches). This strongly suggests MMIO handshake/timing correctness issues that must be fixed.

---

## Brief bug analysis & hypotheses

**Observed symptom:** every recorded pop in the MMIO trace disagrees with the golden-model expected value (3164/3164 mismatches). The number of successful pushes/pops matches (no lost operations), indicating values returned are incorrect rather than ops not occurring.

**Top hypotheses:**

1. **MMIO sampling/timing error** — data is read by host before `DATA_OUT` is stable.
2. **ACK/VALID ordering bug** — clearing or asserting ACK bits in the wrong order produces stale reads.
3. **RTL pointer/arbiter ordering corner case** — when mixed push/pop timing occurs at MMIO boundaries, pointers or bank switching can route wrong words.

A full `bug_report.md` with VCD screenshots and cycle-level analysis is also attached in the re[ository

---

## Development notes & design trade-offs

* **MMIO via mmap file**: simple and portable; it mimics memory-mapped registers without requiring device emulation. Trade-off: file IO / mmap semantics can hide or create race conditions if host and DUT sampling are not carefully synchronized.
* **Separated oracle**: The Python model is intentionally independent. This prevented false positives caused by coupling and gave strong evidence when mismatches appeared.
* **Why not synthesize first?** Functional correctness must be established before committing to synthesis or P&R. Synthesis can be done later (Yosys flow is applicable once bugs are fixed).

---

## Future work & next steps

1. Add `make verify` CI target: build HW, run host, run python oracle, produce plots; fail on mismatches.
2. Run Yosys synthesis to extract resource estimates and validate synthesizability (post-fix).
3. Explore formal checks on pointer invariants and bound-checking once functional semantics are stable.
4. Evaluate dispatch policies (batch enqueue, prioritized queues).
5. Scale up follower counts and queue depths; measure stability and tail latency.
6. Integrate into a microkernel test harness and measure end-to-end application-level speedups.

---

## License & contact

MIT License

**Contact:** [kushkapoor.kk1234@gmail.com](mailto:kushkapoor.kk1234@gmail.com)

---
