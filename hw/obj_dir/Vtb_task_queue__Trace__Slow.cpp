// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Tracing implementation internals
#include "verilated_vcd_c.h"
#include "Vtb_task_queue__Syms.h"


//======================

void Vtb_task_queue::trace(VerilatedVcdC* tfp, int, int) {
    tfp->spTrace()->addInitCb(&traceInit, __VlSymsp);
    traceRegister(tfp->spTrace());
}

void Vtb_task_queue::traceInit(void* userp, VerilatedVcd* tracep, uint32_t code) {
    // Callback from tracep->open()
    Vtb_task_queue__Syms* __restrict vlSymsp = static_cast<Vtb_task_queue__Syms*>(userp);
    if (!Verilated::calcUnusedSigs()) {
        VL_FATAL_MT(__FILE__, __LINE__, __FILE__,
                        "Turning on wave traces requires Verilated::traceEverOn(true) call before time 0.");
    }
    vlSymsp->__Vm_baseCode = code;
    tracep->module(vlSymsp->name());
    tracep->scopeEscape(' ');
    Vtb_task_queue::traceInitTop(vlSymsp, tracep);
    tracep->scopeEscape('.');
}

//======================


void Vtb_task_queue::traceInitTop(void* userp, VerilatedVcd* tracep) {
    Vtb_task_queue__Syms* __restrict vlSymsp = static_cast<Vtb_task_queue__Syms*>(userp);
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    {
        vlTOPp->traceInitSub0(userp, tracep);
    }
}

