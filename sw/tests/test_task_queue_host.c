// sw/tests/test_task_queue_host.c
#define _POSIX_C_SOURCE 200809L

#include "../src/task_queue_mmio.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>

#ifndef MMIO_ENV_VAR
#define MMIO_ENV_VAR "MMIO_PATH"
#endif

// tiny helper for sleeping in microseconds using nanosleep
static void sleep_us(int microseconds) {
    struct timespec ts;
    ts.tv_sec  = microseconds / 1000000;
    ts.tv_nsec = (microseconds % 1000000) * 1000L;
    nanosleep(&ts, NULL);
}

static void ensure_logs_dir() {
    struct stat st;
    if (stat("logs", &st) != 0) {
        if (mkdir("logs", 0755) != 0) {
            perror("mkdir logs");
            exit(1);
        }
    }
}

// Try mmio_init on a candidate path and print diagnostic
static int try_mmio_path(const char *path) {
    if (!path) return -1;
    fprintf(stderr, "[MMIO] trying '%s' ... ", path);
    fflush(stderr);
    int rc = mmio_init(path);
    if (rc == 0) {
        fprintf(stderr, "OK\n");
        return 0;
    } else {
        fprintf(stderr, "FAILED (%s)\n", strerror(errno));
        return -1;
    }
}

// Build a list of candidate mmio paths and try them in order.
// Also accepts an explicit path via --mmio argument or env var MMIO_PATH.
static int init_mmio_flexible(int argc, char **argv) {
    // 1) Check command-line --mmio <path>
    for (int i = 1; i < argc - 1; ++i) {
        if ((strcmp(argv[i], "--mmio") == 0 || strcmp(argv[i], "-m") == 0)) {
            const char *p = argv[i+1];
            if (try_mmio_path(p) == 0) return 0;
        }
    }

    // 2) Check environment variable (MMIO_PATH)
    const char *envp = getenv(MMIO_ENV_VAR);
    if (envp && envp[0]) {
        if (try_mmio_path(envp) == 0) return 0;
    }

    // 3) Common candidate locations relative to likely CWDs:
    const char *candidates[] = {
        // when running from sw/ (recommended)
        "sw_hw/mmio_region.bin",
        "sw_hw/obj_dir/mmio_region.bin",

        // when running from project root and sw/ is a subdir
        "sw/sw_hw/mmio_region.bin",
        "sw/sw_hw/obj_dir/mmio_region.bin",

        // when running from sw/sw_hw/
        "mmio_region.bin",
        "obj_dir/mmio_region.bin",

        // when running from sw/sw_hw/obj_dir
        "../mmio_region.bin",
        "../obj_dir/mmio_region.bin",

        // when running from other common places
        "../sw_hw/mmio_region.bin",
        "../sw_hw/obj_dir/mmio_region.bin",
        "../../sw_hw/mmio_region.bin",
        NULL
    };

    for (int i = 0; candidates[i] != NULL; ++i) {
        if (try_mmio_path(candidates[i]) == 0) return 0;
    }

    // 4) Fallback: a few more locations
    const char *search_roots[] = { ".", "sw", "sw/sw_hw", "sw/sw_hw/obj_dir", "sw_hw", NULL };
    char buf[1024];
    for (int r = 0; search_roots[r] != NULL; ++r) {
        snprintf(buf, sizeof(buf), "%s/mmio_region.bin", search_roots[r]);
        if (try_mmio_path(buf) == 0) return 0;
    }

    return -1;
}

