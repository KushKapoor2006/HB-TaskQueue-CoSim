// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtb_task_queue.h for the primary calling header

#include "Vtb_task_queue.h"
#include "Vtb_task_queue__Syms.h"

//==========

void Vtb_task_queue::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vtb_task_queue::eval\n"); );
    Vtb_task_queue__Syms* __restrict vlSymsp = this->__VlSymsp;  // Setup global symbol table
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
#ifdef VL_DEBUG
    // Debug assertions
    _eval_debug_assertions();
#endif  // VL_DEBUG
    // Initialize
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) _eval_initial_loop(vlSymsp);
    // Evaluate till stable
    int __VclockLoop = 0;
    QData __Vchange = 1;
    do {
        VL_DEBUG_IF(VL_DBG_MSGF("+ Clock loop\n"););
        vlSymsp->__Vm_activity = true;
        _eval(vlSymsp);
        if (VL_UNLIKELY(++__VclockLoop > 100)) {
            // About to fail, so enable debug to see what's not settling.
            // Note you must run make with OPT=-DVL_DEBUG for debug prints.
            int __Vsaved_debug = Verilated::debug();
            Verilated::debug(1);
            __Vchange = _change_request(vlSymsp);
            Verilated::debug(__Vsaved_debug);
            VL_FATAL_MT("testbenches/tb_task_queue.v", 4, "",
                "Verilated model didn't converge\n"
                "- See DIDNOTCONVERGE in the Verilator manual");
        } else {
            __Vchange = _change_request(vlSymsp);
        }
    } while (VL_UNLIKELY(__Vchange));
}

void Vtb_task_queue::_eval_initial_loop(Vtb_task_queue__Syms* __restrict vlSymsp) {
    vlSymsp->__Vm_didInit = true;
    _eval_initial(vlSymsp);
    vlSymsp->__Vm_activity = true;
    // Evaluate till stable
    int __VclockLoop = 0;
    QData __Vchange = 1;
    do {
        _eval_settle(vlSymsp);
        _eval(vlSymsp);
        if (VL_UNLIKELY(++__VclockLoop > 100)) {
            // About to fail, so enable debug to see what's not settling.
            // Note you must run make with OPT=-DVL_DEBUG for debug prints.
            int __Vsaved_debug = Verilated::debug();
            Verilated::debug(1);
            __Vchange = _change_request(vlSymsp);
            Verilated::debug(__Vsaved_debug);
            VL_FATAL_MT("testbenches/tb_task_queue.v", 4, "",
                "Verilated model didn't DC converge\n"
                "- See DIDNOTCONVERGE in the Verilator manual");
        } else {
            __Vchange = _change_request(vlSymsp);
        }
    } while (VL_UNLIKELY(__Vchange));
}