void Vtb_task_queue::traceInitSub0(void* userp, VerilatedVcd* tracep) {
    Vtb_task_queue__Syms* __restrict vlSymsp = static_cast<Vtb_task_queue__Syms*>(userp);
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    const int c = vlSymsp->__Vm_baseCode;
    if (false && tracep && c) {}  // Prevent unused
    // Body
    {
        tracep->declBit(c+29,"clk", false,-1);
        tracep->declBit(c+30,"reset", false,-1);
        tracep->declBit(c+31,"host_mode", false,-1);
        tracep->declBit(c+32,"host_push_req", false,-1);
        tracep->declBus(c+33,"host_data_in", false,-1, 31,0);
        tracep->declBit(c+34,"host_pop_req", false,-1);
        tracep->declBit(c+35,"full", false,-1);
        tracep->declBit(c+36,"valid_out", false,-1);
        tracep->declBus(c+37,"data_out", false,-1, 31,0);
        tracep->declBit(c+38,"tb_done", false,-1);
        tracep->declBit(c+29,"tb_task_queue clk", false,-1);
        tracep->declBit(c+30,"tb_task_queue reset", false,-1);
        tracep->declBit(c+31,"tb_task_queue host_mode", false,-1);
        tracep->declBit(c+32,"tb_task_queue host_push_req", false,-1);
        tracep->declBus(c+33,"tb_task_queue host_data_in", false,-1, 31,0);
        tracep->declBit(c+34,"tb_task_queue host_pop_req", false,-1);
        tracep->declBit(c+35,"tb_task_queue full", false,-1);
        tracep->declBit(c+36,"tb_task_queue valid_out", false,-1);
        tracep->declBus(c+37,"tb_task_queue data_out", false,-1, 31,0);
        tracep->declBit(c+38,"tb_task_queue tb_done", false,-1);
        tracep->declBit(c+26,"tb_task_queue push_req", false,-1);
        tracep->declBus(c+27,"tb_task_queue data_in", false,-1, 31,0);
        tracep->declBit(c+28,"tb_task_queue pop_req", false,-1);
        tracep->declBit(c+1,"tb_task_queue dist_out_valid", false,-1);
        tracep->declBus(c+2,"tb_task_queue dist_out_data", false,-1, 31,0);
        tracep->declBus(c+3,"tb_task_queue arb_grant_out", false,-1, 1,0);
        tracep->declBus(c+4,"tb_task_queue arb_served_bank", false,-1, 1,0);
        tracep->declBus(c+40,"tb_task_queue dut_queue DEPTH", false,-1, 31,0);
        tracep->declBus(c+41,"tb_task_queue dut_queue WIDTH", false,-1, 31,0);
        tracep->declBit(c+29,"tb_task_queue dut_queue clk", false,-1);
        tracep->declBit(c+30,"tb_task_queue dut_queue reset", false,-1);
        tracep->declBit(c+26,"tb_task_queue dut_queue push_req", false,-1);
        tracep->declBus(c+27,"tb_task_queue dut_queue data_in", false,-1, 31,0);
        tracep->declBit(c+35,"tb_task_queue dut_queue full", false,-1);
        tracep->declBit(c+36,"tb_task_queue dut_queue valid_out", false,-1);
        tracep->declBus(c+37,"tb_task_queue dut_queue data_out", false,-1, 31,0);
        tracep->declBit(c+28,"tb_task_queue dut_queue pop_req", false,-1);
        tracep->declBus(c+42,"tb_task_queue dut_queue PTR_W", false,-1, 31,0);
        {int i; for (i=0; i<16; i++) {
                tracep->declBus(c+5+i*1,"tb_task_queue dut_queue mem", true,(i+0), 31,0);}}
        tracep->declBus(c+21,"tb_task_queue dut_queue head", false,-1, 3,0);
        tracep->declBus(c+22,"tb_task_queue dut_queue tail", false,-1, 3,0);
        tracep->declBus(c+23,"tb_task_queue dut_queue count", false,-1, 4,0);
        tracep->declBit(c+29,"tb_task_queue distributor clk", false,-1);
        tracep->declBit(c+30,"tb_task_queue distributor reset", false,-1);
        tracep->declBit(c+36,"tb_task_queue distributor in_valid", false,-1);
        tracep->declBus(c+37,"tb_task_queue distributor in_data", false,-1, 31,0);
        tracep->declBit(c+1,"tb_task_queue distributor out_valid", false,-1);
        tracep->declBus(c+2,"tb_task_queue distributor out_data", false,-1, 31,0);
        tracep->declBit(c+43,"tb_task_queue distributor consumer_ready", false,-1);
        tracep->declBit(c+1,"tb_task_queue distributor buffered_valid", false,-1);
        tracep->declBus(c+2,"tb_task_queue distributor buffered_data", false,-1, 31,0);
        tracep->declBus(c+24,"tb_task_queue distributor in_data_reg", false,-1, 31,0);
        tracep->declBit(c+29,"tb_task_queue arbiter clk", false,-1);
        tracep->declBit(c+30,"tb_task_queue arbiter reset", false,-1);
        tracep->declBit(c+36,"tb_task_queue arbiter in_valid", false,-1);
        tracep->declBus(c+37,"tb_task_queue arbiter in_data", false,-1, 31,0);
        tracep->declBus(c+3,"tb_task_queue arbiter grant_out", false,-1, 1,0);
        tracep->declBus(c+4,"tb_task_queue arbiter served_bank", false,-1, 1,0);
        tracep->declBit(c+25,"tb_task_queue arbiter rr", false,-1);
        tracep->declBit(c+39,"tb_task_queue arbiter dummy_bit", false,-1);
    }
}

void Vtb_task_queue::traceRegister(VerilatedVcd* tracep) {
    // Body
    {
        tracep->addFullCb(&traceFullTop0, __VlSymsp);
        tracep->addChgCb(&traceChgTop0, __VlSymsp);
        tracep->addCleanupCb(&traceCleanup, __VlSymsp);
    }
}

void Vtb_task_queue::traceFullTop0(void* userp, VerilatedVcd* tracep) {
    Vtb_task_queue__Syms* __restrict vlSymsp = static_cast<Vtb_task_queue__Syms*>(userp);
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    // Body
    {
        vlTOPp->traceFullSub0(userp, tracep);
    }
}

