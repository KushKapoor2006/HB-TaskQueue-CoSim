# sw/Makefile
CC = cc
CFLAGS = -O2 -I./include
LDFLAGS =

all: test_host

test_host: tests/test_task_queue_host.c src/task_queue_mmio.c
	$(CC) $(CFLAGS) -o test_task_queue_host tests/test_task_queue_host.c src/task_queue_mmio.c $(LDFLAGS)

run: test_host
	./test_task_queue_host

clean:
	rm -f test_task_queue_host
	rm -rf logs

.PHONY: all test_host run clean
