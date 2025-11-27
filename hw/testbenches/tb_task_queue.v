// tb_task_queue.v
`timescale 1ns/1ps

module tb_task_queue (
    input  logic clk,
    input  logic reset,

    // Host-side control ports (driven by the Verilator host harness)
    input  logic host_mode,           // when 1, host controls push/pop
    input  logic host_push_req,
    input  logic [31:0] host_data_in,
    input  logic host_pop_req,

    // Observability for the host
    output logic full,
    output logic valid_out,
    output logic [31:0] data_out,

    // When TB/host finished
    output logic tb_done
);

    reg        push_req;
    reg [31:0] data_in;
    reg        pop_req;

    wire       dist_out_valid;
    wire [31:0] dist_out_data;
    wire [1:0] arb_grant_out;
    wire [1:0] arb_served_bank;

    initial tb_done = 1'b0;

    hb_task_queue_core #(.DEPTH(16)) dut_queue (
        .clk(clk),
        .reset(reset),
        .push_req(push_req),
        .data_in(data_in),
        .full(full),
        .valid_out(valid_out),
        .data_out(data_out),
        .pop_req(pop_req)
    );

    hb_task_distributor distributor (
        .clk(clk),
        .reset(reset),
        .in_valid(valid_out),
        .in_data(data_out),
        .out_valid(dist_out_valid),
        .out_data(dist_out_data),
        .consumer_ready(1'b1)
    );

    hb_arbiter_banked arbiter (
        .clk(clk),
        .reset(reset),
        .in_valid(valid_out),
        .in_data(data_out),
        .grant_out(arb_grant_out),
        .served_bank(arb_served_bank)
    );

    always_comb begin
        if (host_mode) begin
            push_req = host_push_req;
            data_in  = host_data_in;
            pop_req  = host_pop_req;
        end else begin
            push_req = 1'b0;
            data_in  = 32'h0;
            pop_req  = 1'b0;
        end
    end

    wire __unused_signals = dist_out_valid | (|dist_out_data) | (|arb_grant_out) | (|arb_served_bank);
    // synthesis translate_off
    initial begin
        if (__unused_signals) begin end
    end
    // synthesis translate_on

endmodule
