`timescale 1ps / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 11/13/2023 05:16:59 PM
// Design Name: 
// Module Name: not_gate_simple
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


module not_gate_simple(
input wire X,
output wire Y
    );
    
//     (* ALLOW_COMBINATORIAL_LOOPS  = "yes" *)   assign #10 Y = ~X;
          (* ALLOW_COMBINATORIAL_LOOPS  = "yes" *)   assign  Y = ~X;

endmodule
