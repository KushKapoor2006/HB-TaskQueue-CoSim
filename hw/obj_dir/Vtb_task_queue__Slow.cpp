// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtb_task_queue.h for the primary calling header

#include "Vtb_task_queue.h"
#include "Vtb_task_queue__Syms.h"

//==========

VL_CTOR_IMP(Vtb_task_queue) {
    Vtb_task_queue__Syms* __restrict vlSymsp = __VlSymsp = new Vtb_task_queue__Syms(this, name());
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Reset internal values
    
    // Reset structure values
    _ctor_var_reset();
}

void Vtb_task_queue::__Vconfigure(Vtb_task_queue__Syms* vlSymsp, bool first) {
    if (false && first) {}  // Prevent unused
    this->__VlSymsp = vlSymsp;
    if (false && this->__VlSymsp) {}  // Prevent unused
    Verilated::timeunit(-9);
    Verilated::timeprecision(-12);
}

Vtb_task_queue::~Vtb_task_queue() {
    VL_DO_CLEAR(delete __VlSymsp, __VlSymsp = NULL);
}

void Vtb_task_queue::_initial__TOP__1(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_initial__TOP__1\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->tb_done = 0U;
}

void Vtb_task_queue::_settle__TOP__3(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_settle__TOP__3\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->tb_task_queue__DOT__push_req = ((IData)(vlTOPp->host_mode) 
                                            & (IData)(vlTOPp->host_push_req));
    if (vlTOPp->host_mode) {
        vlTOPp->tb_task_queue__DOT__data_in = vlTOPp->host_data_in;
        vlTOPp->tb_task_queue__DOT__pop_req = ((IData)(vlTOPp->host_pop_req) 
                                               & 1U);
    } else {
        vlTOPp->tb_task_queue__DOT__data_in = 0U;
        vlTOPp->tb_task_queue__DOT__pop_req = 0U;
    }
    vlTOPp->full = (0x10U == (IData)(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count));
    vlTOPp->valid_out = (0U != (IData)(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count));
    vlTOPp->data_out = vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem
        [vlTOPp->tb_task_queue__DOT__dut_queue__DOT__head];
}

void Vtb_task_queue::_initial__TOP__5(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_initial__TOP__5\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    if (VL_UNLIKELY((1U & VL_REDXOR_32(vlTOPp->data_out)))) {
        VL_WRITEF("\n");
    }
}

void Vtb_task_queue::_eval_initial(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_eval_initial\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->_initial__TOP__1(vlSymsp);
    vlTOPp->__Vclklast__TOP__clk = vlTOPp->clk;
    vlTOPp->_initial__TOP__5(vlSymsp);
}

void Vtb_task_queue::final() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::final\n"); );
    // Variables
    Vtb_task_queue__Syms* __restrict vlSymsp = this->__VlSymsp;
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
}

void Vtb_task_queue::_eval_settle(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_eval_settle\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    vlTOPp->_settle__TOP__3(vlSymsp);
    vlTOPp->__Vm_traceActivity[2U] = 1U;
    vlTOPp->__Vm_traceActivity[1U] = 1U;
    vlTOPp->__Vm_traceActivity[0U] = 1U;
}

void Vtb_task_queue::_ctor_var_reset() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_ctor_var_reset\n"); );
    // Body
    clk = VL_RAND_RESET_I(1);
    reset = VL_RAND_RESET_I(1);
    host_mode = VL_RAND_RESET_I(1);
    host_push_req = VL_RAND_RESET_I(1);
    host_data_in = VL_RAND_RESET_I(32);
    host_pop_req = VL_RAND_RESET_I(1);
    full = VL_RAND_RESET_I(1);
    valid_out = VL_RAND_RESET_I(1);
    data_out = VL_RAND_RESET_I(32);
    tb_done = VL_RAND_RESET_I(1);
    tb_task_queue__DOT__push_req = VL_RAND_RESET_I(1);
    tb_task_queue__DOT__data_in = VL_RAND_RESET_I(32);
    tb_task_queue__DOT__pop_req = VL_RAND_RESET_I(1);
    tb_task_queue__DOT__arb_grant_out = VL_RAND_RESET_I(2);
    tb_task_queue__DOT__arb_served_bank = VL_RAND_RESET_I(2);
    { int __Vi0=0; for (; __Vi0<16; ++__Vi0) {
            tb_task_queue__DOT__dut_queue__DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }}
    tb_task_queue__DOT__dut_queue__DOT__head = VL_RAND_RESET_I(4);
    tb_task_queue__DOT__dut_queue__DOT__tail = VL_RAND_RESET_I(4);
    tb_task_queue__DOT__dut_queue__DOT__count = VL_RAND_RESET_I(5);
    tb_task_queue__DOT__distributor__DOT__buffered_valid = VL_RAND_RESET_I(1);
    tb_task_queue__DOT__distributor__DOT__buffered_data = VL_RAND_RESET_I(32);
    tb_task_queue__DOT__distributor__DOT__in_data_reg = VL_RAND_RESET_I(32);
    tb_task_queue__DOT__arbiter__DOT__rr = VL_RAND_RESET_I(1);
    { int __Vi0=0; for (; __Vi0<3; ++__Vi0) {
            __Vm_traceActivity[__Vi0] = VL_RAND_RESET_I(1);
    }}
}