VL_INLINE_OPT void Vtb_task_queue::_sequent__TOP__2(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_sequent__TOP__2\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Variables
    CData/*3:0*/ __Vdly__tb_task_queue__DOT__dut_queue__DOT__head;
    CData/*4:0*/ __Vdly__tb_task_queue__DOT__dut_queue__DOT__count;
    CData/*3:0*/ __Vdly__tb_task_queue__DOT__dut_queue__DOT__tail;
    CData/*3:0*/ __Vdlyvdim0__tb_task_queue__DOT__dut_queue__DOT__mem__v0;
    CData/*0:0*/ __Vdlyvset__tb_task_queue__DOT__dut_queue__DOT__mem__v0;
    CData/*0:0*/ __Vdly__tb_task_queue__DOT__distributor__DOT__buffered_valid;
    CData/*0:0*/ __Vdly__tb_task_queue__DOT__arbiter__DOT__rr;
    IData/*31:0*/ __Vdlyvval__tb_task_queue__DOT__dut_queue__DOT__mem__v0;
    // Body
    __Vdly__tb_task_queue__DOT__arbiter__DOT__rr = vlTOPp->tb_task_queue__DOT__arbiter__DOT__rr;
    __Vdly__tb_task_queue__DOT__distributor__DOT__buffered_valid 
        = vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_valid;
    __Vdly__tb_task_queue__DOT__dut_queue__DOT__tail 
        = vlTOPp->tb_task_queue__DOT__dut_queue__DOT__tail;
    __Vdlyvset__tb_task_queue__DOT__dut_queue__DOT__mem__v0 = 0U;
    __Vdly__tb_task_queue__DOT__dut_queue__DOT__head 
        = vlTOPp->tb_task_queue__DOT__dut_queue__DOT__head;
    __Vdly__tb_task_queue__DOT__dut_queue__DOT__count 
        = vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count;
    if (vlTOPp->reset) {
        vlTOPp->tb_task_queue__DOT__arb_grant_out = 0U;
        vlTOPp->tb_task_queue__DOT__arb_served_bank = 0U;
        __Vdly__tb_task_queue__DOT__arbiter__DOT__rr = 0U;
    } else {
        vlTOPp->tb_task_queue__DOT__arb_grant_out = 0U;
        vlTOPp->tb_task_queue__DOT__arb_served_bank = 0U;
        if (vlTOPp->valid_out) {
            if (vlTOPp->tb_task_queue__DOT__arbiter__DOT__rr) {
                vlTOPp->tb_task_queue__DOT__arb_grant_out = 2U;
                vlTOPp->tb_task_queue__DOT__arb_served_bank = 2U;
                __Vdly__tb_task_queue__DOT__arbiter__DOT__rr = 0U;
            } else {
                vlTOPp->tb_task_queue__DOT__arb_grant_out = 1U;
                vlTOPp->tb_task_queue__DOT__arb_served_bank = 1U;
                __Vdly__tb_task_queue__DOT__arbiter__DOT__rr = 1U;
            }
        }
    }
    if (vlTOPp->reset) {
        __Vdly__tb_task_queue__DOT__distributor__DOT__buffered_valid = 0U;
        vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_data = 0U;
    } else {
        if (vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_valid) {
            __Vdly__tb_task_queue__DOT__distributor__DOT__buffered_valid = 0U;
        }
        if (((IData)(vlTOPp->valid_out) & (~ (IData)(vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_valid)))) {
            vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_data 
                = vlTOPp->tb_task_queue__DOT__distributor__DOT__in_data_reg;
            __Vdly__tb_task_queue__DOT__distributor__DOT__buffered_valid = 1U;
        }
    }
    if (vlTOPp->reset) {
        __Vdly__tb_task_queue__DOT__dut_queue__DOT__tail = 0U;
    } else {
        if (((IData)(vlTOPp->tb_task_queue__DOT__push_req) 
             & (~ (IData)(vlTOPp->full)))) {
            __Vdlyvval__tb_task_queue__DOT__dut_queue__DOT__mem__v0 
                = vlTOPp->tb_task_queue__DOT__data_in;
            __Vdlyvset__tb_task_queue__DOT__dut_queue__DOT__mem__v0 = 1U;
            __Vdlyvdim0__tb_task_queue__DOT__dut_queue__DOT__mem__v0 
                = vlTOPp->tb_task_queue__DOT__dut_queue__DOT__tail;
            __Vdly__tb_task_queue__DOT__dut_queue__DOT__tail 
                = (0xfU & ((IData)(1U) + (IData)(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__tail)));
        }
    }
    if (vlTOPp->reset) {
        __Vdly__tb_task_queue__DOT__dut_queue__DOT__head = 0U;
        __Vdly__tb_task_queue__DOT__dut_queue__DOT__count = 0U;
    } else {
        if (((IData)(vlTOPp->tb_task_queue__DOT__push_req) 
             & (~ (IData)(vlTOPp->full)))) {
            __Vdly__tb_task_queue__DOT__dut_queue__DOT__count 
                = (0x1fU & ((IData)(1U) + (IData)(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count)));
        }
        if (((IData)(vlTOPp->tb_task_queue__DOT__pop_req) 
             & (0U != (IData)(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count)))) {
            __Vdly__tb_task_queue__DOT__dut_queue__DOT__head 
                = (0xfU & ((IData)(1U) + (IData)(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__head)));
            __Vdly__tb_task_queue__DOT__dut_queue__DOT__count 
                = (0x1fU & ((IData)(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count) 
                            - (IData)(1U)));
        }
    }
    vlTOPp->tb_task_queue__DOT__arbiter__DOT__rr = __Vdly__tb_task_queue__DOT__arbiter__DOT__rr;
    vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_valid 
        = __Vdly__tb_task_queue__DOT__distributor__DOT__buffered_valid;
    vlTOPp->tb_task_queue__DOT__dut_queue__DOT__tail 
        = __Vdly__tb_task_queue__DOT__dut_queue__DOT__tail;
    if (__Vdlyvset__tb_task_queue__DOT__dut_queue__DOT__mem__v0) {
        vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[__Vdlyvdim0__tb_task_queue__DOT__dut_queue__DOT__mem__v0] 
            = __Vdlyvval__tb_task_queue__DOT__dut_queue__DOT__mem__v0;
    }
    vlTOPp->tb_task_queue__DOT__dut_queue__DOT__head 
        = __Vdly__tb_task_queue__DOT__dut_queue__DOT__head;
    vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count 
        = __Vdly__tb_task_queue__DOT__dut_queue__DOT__count;
    vlTOPp->tb_task_queue__DOT__distributor__DOT__in_data_reg 
        = ((IData)(vlTOPp->reset) ? 0U : vlTOPp->data_out);
    vlTOPp->full = (0x10U == (IData)(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count));
    vlTOPp->valid_out = (0U != (IData)(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count));
    vlTOPp->data_out = vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem
        [vlTOPp->tb_task_queue__DOT__dut_queue__DOT__head];
}

