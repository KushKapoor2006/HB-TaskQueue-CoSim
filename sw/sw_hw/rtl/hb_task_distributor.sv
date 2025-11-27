// hb_task_distributor.sv
module hb_task_distributor (
    input  logic clk,
    input  logic reset,                 // synchronous reset
    input  logic in_valid,
    input  logic [31:0] in_data,
    output logic out_valid,
    output logic [31:0] out_data,
    input  logic consumer_ready
);

    logic buffered_valid;
    logic [31:0] buffered_data;
    logic [31:0] in_data_reg;

    assign out_valid = buffered_valid;
    assign out_data  = buffered_data;

    // capture in_data to use it (avoid UNUSED warning)
    always_ff @(posedge clk) begin
        if (reset) begin
            buffered_valid <= 0;
            buffered_data <= 32'h0;
            in_data_reg <= 32'h0;
        end else begin
            in_data_reg <= in_data;
            if (buffered_valid && consumer_ready) begin
                buffered_valid <= 0;
            end

            if (in_valid && !buffered_valid) begin
                buffered_data <= in_data_reg;
                buffered_valid <= 1;
            end
        end
    end

endmodule
