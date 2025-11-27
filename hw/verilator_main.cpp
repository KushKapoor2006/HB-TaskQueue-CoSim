// verilator_main.cpp
// Verilator host harness: writes outputs into ./outputs directory (sim.vcd, results.json, run.log, metrics.csv).
#include "Vtb_task_queue.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <iostream>
#include <deque>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>

using namespace std;

// Global pointers
static Vtb_task_queue *top = nullptr;
static VerilatedVcdC *tfp = nullptr;
static uint64_t sim_time = 0;
static uint64_t cycles = 0;

// Metrics struct
struct Metrics {
    uint64_t attempted_pushes = 0;
    uint64_t successful_pushes = 0;
    uint64_t refused_pushes = 0;
    uint64_t attempted_pops = 0;
    uint64_t successful_pops = 0;
    uint64_t refused_pops = 0;
    uint64_t mismatches = 0;
    uint64_t sim_cycles = 0;
} metrics;

// Utility: ensure outputs directory exists (mode 0755)
static void ensure_outputs_dir() {
    const char *dir = "outputs";
    struct stat st;
    if (stat(dir, &st) == 0) {
        if (!S_ISDIR(st.st_mode)) {
            fprintf(stderr, "Error: 'outputs' exists and is not a directory\n");
            exit(1);
        }
        return;
    }
    if (mkdir(dir, 0755) != 0) {
        perror("mkdir outputs");
        exit(1);
    }
}

// Write a human-readable log (append)
static void write_log_header(FILE *logf) {
    time_t now = time(NULL);
    char tbuf[64];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S UTC", gmtime(&now));
    fprintf(logf, "=== Simulation run at %s ===\n", tbuf);
}

// Write metrics JSON into outputs/results.json (simple formatting)
static void write_results_json(const char *path, const Metrics &m) {
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("fopen results.json");
        return;
    }
    fprintf(f, "{\n");
    fprintf(f, "  \"attempted_pushes\": %llu,\n", (unsigned long long)m.attempted_pushes);
    fprintf(f, "  \"successful_pushes\": %llu,\n", (unsigned long long)m.successful_pushes);
    fprintf(f, "  \"refused_pushes\": %llu,\n", (unsigned long long)m.refused_pushes);
    fprintf(f, "  \"attempted_pops\": %llu,\n", (unsigned long long)m.attempted_pops);
    fprintf(f, "  \"successful_pops\": %llu,\n", (unsigned long long)m.successful_pops);
    fprintf(f, "  \"refused_pops\": %llu,\n", (unsigned long long)m.refused_pops);
    fprintf(f, "  \"mismatches\": %llu,\n", (unsigned long long)m.mismatches);
    fprintf(f, "  \"sim_cycles\": %llu\n", (unsigned long long)m.sim_cycles);
    fprintf(f, "}\n");
    fclose(f);
}

// Write a small CSV summary (headers + one row)
static void write_metrics_csv(const char *path, const Metrics &m) {
    FILE *f = fopen(path, "w");
    if (!f) {
        perror("fopen metrics.csv");
        return;
    }
    fprintf(f, "attempted_pushes,successful_pushes,refused_pushes,attempted_pops,successful_pops,refused_pops,mismatches,sim_cycles\n");
    fprintf(f, "%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu\n",
            (unsigned long long)m.attempted_pushes,
            (unsigned long long)m.successful_pushes,
            (unsigned long long)m.refused_pushes,
            (unsigned long long)m.attempted_pops,
            (unsigned long long)m.successful_pops,
            (unsigned long long)m.refused_pops,
            (unsigned long long)m.mismatches,
            (unsigned long long)m.sim_cycles);
    fclose(f);
}

// Clock tick: falling + rising, dump VCD at each half-step
static void tick() {
    // falling edge
    top->clk = 0;
    top->eval();
    if (tfp) tfp->dump(sim_time++);
    // rising edge
    top->clk = 1;
    top->eval();
    if (tfp) tfp->dump(sim_time++);
    cycles++;
}