VL_INLINE_OPT void Vtb_task_queue::_combo__TOP__4(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_combo__TOP__4\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    if (vlTOPp->host_mode) {
        vlTOPp->tb_task_queue__DOT__data_in = vlTOPp->host_data_in;
        vlTOPp->tb_task_queue__DOT__pop_req = ((IData)(vlTOPp->host_pop_req) 
                                               & 1U);
        vlTOPp->tb_task_queue__DOT__push_req = ((IData)(vlTOPp->host_push_req) 
                                                & 1U);
    } else {
        vlTOPp->tb_task_queue__DOT__data_in = 0U;
        vlTOPp->tb_task_queue__DOT__pop_req = 0U;
        vlTOPp->tb_task_queue__DOT__push_req = 0U;
    }
}

void Vtb_task_queue::_eval(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_eval\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    if (((IData)(vlTOPp->clk) & (~ (IData)(vlTOPp->__Vclklast__TOP__clk)))) {
        vlTOPp->_sequent__TOP__2(vlSymsp);
        vlTOPp->__Vm_traceActivity[1U] = 1U;
    }
    vlTOPp->_combo__TOP__4(vlSymsp);
    vlTOPp->__Vm_traceActivity[2U] = 1U;
    // Final
    vlTOPp->__Vclklast__TOP__clk = vlTOPp->clk;
}

VL_INLINE_OPT QData Vtb_task_queue::_change_request(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_change_request\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    return (vlTOPp->_change_request_1(vlSymsp));
}

VL_INLINE_OPT QData Vtb_task_queue::_change_request_1(Vtb_task_queue__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_change_request_1\n"); );
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    // Change detection
    QData __req = false;  // Logically a bool
    return __req;
}

#ifdef VL_DEBUG
void Vtb_task_queue::_eval_debug_assertions() {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_task_queue::_eval_debug_assertions\n"); );
    // Body
    if (VL_UNLIKELY((clk & 0xfeU))) {
        Verilated::overWidthError("clk");}
    if (VL_UNLIKELY((reset & 0xfeU))) {
        Verilated::overWidthError("reset");}
    if (VL_UNLIKELY((host_mode & 0xfeU))) {
        Verilated::overWidthError("host_mode");}
    if (VL_UNLIKELY((host_push_req & 0xfeU))) {
        Verilated::overWidthError("host_push_req");}
    if (VL_UNLIKELY((host_pop_req & 0xfeU))) {
        Verilated::overWidthError("host_pop_req");}
}
#endif  // VL_DEBUG
