// sw/src/task_queue_mmio.h
#ifndef TASK_QUEUE_MMIO_H
#define TASK_QUEUE_MMIO_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>   // for FILE

// Default MMIO file path (relative to the sw/ directory)
// We talk to the copied HW model in sw/sw_hw/
#ifndef MMIO_DEFAULT_PATH
#define MMIO_DEFAULT_PATH "sw_hw/mmio_region.bin"
#endif

// API
int mmio_init(const char *path);   // map mmio file (path may be NULL to use default)
void mmio_close(void);

int mmio_push(uint32_t value, int timeout_ms); // 0=success, -1=refused, -2=timeout
int mmio_pop(uint32_t *out, int timeout_ms);   // 0=success, -1=refused, -2=timeout

bool mmio_is_full(void);
bool mmio_is_valid(void);

// Logging helper
void mmio_write_log_header(FILE *f);

#endif // TASK_QUEUE_MMIO_H