// Reset N rising edges (assert during ticks)
static void reset_cycles(int n) {
    top->reset = 1;
    for (int i=0;i<n;i++) tick();
    top->reset = 0;
    tick(); // one cycle after deassert
}

// Host helpers (non-blocking semantics)
static int host_try_push(uint32_t v) {
    metrics.attempted_pushes++;
    if (top->full) {
        metrics.refused_pushes++;
        return -1;
    }
    top->host_push_req = 1;
    top->host_data_in = v;
    tick();
    top->host_push_req = 0;
    top->host_data_in = 0;
    metrics.successful_pushes++;
    return 0;
}

static int host_try_pop(uint32_t *out) {
    metrics.attempted_pops++;
    if (!top->valid_out) {
        metrics.refused_pops++;
        return -1;
    }
    uint32_t sampled = (uint32_t)(top->data_out & 0xFFFFFFFF);
    top->host_pop_req = 1;
    tick();
    top->host_pop_req = 0;
    *out = sampled;
    metrics.successful_pops++;
    return 0;
}

// Deterministic test
static uint64_t run_deterministic_test(FILE *logf) {
    fprintf(logf, "[HOST] Running deterministic test...\n");
    fflush(logf);
    deque<uint32_t> sw;
    metrics = Metrics();
    top->host_mode = 1;
    reset_cycles(4);

    auto push = [&](uint32_t v) {
        int rc = host_try_push(v);
        if (rc == 0) {
            sw.push_back(v);
            fprintf(logf, "push OK 0x%08x\n", v);
        } else {
            fprintf(logf, "push REFUSED 0x%08x\n", v);
        }
    };
    auto pop = [&]() {
        uint32_t out;
        int rc = host_try_pop(&out);
        if (rc == 0) {
            if (sw.empty()) {
                fprintf(logf, "MISMATCH: popped but SW empty -> 0x%08x\n", out);
                metrics.mismatches++;
            } else {
                uint32_t expected = sw.front();
                sw.pop_front();
                if (expected != out) {
                    fprintf(logf, "MISMATCH: expected 0x%08x got 0x%08x\n", expected, out);
                    metrics.mismatches++;
                } else {
                    fprintf(logf, "pop OK 0x%08x\n", out);
                }
            }
        } else {
            if (!sw.empty()) {
                fprintf(logf, "MISMATCH: pop refused but SW had data\n");
                metrics.mismatches++;
            } else {
                fprintf(logf, "pop REFUSED (empty)\n");
            }
        }
    };

    push(0xA5A5A5A5);
    push(0xDEADBEEF);
    push(0x01234567);
    push(0x89ABCDEF);
    pop();
    pop();
    push(0xCAFEBABE);
    push(0x0BADF00D);
    while (!sw.empty()) pop();

    top->tb_done = 1;
    fprintf(logf, "[HOST] deterministic test done. cycles simulated: %llu\n", (unsigned long long)cycles);
    return metrics.mismatches;
}

