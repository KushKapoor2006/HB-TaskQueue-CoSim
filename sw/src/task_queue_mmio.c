// sw/src/task_queue_mmio.c
#define _POSIX_C_SOURCE 200809L

#include "task_queue_mmio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>

static volatile uint8_t *mmio = NULL;
static size_t mmio_size = 4096;
static char mmio_path_used[512];

// MMIO offsets (must match verilator_main.cpp in sw/sw_hw/)
enum {
    OFF_CTRL     = 0x00,   // ctrl: push_req (1), pop_req (2)
    OFF_DATA_IN  = 0x04,
    OFF_ACK      = 0x08,   // ack: PUSH_OK(1), PUSH_REFUSED(2), POP_OK(4), POP_REFUSED(8)
    OFF_STATUS   = 0x0C,   // status: FULL(1), VALID(2)
    OFF_DATA_OUT = 0x10,
    OFF_TB_DONE  = 0x14
};

static inline uint32_t read32(size_t off) {
    uint32_t v;
    const void *src = (const void *)((const uint8_t *)mmio + off);
    memcpy(&v, src, sizeof(v));
    return v;
}

static inline void write32(size_t off, uint32_t v) {
    void *dst = (void *)((uint8_t *)mmio + off);
    memcpy(dst, &v, sizeof(v));
}

// small sleep in milliseconds using nanosleep
static void sleep_ms(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

int mmio_init(const char *path) {
    const char *p = path ? path : MMIO_DEFAULT_PATH;

    int fd = open(p, O_RDWR);
    if (fd < 0) {
        perror("mmio_init: open");
        return -1;
    }
    mmio = mmap(NULL, mmio_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (mmio == MAP_FAILED) {
        perror("mmio_init: mmap");
        mmio = NULL;
        return -1;
    }

    strncpy(mmio_path_used, p, sizeof(mmio_path_used) - 1);
    mmio_path_used[sizeof(mmio_path_used) - 1] = '\0';
    return 0;
}

void mmio_close(void) {
    if (mmio) {
        munmap((void *)mmio, mmio_size);
        mmio = NULL;
    }
}

// return 0 success, -1 refused, -2 timeout
int mmio_push(uint32_t value, int timeout_ms) {
    if (!mmio) return -2;

    // write data then set push bit
    uint32_t ctrl = read32(OFF_CTRL);
    write32(OFF_DATA_IN, value);
    write32(OFF_CTRL, ctrl | 0x1);

    int waited = 0;
    const int step_ms = 1;

    while (waited < timeout_ms) {
        uint32_t ack = read32(OFF_ACK);
        if (ack & 0x1) {   // PUSH_OK
            write32(OFF_ACK, ack & ~0x1);
            return 0;
        }
        if (ack & 0x2) {   // PUSH_REFUSED
            write32(OFF_ACK, ack & ~0x2);
            return -1;
        }
        sleep_ms(step_ms);
        waited += step_ms;
    }
    return -2;
}

// return 0 success, -1 refused, -2 timeout
int mmio_pop(uint32_t *out, int timeout_ms) {
    if (!mmio) return -2;

    uint32_t ctrl = read32(OFF_CTRL);
    write32(OFF_CTRL, ctrl | 0x2); // set pop_req

    int waited = 0;
    const int step_ms = 1;

    while (waited < timeout_ms) {
        uint32_t ack = read32(OFF_ACK);
        if (ack & 0x4) { // POP_OK
            uint32_t data = read32(OFF_DATA_OUT);
            if (out) *out = data;
            write32(OFF_ACK, ack & ~0x4);
            return 0;
        }
        if (ack & 0x8) { // POP_REFUSED
            write32(OFF_ACK, ack & ~0x8);
            return -1;
        }
        sleep_ms(step_ms);
        waited += step_ms;
    }
    return -2;
}

bool mmio_is_full(void) {
    if (!mmio) return true;
    uint32_t st = read32(OFF_STATUS);
    return (st & 0x1) != 0;
}

bool mmio_is_valid(void) {
    if (!mmio) return false;
    uint32_t st = read32(OFF_STATUS);
    return (st & 0x2) != 0;
}

void mmio_write_log_header(FILE *f) {
    time_t now = time(NULL);
    char buf[64];
    struct tm tm;
    gmtime_r(&now, &tm);
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", &tm);
    fprintf(f, "Run at: %s\nMMIO path: %s\n\n", buf,
            mmio_path_used[0] ? mmio_path_used : MMIO_DEFAULT_PATH);
}
