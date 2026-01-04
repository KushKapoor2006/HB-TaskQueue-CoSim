module task_queue_top (
    input  logic clk,
    input  logic reset,
    
    // Producer Interface
    input  logic push_req,
    input  logic [31:0] data_in,
    output logic full,
    
    // Consumer Interface (Simulated)
    input  logic consumer_ready,
    output logic [1:0] grant_out,
    output logic [31:0] final_data_out
);

    // Internal Signals
    logic fifo_valid, fifo_pop;
    logic [31:0] fifo_data;
    logic dist_valid;
    logic [31:0] dist_data;
    
    // 1. The Core FIFO (Stores the Task)
    hb_task_queue_core #(.DEPTH(16)) u_queue (
        .clk(clk),
        .reset(reset),
        .push_req(push_req),
        .data_in(data_in),
        .full(full),
        .valid_out(fifo_valid),
        .data_out(fifo_data),
        .pop_req(fifo_pop) // Controlled by Distributor
    );

    // 2. The Distributor (Fetches from FIFO, offers to consumers)
    // Logic to pop from FIFO only when Distributor accepts new data
    assign fifo_pop = fifo_valid && (!dist_valid || consumer_ready);

    hb_task_distributor u_dist (
        .clk(clk),
        .reset(reset),
        .in_valid(fifo_valid),
        .in_data(fifo_data),
        .out_valid(dist_valid), // Tells arbiter we have data
        .out_data(dist_data),
        .consumer_ready(consumer_ready)
    );

    // 3. The Arbiter (Decides which bank gets the task)
    // We verify the path through the arbiter logic
    logic [1:0] served_bank;
    hb_arbiter_banked u_arb (
        .clk(clk),
        .reset(reset),
        .in_valid(dist_valid),
        .in_data(dist_data), // Connected to prevent optimization
        .grant_out(grant_out),
        .served_bank(served_bank)
    );

    // Output mapping to preserve logic
    assign final_data_out = dist_data;

endmodule