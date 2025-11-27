// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Tracing implementation internals
#include "verilated_vcd_c.h"
#include "Vtb_task_queue__Syms.h"


void Vtb_task_queue::traceChgTop0(void* userp, VerilatedVcd* tracep) {
    Vtb_task_queue__Syms* __restrict vlSymsp = static_cast<Vtb_task_queue__Syms*>(userp);
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Variables
    if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;
    // Body
    {
        vlTOPp->traceChgSub0(userp, tracep);
    }
}

void Vtb_task_queue::traceChgSub0(void* userp, VerilatedVcd* tracep) {
    Vtb_task_queue__Syms* __restrict vlSymsp = static_cast<Vtb_task_queue__Syms*>(userp);
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    vluint32_t* const oldp = tracep->oldp(vlSymsp->__Vm_baseCode + 1);
    if (false && oldp) {}  // Prevent unused
    // Body
    {
        if (VL_UNLIKELY(vlTOPp->__Vm_traceActivity[1U])) {
            tracep->chgBit(oldp+0,(vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_valid));
            tracep->chgIData(oldp+1,(vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_data),32);
            tracep->chgCData(oldp+2,(vlTOPp->tb_task_queue__DOT__arb_grant_out),2);
            tracep->chgCData(oldp+3,(vlTOPp->tb_task_queue__DOT__arb_served_bank),2);
            tracep->chgIData(oldp+4,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[0]),32);
            tracep->chgIData(oldp+5,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[1]),32);
            tracep->chgIData(oldp+6,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[2]),32);
            tracep->chgIData(oldp+7,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[3]),32);
            tracep->chgIData(oldp+8,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[4]),32);
            tracep->chgIData(oldp+9,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[5]),32);
            tracep->chgIData(oldp+10,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[6]),32);
            tracep->chgIData(oldp+11,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[7]),32);
            tracep->chgIData(oldp+12,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[8]),32);
            tracep->chgIData(oldp+13,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[9]),32);
            tracep->chgIData(oldp+14,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[10]),32);
            tracep->chgIData(oldp+15,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[11]),32);
            tracep->chgIData(oldp+16,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[12]),32);
            tracep->chgIData(oldp+17,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[13]),32);
            tracep->chgIData(oldp+18,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[14]),32);
            tracep->chgIData(oldp+19,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[15]),32);
            tracep->chgCData(oldp+20,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__head),4);
            tracep->chgCData(oldp+21,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__tail),4);
            tracep->chgCData(oldp+22,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count),5);
            tracep->chgIData(oldp+23,(vlTOPp->tb_task_queue__DOT__distributor__DOT__in_data_reg),32);
            tracep->chgBit(oldp+24,(vlTOPp->tb_task_queue__DOT__arbiter__DOT__rr));
        }
        if (VL_UNLIKELY(vlTOPp->__Vm_traceActivity[2U])) {
            tracep->chgBit(oldp+25,(vlTOPp->tb_task_queue__DOT__push_req));
            tracep->chgIData(oldp+26,(vlTOPp->tb_task_queue__DOT__data_in),32);
            tracep->chgBit(oldp+27,(vlTOPp->tb_task_queue__DOT__pop_req));
        }
        tracep->chgBit(oldp+28,(vlTOPp->clk));
        tracep->chgBit(oldp+29,(vlTOPp->reset));
        tracep->chgBit(oldp+30,(vlTOPp->host_mode));
        tracep->chgBit(oldp+31,(vlTOPp->host_push_req));
        tracep->chgIData(oldp+32,(vlTOPp->host_data_in),32);
        tracep->chgBit(oldp+33,(vlTOPp->host_pop_req));
        tracep->chgBit(oldp+34,(vlTOPp->full));
        tracep->chgBit(oldp+35,(vlTOPp->valid_out));
        tracep->chgIData(oldp+36,(vlTOPp->data_out),32);
        tracep->chgBit(oldp+37,(vlTOPp->tb_done));
        tracep->chgBit(oldp+38,((1U & VL_REDXOR_32(vlTOPp->data_out))));
    }
}

void Vtb_task_queue::traceCleanup(void* userp, VerilatedVcd* /*unused*/) {
    Vtb_task_queue__Syms* __restrict vlSymsp = static_cast<Vtb_task_queue__Syms*>(userp);
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    {
        vlSymsp->__Vm_activity = false;
        vlTOPp->__Vm_traceActivity[0U] = 0U;
        vlTOPp->__Vm_traceActivity[1U] = 0U;
        vlTOPp->__Vm_traceActivity[2U] = 0U;
    }
}
