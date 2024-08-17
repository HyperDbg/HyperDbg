`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 06/20/2024 10:21:57 AM
// Design Name: 
// Module Name: freq_generator
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module freq_generator(
   input logic clk_in,        // Input clock
    input logic reset,         // Reset signal
    input logic [31:0] div_int, // Integer part of the division factor
    input logic [31:0] div_frac, // Fractional part of the division factor (in fixed-point format, e.g., 0.5 = 32'd50000000)
    output logic clk_out       // Output clock
);

    logic [31:0] int_counter;
    logic [31:0] frac_counter;
    logic frac_tick;
    logic clk_out_internal;

    // Integer Clock Division Logic
    always_ff @(posedge clk_in or posedge reset) begin
        if (reset) begin
            int_counter <= 32'd0;
            clk_out_internal <= 1'b0;
        end else begin
            if (int_counter >= (div_int - 1)) begin
                int_counter <= 32'd0;
                clk_out_internal <= ~clk_out_internal;
            end else begin
                int_counter <= int_counter + 1;
            end
        end
    end

    // Fractional Clock Division Logic
    always_ff @(posedge clk_in or posedge reset) begin
        if (reset) begin
            frac_counter <= 32'd0;
            frac_tick <= 1'b0;
        end else begin
            frac_counter <= frac_counter + div_frac;
            if (frac_counter >= 32'd100000000) begin // When the counter exceeds 1.0 in fixed-point format
                frac_counter <= frac_counter - 32'd100000000;
                frac_tick <= 1'b1;
            end else begin
                frac_tick <= 1'b0;
            end
        end
    end

    // Combine Integer and Fractional Divisions
    always_ff @(posedge clk_in or posedge reset) begin
        if (reset) begin
            clk_out <= 1'b0;
        end else if (int_counter == 0 && frac_tick) begin
            clk_out <= ~clk_out_internal;
        end else begin
            clk_out <= clk_out_internal;
        end
    end

endmodule