int main(int argc, char **argv) {
    if (init_mmio_flexible(argc, argv) != 0) {
        fprintf(stderr, "Could not open MMIO file; ensure sw/sw_hw/Vtb_task_queue is running.\n");
        fprintf(stderr, "Suggestions:\n");
        fprintf(stderr, " - Start the HW harness from sw/sw_hw like: (cd sw/sw_hw && ../obj_dir/Vtb_task_queue)\n");
        fprintf(stderr, " - Or call SW with exact path: ./test_task_queue_host --mmio sw/sw_hw/mmio_region.bin\n");
        fprintf(stderr, " - Or set environment: export MMIO_PATH=/abs/path/to/mmio_region.bin\n");
        return 1;
    }

    ensure_logs_dir();
    FILE *logf = fopen("logs/run.log", "w");
    if (!logf) { perror("open logs/run.log"); return 1; }
    mmio_write_log_header(logf);

    // NEW: open trace file for Python golden model
    FILE *tracef = fopen("logs/trace.csv", "w");
    if (!tracef) {
        perror("open logs/trace.csv");
        fclose(logf);
        return 1;
    }
    fprintf(tracef, "op,value\n");

    // Metrics
    unsigned long attempted_push = 0, success_push = 0, refused_push = 0;
    unsigned long attempted_pop = 0, success_pop = 0, refused_pop = 0;
    unsigned long mismatches = 0;

    // Deterministic small test
    fprintf(logf, "[SW] Deterministic test\n");
    uint32_t vals[] = {0xA5A5A5A5, 0xDEADBEEF, 0x01234567, 0x89ABCDEF};
    for (int i = 0; i < 4; i++) {
        attempted_push++;
        int r = mmio_push(vals[i], 1000);
        if (r == 0) {
            success_push++;
            fprintf(logf, "push OK 0x%08x\n", vals[i]);
            // log successful push
            fprintf(tracef, "push,0x%08x\n", vals[i]);
        } else if (r == -1) {
            refused_push++;
            fprintf(logf, "push REFUSED 0x%08x\n", vals[i]);
        } else {
            fprintf(logf, "push TIMEOUT 0x%08x\n", vals[i]);
        }
    }

    // pop two
    for (int i = 0; i < 2; i++) {
        attempted_pop++;
        uint32_t out;
        int r = mmio_pop(&out, 1000);
        if (r == 0) {
            success_pop++;
            fprintf(logf, "pop OK 0x%08x\n", out);
            // log successful pop
            fprintf(tracef, "pop,0x%08x\n", out);
        } else if (r == -1) {
            refused_pop++;
            fprintf(logf, "pop REFUSED\n");
        } else {
            fprintf(logf, "pop TIMEOUT\n");
        }
    }

    // Randomized test
    fprintf(logf, "[SW] Randomized test\n");
    srand((unsigned)time(NULL));
    const int OPS = 10000;

    // software golden queue (simple ring buffer, depth 16)
    uint32_t swbuf[16];
    int swhead = 0, swtail = 0, swcount = 0, swdepth = 16;

    for (int i = 0; i < OPS; i++) {
        int op = rand() % 3;
        if (op == 0) {
            uint32_t v = (uint32_t)rand();
            attempted_push++;
            int r = mmio_push(v, 100);
            if (r == 0) {
                success_push++;
                if (swcount < swdepth) {
                    swbuf[swtail] = v;
                    swtail = (swtail + 1) % swdepth;
                    swcount++;
                }
                // log successful push
                fprintf(tracef, "push,0x%08x\n", v);
            } else if (r == -1) {
                refused_push++;
            } else {
                fprintf(logf, "push timeout\n");
            }
        } else if (op == 1) {
            attempted_pop++;
            uint32_t out;
            int r = mmio_pop(&out, 100);
            if (r == 0) {
                success_pop++;
                if (swcount == 0) {
                    fprintf(logf, "MISMATCH: popped but SW empty\n");
                    mismatches++;
                } else {
                    uint32_t expected = swbuf[swhead];
                    swhead = (swhead + 1) % swdepth;
                    swcount--;
                    if (expected != out) {
                        fprintf(logf, "MISMATCH: expected 0x%08x got 0x%08x\n", expected, out);
                        mismatches++;
                    }
                }
                // log successful pop
                fprintf(tracef, "pop,0x%08x\n", out);
            } else if (r == -1) {
                refused_pop++;
            } else {
                fprintf(logf, "pop timeout\n");
            }
        } else {
            // idle - tiny pause to let HW advance
            sleep_us(10);
        }
    }

    // drain SW model
    while (swcount > 0) {
        attempted_pop++;
        uint32_t out;
        int r = mmio_pop(&out, 1000);
        if (r == 0) {
            success_pop++;
            uint32_t expected = swbuf[swhead];
            swhead = (swhead + 1) % swdepth;
            swcount--;
            if (expected != out) {
                fprintf(logf, "MISMATCH at drain: expected 0x%08x got 0x%08x\n", expected, out);
                mismatches++;
            }
            // log successful pop
            fprintf(tracef, "pop,0x%08x\n", out);
        } else {
            fprintf(logf, "Drain pop refused/timeout\n");
            break;
        }
    }

    // flush and close trace file
    fflush(tracef);
    fclose(tracef);

    // signal TB_DONE so the sw_hw simulator can exit (try multiple candidate paths)
    {
        const char *candidates[] = {
            "sw_hw/mmio_region.bin",
            "sw_hw/obj_dir/mmio_region.bin",
            "../sw_hw/mmio_region.bin",
            "../sw_hw/obj_dir/mmio_region.bin",
            "sw/sw_hw/mmio_region.bin",
            "sw/sw_hw/obj_dir/mmio_region.bin",
            "mmio_region.bin",
            "obj_dir/mmio_region.bin",
            NULL
        };
        for (int i = 0; candidates[i] != NULL; ++i) {
            int fd = open(candidates[i], O_RDWR);
            if (fd >= 0) {
                uint32_t one = 1;
                if (lseek(fd, 0x14, SEEK_SET) >= 0) {
                    ssize_t w = write(fd, &one, 4);
                    (void)w;
                }
                close(fd);
                break;
            }
        }
    }

    // write results.json into logs/
    FILE *resf = fopen("logs/results.json", "w");
    if (resf) {
        fprintf(resf, "{\n");
        fprintf(resf, "  \"attempted_pushes\": %lu,\n", attempted_push);
        fprintf(resf, "  \"successful_pushes\": %lu,\n", success_push);
        fprintf(resf, "  \"refused_pushes\": %lu,\n", refused_push);
        fprintf(resf, "  \"attempted_pops\": %lu,\n", attempted_pop);
        fprintf(resf, "  \"successful_pops\": %lu,\n", success_pop);
        fprintf(resf, "  \"refused_pops\": %lu,\n", refused_pop);
        fprintf(resf, "  \"mismatches\": %lu\n", mismatches);
        fprintf(resf, "}\n");
        fclose(resf);
    }

    FILE *csvf = fopen("logs/metrics.csv", "w");
    if (csvf) {
        fprintf(csvf, "attempted_pushes,successful_pushes,refused_pushes,attempted_pops,successful_pops,refused_pops,mismatches\n");
        fprintf(csvf, "%lu,%lu,%lu,%lu,%lu,%lu,%lu\n",
                attempted_push, success_push, refused_push,
                attempted_pop, success_pop, refused_pop, mismatches);
        fclose(csvf);
    }

    fprintf(logf, "=== Summary ===\n");
    fprintf(logf, "attempted pushes: %lu\nsuccessful pushes: %lu\nrefused pushes: %lu\n",
            attempted_push, success_push, refused_push);
    fprintf(logf, "attempted pops: %lu\nsuccessful pops: %lu\nrefused pops: %lu\n",
            attempted_pop, success_pop, refused_pop);
    fprintf(logf, "mismatches: %lu\n", mismatches);
    fclose(logf);

    mmio_close();
    return (mismatches == 0) ? 0 : 2;
}
