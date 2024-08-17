`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/08/2024 04:39:27 PM
// Design Name: 
// Module Name: PL_CORE
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


module PL_CORE(

(* clock_buffer_type="none" *) input  wire QDR4_CLK_100MHZ_P,
(* clock_buffer_type="none" *) input  wire QDR4_CLK_100MHZ_N,

//    input   wire reg_clk,
    input   wire uart_rx_line,
//    input wire reg_eop,
//    input   logic SPI_nCS,
//    output  wire clk_reg_out,
    output  wire uart_tx_line,
//    output  logic laser_module_trigger,

    output  wire [3:0] led_out_ff
);

wire clk_main;
  clk_wiz_11 clock_main
   (
    // Clock out ports
    .clk_main(clk_main),     // output clk_main
    // Status and control signals
    .reset(reset), // input reset
    .locked(locked),       // output locked
   // Clock in ports
    .clk_in1_p(QDR4_CLK_100MHZ_P),    // input clk_in1_p
    .clk_in1_n(QDR4_CLK_100MHZ_N)    // input clk_in1_n
);


UART_FF UART_FF_INS (
        .clk                    (clk_main),
        .rst                    ('b0),
        .reg_clk               (clk_main),
        .reg_eop                (clk_main),
        .uart_rx_line        (uart_rx_line),
        .uart_tx_line           (uart_tx_line),
        .gpio                   (gpio),
        .led_out_ff            (led_out_ff)
    );

endmodule
