# hw/Makefile
TOP=tb_task_queue
VERILATOR=verilator
VERILATOR_FLAGS=--cc --exe --build -Wall -sv --trace -Mdir obj_dir --top-module $(TOP)
SRCS=testbenches/tb_task_queue.v \
     rtl/hb_task_queue_core.sv \
     rtl/hb_task_distributor.sv \
     rtl/hb_arbiter_banked.sv \
     verilator_main.cpp

all: sim

sim:
	mkdir -p obj_dir
	$(VERILATOR) $(VERILATOR_FLAGS) $(SRCS)

run: sim
	./obj_dir/Vtb_task_queue

clean:
	rm -rf obj_dir mmio_region.bin

.PHONY: all sim run clean