void Vtb_task_queue::traceFullSub0(void* userp, VerilatedVcd* tracep) {
    Vtb_task_queue__Syms* __restrict vlSymsp = static_cast<Vtb_task_queue__Syms*>(userp);
    Vtb_task_queue* const __restrict vlTOPp VL_ATTR_UNUSED = vlSymsp->TOPp;
    vluint32_t* const oldp = tracep->oldp(vlSymsp->__Vm_baseCode);
    if (false && oldp) {}  // Prevent unused
    // Body
    {
        tracep->fullBit(oldp+1,(vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_valid));
        tracep->fullIData(oldp+2,(vlTOPp->tb_task_queue__DOT__distributor__DOT__buffered_data),32);
        tracep->fullCData(oldp+3,(vlTOPp->tb_task_queue__DOT__arb_grant_out),2);
        tracep->fullCData(oldp+4,(vlTOPp->tb_task_queue__DOT__arb_served_bank),2);
        tracep->fullIData(oldp+5,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[0]),32);
        tracep->fullIData(oldp+6,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[1]),32);
        tracep->fullIData(oldp+7,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[2]),32);
        tracep->fullIData(oldp+8,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[3]),32);
        tracep->fullIData(oldp+9,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[4]),32);
        tracep->fullIData(oldp+10,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[5]),32);
        tracep->fullIData(oldp+11,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[6]),32);
        tracep->fullIData(oldp+12,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[7]),32);
        tracep->fullIData(oldp+13,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[8]),32);
        tracep->fullIData(oldp+14,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[9]),32);
        tracep->fullIData(oldp+15,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[10]),32);
        tracep->fullIData(oldp+16,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[11]),32);
        tracep->fullIData(oldp+17,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[12]),32);
        tracep->fullIData(oldp+18,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[13]),32);
        tracep->fullIData(oldp+19,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[14]),32);
        tracep->fullIData(oldp+20,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__mem[15]),32);
        tracep->fullCData(oldp+21,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__head),4);
        tracep->fullCData(oldp+22,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__tail),4);
        tracep->fullCData(oldp+23,(vlTOPp->tb_task_queue__DOT__dut_queue__DOT__count),5);
        tracep->fullIData(oldp+24,(vlTOPp->tb_task_queue__DOT__distributor__DOT__in_data_reg),32);
        tracep->fullBit(oldp+25,(vlTOPp->tb_task_queue__DOT__arbiter__DOT__rr));
        tracep->fullBit(oldp+26,(vlTOPp->tb_task_queue__DOT__push_req));
        tracep->fullIData(oldp+27,(vlTOPp->tb_task_queue__DOT__data_in),32);
        tracep->fullBit(oldp+28,(vlTOPp->tb_task_queue__DOT__pop_req));
        tracep->fullBit(oldp+29,(vlTOPp->clk));
        tracep->fullBit(oldp+30,(vlTOPp->reset));
        tracep->fullBit(oldp+31,(vlTOPp->host_mode));
        tracep->fullBit(oldp+32,(vlTOPp->host_push_req));
        tracep->fullIData(oldp+33,(vlTOPp->host_data_in),32);
        tracep->fullBit(oldp+34,(vlTOPp->host_pop_req));
        tracep->fullBit(oldp+35,(vlTOPp->full));
        tracep->fullBit(oldp+36,(vlTOPp->valid_out));
        tracep->fullIData(oldp+37,(vlTOPp->data_out),32);
        tracep->fullBit(oldp+38,(vlTOPp->tb_done));
        tracep->fullBit(oldp+39,((1U & VL_REDXOR_32(vlTOPp->data_out))));
        tracep->fullIData(oldp+40,(0x10U),32);
        tracep->fullIData(oldp+41,(0x20U),32);
        tracep->fullIData(oldp+42,(4U),32);
        tracep->fullBit(oldp+43,(1U));
    }
}
