TOP=tb_task_queue
VERILATOR=verilator
VERILATOR_FLAGS=--cc --exe --build -Wall -sv --trace -Mdir obj_dir --top-module $(TOP)

SRCS=testbenches/tb_task_queue.v \
     rtl/hb_task_queue_core.sv \
     rtl/hb_task_distributor.sv \
     rtl/hb_arbiter_banked.sv \
     verilator_main.cpp

TARGET=obj_dir/V$(TOP)

all: sim

sim:
	mkdir -p obj_dir
	mkdir -p outputs
	$(VERILATOR) $(VERILATOR_FLAGS) $(SRCS)

run: sim
	./$(TARGET)

clean:
	rm -rf obj_dir outputs

.PHONY: all sim run clean
