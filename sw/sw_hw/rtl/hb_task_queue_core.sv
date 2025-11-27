// hw/rtl/hb_task_queue_core.sv
module hb_task_queue_core #(
    parameter DEPTH = 16,
    parameter WIDTH = 32
)(
    input  logic clk,
    input  logic reset,
    input  logic push_req,
    input  logic [WIDTH-1:0] data_in,
    output logic full,
    output logic valid_out,
    output logic [WIDTH-1:0] data_out,
    input  logic pop_req
);

    localparam PTR_W = $clog2(DEPTH);
    logic [WIDTH-1:0] mem [0:DEPTH-1];
    logic [PTR_W-1:0] head, tail;
    logic [$clog2(DEPTH+1)-1:0] count;

    assign full = (count == DEPTH);
    assign valid_out = (count != 0);
    assign data_out = mem[head];

    always_ff @(posedge clk) begin
        if (reset) begin
            head <= '0;
            tail <= '0;
            count <= '0;
        end else begin
            if (push_req && !full) begin
                mem[tail] <= data_in;
                tail <= tail + 1;
                count <= count + 1;
            end
            if (pop_req && (count != 0)) begin
                head <= head + 1;
                count <= count - 1;
            end
        end
    end

endmodule
