// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Primary design header
//
// This header should be included by all source files instantiating the design.
// The class here is then constructed to instantiate the design.
// See the Verilator manual for examples.

#ifndef _VTB_TASK_QUEUE_H_
#define _VTB_TASK_QUEUE_H_  // guard

#include "verilated_heavy.h"

//==========

class Vtb_task_queue__Syms;
class Vtb_task_queue_VerilatedVcd;


//----------

VL_MODULE(Vtb_task_queue) {
  public:
    
    // PORTS
    // The application code writes and reads these signals to
    // propagate new values into/out from the Verilated model.
    VL_IN8(clk,0,0);
    VL_IN8(reset,0,0);
    VL_IN8(host_mode,0,0);
    VL_IN8(host_push_req,0,0);
    VL_IN8(host_pop_req,0,0);
    VL_OUT8(full,0,0);
    VL_OUT8(valid_out,0,0);
    VL_OUT8(tb_done,0,0);
    VL_IN(host_data_in,31,0);
    VL_OUT(data_out,31,0);
    
    // LOCAL SIGNALS
    // Internals; generally not touched by application code
    CData/*0:0*/ tb_task_queue__DOT__push_req;
    CData/*0:0*/ tb_task_queue__DOT__pop_req;
    CData/*1:0*/ tb_task_queue__DOT__arb_grant_out;
    CData/*1:0*/ tb_task_queue__DOT__arb_served_bank;
    CData/*3:0*/ tb_task_queue__DOT__dut_queue__DOT__head;
    CData/*3:0*/ tb_task_queue__DOT__dut_queue__DOT__tail;
    CData/*4:0*/ tb_task_queue__DOT__dut_queue__DOT__count;
    CData/*0:0*/ tb_task_queue__DOT__distributor__DOT__buffered_valid;
    CData/*0:0*/ tb_task_queue__DOT__arbiter__DOT__rr;
    IData/*31:0*/ tb_task_queue__DOT__data_in;
    IData/*31:0*/ tb_task_queue__DOT__distributor__DOT__buffered_data;
    IData/*31:0*/ tb_task_queue__DOT__distributor__DOT__in_data_reg;
    IData/*31:0*/ tb_task_queue__DOT__dut_queue__DOT__mem[16];
    
    // LOCAL VARIABLES
    // Internals; generally not touched by application code
    CData/*0:0*/ __Vclklast__TOP__clk;
    CData/*0:0*/ __Vm_traceActivity[3];
    
    // INTERNAL VARIABLES
    // Internals; generally not touched by application code
    Vtb_task_queue__Syms* __VlSymsp;  // Symbol table
    
    // CONSTRUCTORS
  private:
    VL_UNCOPYABLE(Vtb_task_queue);  ///< Copying not allowed
  public:
    /// Construct the model; called by application code
    /// The special name  may be used to make a wrapper with a
    /// single model invisible with respect to DPI scope names.
    Vtb_task_queue(const char* name = "TOP");
    /// Destroy the model; called (often implicitly) by application code
    ~Vtb_task_queue();
    /// Trace signals in the model; called by application code
    void trace(VerilatedVcdC* tfp, int levels, int options = 0);
    
    // API METHODS
    /// Evaluate the model.  Application must call when inputs change.
    void eval() { eval_step(); }
    /// Evaluate when calling multiple units/models per time step.
    void eval_step();
    /// Evaluate at end of a timestep for tracing, when using eval_step().
    /// Application must call after all eval() and before time changes.
    void eval_end_step() {}
    /// Simulation complete, run final blocks.  Application must call on completion.
    void final();
    
    // INTERNAL METHODS
  private:
    static void _eval_initial_loop(Vtb_task_queue__Syms* __restrict vlSymsp);
  public:
    void __Vconfigure(Vtb_task_queue__Syms* symsp, bool first);
  private:
    static QData _change_request(Vtb_task_queue__Syms* __restrict vlSymsp);
    static QData _change_request_1(Vtb_task_queue__Syms* __restrict vlSymsp);
  public:
    static void _combo__TOP__4(Vtb_task_queue__Syms* __restrict vlSymsp);
  private:
    void _ctor_var_reset() VL_ATTR_COLD;
  public:
    static void _eval(Vtb_task_queue__Syms* __restrict vlSymsp);
  private:
#ifdef VL_DEBUG
    void _eval_debug_assertions();
#endif  // VL_DEBUG
  public:
    static void _eval_initial(Vtb_task_queue__Syms* __restrict vlSymsp) VL_ATTR_COLD;
    static void _eval_settle(Vtb_task_queue__Syms* __restrict vlSymsp) VL_ATTR_COLD;
    static void _initial__TOP__1(Vtb_task_queue__Syms* __restrict vlSymsp) VL_ATTR_COLD;
    static void _initial__TOP__5(Vtb_task_queue__Syms* __restrict vlSymsp) VL_ATTR_COLD;
    static void _sequent__TOP__2(Vtb_task_queue__Syms* __restrict vlSymsp);
    static void _settle__TOP__3(Vtb_task_queue__Syms* __restrict vlSymsp) VL_ATTR_COLD;
  private:
    static void traceChgSub0(void* userp, VerilatedVcd* tracep);
    static void traceChgTop0(void* userp, VerilatedVcd* tracep);
    static void traceCleanup(void* userp, VerilatedVcd* /*unused*/);
    static void traceFullSub0(void* userp, VerilatedVcd* tracep) VL_ATTR_COLD;
    static void traceFullTop0(void* userp, VerilatedVcd* tracep) VL_ATTR_COLD;
    static void traceInitSub0(void* userp, VerilatedVcd* tracep) VL_ATTR_COLD;
    static void traceInitTop(void* userp, VerilatedVcd* tracep) VL_ATTR_COLD;
    void traceRegister(VerilatedVcd* tracep) VL_ATTR_COLD;
    static void traceInit(void* userp, VerilatedVcd* tracep, uint32_t code) VL_ATTR_COLD;
} VL_ATTR_ALIGNED(VL_CACHE_LINE_BYTES);

//----------


#endif  // guard
