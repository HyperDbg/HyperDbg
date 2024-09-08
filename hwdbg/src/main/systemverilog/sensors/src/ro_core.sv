`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 12/01/2022 04:46:30 PM
// Design Name: 
// Module Name: reg1_core
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





module ro_core(
	input wire [0:0] in_d,
	output wire [0:0] wire_inj
    );
//    localparam size_i = 127000;
//localparam size_i = 64000;
//localparam size_i = 1000;
//localparam size_i = 128;
localparam size_i = 1;

    reg ce1;
    reg CLR1;
    wire [size_i-1:0] inp_r;
    
//   assign inp_r=in_d[size_i-1:0];
   assign inp_r=in_d[size_i-1:0];

   
   (* S = "TRUE" *)(* KEEP = "TRUE" *) wire [size_i-1:0] q_out;
   assign wire_inj = q_out;
    
   

ro_fanout ro_fanout_ins[size_i-1:0] (
   .R_in(inp_r),      // 1-bit Enable
   .R_out(q_out)        // 1-bit Data output
);



endmodule




