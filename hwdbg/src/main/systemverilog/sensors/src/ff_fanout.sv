`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/16/2023 02:48:20 PM
// Design Name: 
// Module Name: ff_fanout
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


module ff_fanout(
input wire clk,
input wire CE,
input wire rst,
input wire inp,
output reg res
    );
    
    

//reg rst_em; // input clock on FPGA
//reg[27:0] counter=28'd0;
//parameter DIVISOR = 28'd1;
//// The frequency of the output clk_out
////  = The frequency of the input clk_in divided by DIVISOR
//// For example: Fclk_in = 50Mhz, if you want to get 1Hz signal to blink LEDs
//// You will modify the DIVISOR parameter value to 28'd50.000.000
//// Then the frequency of the output clk_out = 50Mhz/50.000.000 = 1Hz
//always @(posedge rst)
//begin
// counter <= counter + 28'd1;
// if(counter>=(DIVISOR-1))
//  counter <= 28'd0;
  
  
// rst_em <= (counter<DIVISOR/2)?1'b1:1'b0;
//end



    localparam size_i = 256;
    
    
    (* S = "TRUE" *) (* KEEP = "TRUE" *) wire [size_i-1:0] q_output;
      (* S = "TRUE" *) (* KEEP = "TRUE" *) wire [size_i-1:0] d_input;
     assign d_input =  ({size_i{inp}});
     wire CLR;
     assign CLR= 1'b0;
     always @(posedge clk)begin
     
     res <= &q_output;

     end
     
     (* S = "TRUE" *) (* KEEP = "TRUE" *)  FDRE #(.INIT(1'b0)) FDRE_inst[size_i-1:0](
     .Q(q_output),
     .C(clk),
     .CE(1'b1),
     .R(rst),
     .D(d_input)
     
     
     );



//      (* S = "TRUE" *) (* KEEP = "TRUE" *) FDRE #(.INIT(1'b0)) FDRE_inst[size_i-1:0](
//     .Q(q_output),
//     .C(clk),
//     .CE(CE),
//     .R(1'b0),
//     .D(d_input)
     
     
//     );
     
endmodule
