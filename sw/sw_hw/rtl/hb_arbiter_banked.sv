// hb_arbiter_banked.sv
// Simple round-robin bank arbiter stub that accepts incoming valid/data and selects a bank each acceptance.
// Data is not used for decision-making, but we reference it in a non-synth block to avoid UNUSED warnings.

module hb_arbiter_banked (
    input  logic clk,
    input  logic reset,                 // synchronous reset
    input  logic in_valid,
    input  logic [31:0] in_data,
    output logic [1:0] grant_out,
    output logic [1:0] served_bank
);

    logic rr;

    // small, synthesizable reduction of in_data so we can reference the signal
    wire dummy_bit = ^in_data; // XOR-reduce to 1 bit

    always_ff @(posedge clk) begin
        if (reset) begin
            grant_out   <= 2'b00;
            served_bank <= 2'b00;
            rr          <= 0;
        end else begin
            grant_out   <= 2'b00;
            served_bank <= 2'b00;
            if (in_valid) begin
                if (rr == 0) begin
                    grant_out   <= 2'b01;
                    served_bank <= 2'b01;
                    rr          <= 1;
                end else begin
                    grant_out   <= 2'b10;
                    served_bank <= 2'b10;
                    rr          <= 0;
                end
            end
        end
    end

    // Reference dummy_bit in a non-synthesizable block so tools (like Verilator)
    // treat it as used while leaving synthesizable logic unchanged.
    // This avoids UNUSED warnings without affecting hardware behavior.
    // synthesis translate_off
    initial begin
        if (dummy_bit) begin
            // no-op: prevents dummy_bit from being optimized away / flagged unused
            $display("");
        end
    end
    // synthesis translate_on

endmodule
