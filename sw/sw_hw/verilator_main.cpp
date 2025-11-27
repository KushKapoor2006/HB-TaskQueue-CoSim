// hw/verilator_main.cpp
// Verilator harness that provides a simple MMIO region via a memory-mapped file.
// MMIO file path: "mmio_region.bin" in the hw/ directory. SW should mmap the same file.
//
// MMIO layout (offsets in bytes):
// 0x00 CTRL       : bits: PUSH_REQ(0x1), POP_REQ(0x2)
// 0x04 DATA_IN    : uint32_t (value to push)
// 0x08 ACK        : bits: PUSH_OK(0x1), PUSH_REFUSED(0x2), POP_OK(0x4), POP_REFUSED(0x8)
// 0x0C STATUS     : bits: FULL(0x1), VALID(0x2)
// 0x10 DATA_OUT   : uint32_t (value popped)
// 0x14 TB_DONE    : uint32_t (software can set to 1 to ask sim to stop)
// file size: 4096 bytes

#include "Vtb_task_queue.h"
#include "verilated.h"
#include "verilated_vcd_c.h"

#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>

using namespace std;

static Vtb_task_queue *top = nullptr;
static VerilatedVcdC *tfp = nullptr;
static uint64_t tick_count = 0;

// MMIO definitions
const char *MMIO_FILE = "mmio_region.bin";
const size_t MMIO_SIZE = 4096;

inline uint32_t mmio_read32(volatile uint8_t *base, size_t off) {
    uint32_t v;
    const void *src = (const void *)((const uint8_t *)base + off);
    memcpy(&v, src, sizeof(v));
    return v;
}

inline void mmio_write32(volatile uint8_t *base, size_t off, uint32_t val) {
    void *dst = (void *)((uint8_t *)const_cast<volatile uint8_t *>(base) + off);
    memcpy(dst, &val, sizeof(val));
    // no need for clear_cache here; this is just an MMIO data file
}

// tick helper: one full clock (falling + rising) with VCD dump
static void tick() {
    // falling edge
    top->clk = 0;
    top->eval();
    if (tfp) tfp->dump(tick_count++);
    // rising edge
    top->clk = 1;
    top->eval();
    if (tfp) tfp->dump(tick_count++);
}

// create or open mmio file and mmap it, return pointer
volatile uint8_t *mmio_map_or_die(const char *path) {
    int fd = open(path, O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        perror("open mmio file");
        exit(1);
    }
    if (ftruncate(fd, MMIO_SIZE) != 0) {
        perror("ftruncate mmio file");
        exit(1);
    }
    void *ptr = mmap(NULL, MMIO_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    close(fd);
    return (volatile uint8_t *)ptr;
}

int main(int argc, char **argv) {
    Verilated::commandArgs(argc, argv);

    // map mmio file (creates file if missing)
    volatile uint8_t *mmio = mmio_map_or_die(MMIO_FILE);

    // zero region
    memset((void*)mmio, 0, MMIO_SIZE);

    // build model & tracing
    top = new Vtb_task_queue;
    Verilated::traceEverOn(true);
    tfp = new VerilatedVcdC;
    top->trace(tfp, 99);
    tfp->open("sim.vcd"); // writes to hw/sim.vcd

    // initial signals
    top->clk = 0;
    top->reset = 1;
    top->host_mode = 1;
    top->host_push_req = 0;
    top->host_pop_req = 0;
    top->host_data_in = 0;
    top->tb_done = 0;
    top->eval();
    tfp->dump(tick_count++);

    // Deassert reset after a few cycles
    for (int i=0;i<4;i++) tick();
    top->reset = 0;
    tick();

    // Main loop: poll MMIO for requests until tb_done is set by SW or until ctrl-c
    cout << "[hw] MMIO bridge running. MMIO file: " << MMIO_FILE << endl;
    while (true) {
        uint32_t ctrl = mmio_read32(mmio, 0x00);
        uint32_t data_in = mmio_read32(mmio, 0x04);
        uint32_t ack = mmio_read32(mmio, 0x08);
        uint32_t status = mmio_read32(mmio, 0x0C);
        uint32_t tb_done = mmio_read32(mmio, 0x14);

        // If software asked to stop, break
        if (tb_done != 0) {
            cout << "[hw] tb_done observed from MMIO, exiting." << endl;
            break;
        }

        bool did_something = false;

        // Handle push request
        if (ctrl & 0x1) {
            // check current full status
            // sample DUT signals
            // tick once without asserting host push to get up-to-date status
            // (but take immediate status as of now)
            bool is_full = (top->full != 0);
            if (is_full) {
                // refuse push
                uint32_t new_ack = mmio_read32(mmio, 0x08);
                new_ack |= 0x2; // PUSH_REFUSED
                mmio_write32(mmio, 0x08, new_ack);
                // clear ctrl
                msync((void*)mmio, MMIO_SIZE, MS_SYNC);
                mmio_write32(mmio, 0x00, ctrl & ~0x1);
                did_something = true;
            } else {
                // perform push by pulsing host_push_req for one cycle
                top->host_data_in = data_in;
                top->host_push_req = 1;
                tick(); // rising edge executes push
                top->host_push_req = 0;
                // set ACK push_ok
                uint32_t new_ack = mmio_read32(mmio, 0x08);
                new_ack |= 0x1; // PUSH_OK
                mmio_write32(mmio, 0x08, new_ack);
                // clear ctrl
                mmio_write32(mmio, 0x00, ctrl & ~0x1);
                did_something = true;
            }
        }

        // Handle pop request
        if (ctrl & 0x2) {
            // check current valid
            bool is_valid = (top->valid_out != 0);
            if (!is_valid) {
                // refuse pop
                uint32_t new_ack = mmio_read32(mmio, 0x08);
                new_ack |= 0x8; // POP_REFUSED
                mmio_write32(mmio, 0x08, new_ack);
                // clear ctrl
                mmio_write32(mmio, 0x00, ctrl & ~0x2);
                did_something = true;
            } else {
                // pulse pop_req for a cycle and capture the popped data
                top->host_pop_req = 1;
                tick(); // rising edge triggers pop
                // read data_out sample
                uint32_t popped = (uint32_t) (top->data_out & 0xFFFFFFFF);
                // write DATA_OUT
                mmio_write32(mmio, 0x10, popped);
                // set ack pop_ok
                uint32_t new_ack = mmio_read32(mmio, 0x08);
                new_ack |= 0x4; // POP_OK
                mmio_write32(mmio, 0x08, new_ack);
                // clear ctrl
                mmio_write32(mmio, 0x00, ctrl & ~0x2);
                top->host_pop_req = 0;
                did_something = true;
            }
        }

        // if we didn't do push/pop, advance one idle cycle to keep simulation moving
        if (!did_something) {
            tick();
        }

        // update status register (full / valid)
        uint32_t status_bits = 0;
        if (top->full) status_bits |= 0x1;
        if (top->valid_out) status_bits |= 0x2;
        mmio_write32(mmio, 0x0C, status_bits);

        // small msync to flush mmio to file (helps other process see updates)
        msync((void*)mmio, MMIO_SIZE, MS_SYNC);

        // small sleep to avoid busy-wait burning CPU if desired (commented out for max speed)
        // usleep(10);
    }

    // finalization
    top->final();
    if (tfp) {
        tfp->close();
        delete tfp;
        tfp = nullptr;
    }
    delete top;
    return 0;
}