// Randomized test
static uint64_t run_randomized_test(FILE *logf, unsigned seed, int ops=10000) {
    fprintf(logf, "[HOST] Running randomized test seed=%u ops=%d\n", seed, ops);
    fflush(logf);
    deque<uint32_t> sw;
    metrics = Metrics();
    top->host_mode = 1;
    top->tb_done = 0;
    reset_cycles(4);

    srand(seed);
    for (int i=0;i<ops;i++) {
        int op = rand() % 3;
        if (op == 0) {
            uint32_t v = (uint32_t)rand();
            int rc = host_try_push(v);
            if (rc == 0) {
                sw.push_back(v);
            }
        } else if (op == 1) {
            uint32_t out;
            int rc = host_try_pop(&out);
            if (rc == 0) {
                if (sw.empty()) {
                    fprintf(logf, "MISMATCH: popped but SW empty -> 0x%08x\n", out);
                    metrics.mismatches++;
                } else {
                    uint32_t expected = sw.front();
                    sw.pop_front();
                    if (expected != out) {
                        fprintf(logf, "MISMATCH: expected 0x%08x got 0x%08x\n", expected, out);
                        metrics.mismatches++;
                    }
                }
            } else {
                if (!sw.empty()) {
                    fprintf(logf, "MISMATCH: driver refused pop but SW had data\n");
                    metrics.mismatches++;
                }
            }
        } else {
            tick();
        }
    }

    // drain
    while (!sw.empty()) {
        uint32_t out;
        int rc = host_try_pop(&out);
        if (rc == 0) {
            uint32_t expected = sw.front();
            sw.pop_front();
            if (expected != out) {
                fprintf(logf, "MISMATCH: expected 0x%08x got 0x%08x\n", expected, out);
                metrics.mismatches++;
            }
        } else {
            fprintf(logf, "MISMATCH: expected to pop remaining but hardware refused\n");
            metrics.mismatches++;
            break;
        }
    }

    top->tb_done = 1;
    fprintf(logf, "[HOST] randomized test done. cycles simulated: %llu\n", (unsigned long long)cycles);
    return metrics.mismatches;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);

    // Ensure outputs directory
    ensure_outputs_dir();

    // Open run log
    FILE *logf = fopen("outputs/run.log", "w");
    if (!logf) {
        perror("fopen run.log");
        return 1;
    }
    write_log_header(logf);

    // create model & tracing
    top = new Vtb_task_queue;
    Verilated::traceEverOn(true);
    tfp = new VerilatedVcdC;
    // place VCD in outputs/
    top->trace(tfp, 99);
    tfp->open("outputs/sim.vcd");

    // initial signals
    top->clk = 0;
    top->reset = 1;
    top->host_mode = 1;
    top->host_push_req = 0;
    top->host_pop_req = 0;
    top->host_data_in = 0;
    top->tb_done = 0;
    top->eval();
    if (tfp) tfp->dump(sim_time++);

    // Run deterministic test
    cycles = 0;
    uint64_t mism1 = run_deterministic_test(logf);

    // Run randomized test
    cycles = 0;
    metrics = Metrics();
    unsigned seed = (unsigned)time(NULL);
    uint64_t mism2 = run_randomized_test(logf, seed, 10000);

    // Print summary to stdout and log
    fprintf(logf, "================= RESULTS =================\n");
    fprintf(logf, "Random test mismatches: %llu\n", (unsigned long long)mism2);
    fprintf(logf, "Random test pushed: attempts=%llu success=%llu refused=%llu\n",
            (unsigned long long)metrics.attempted_pushes,
            (unsigned long long)metrics.successful_pushes,
            (unsigned long long)metrics.refused_pushes);
    fprintf(logf, "Random test popped: attempts=%llu success=%llu refused=%llu\n",
            (unsigned long long)metrics.attempted_pops,
            (unsigned long long)metrics.successful_pops,
            (unsigned long long)metrics.refused_pops);
    fprintf(logf, "Cycles simulated (last run): %llu\n", (unsigned long long)cycles);
    fflush(logf);

    cout << "[HOST] randomized test mismatches: " << mism2 << endl;
    cout << "[HOST] pushed: attempts=" << metrics.attempted_pushes << " success=" << metrics.successful_pushes << " refused=" << metrics.refused_pushes << endl;
    cout << "[HOST] popped: attempts=" << metrics.attempted_pops << " success=" << metrics.successful_pops << " refused=" << metrics.refused_pops << endl;
    cout << "[HOST] cycles simulated: " << cycles << endl;

    // Write structured artifacts into outputs/
    metrics.sim_cycles = cycles;
    write_results_json("outputs/results.json", metrics);
    write_metrics_csv("outputs/metrics.csv", metrics);

    // Close and cleanup
    if (tfp) {
        tfp->close();
        delete tfp;
        tfp = nullptr;
    }
    if (top) {
        top->final();
        delete top;
        top = nullptr;
    }

    fclose(logf);
    return (mism1 + mism2) == 0 ? 0 : 2;
}
