`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 03/15/2024 01:55:18 PM
// Design Name: 
// Module Name: ro_fanout
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


module ro_fanout(
input wire R_in,
output wire R_out
    );

        localparam size_i = 1;
        
    (* S = "TRUE" *) (* KEEP = "TRUE" *) wire [size_i-1:0] ro_input;
        (* S = "TRUE" *) (* KEEP = "TRUE" *) wire [size_i-1:0] ro_output;
        
    assign R_out = ro_output[0];

    assign ro_input =  ({size_i{R_in}});
     
     (* S = "TRUE" *) (* KEEP = "TRUE" *) ro ro_inst[size_i-1:0](
     .R_in(ro_input),
     .R_out(ro_output)
     
     );
     
     
//          (* S = "TRUE" *) (* KEEP = "TRUE" *) ro_sv #(.INVERTERS_PER_RING(3), .INVERTER_DELAY(1)) ro_inst[size_i-1:0](
//     .R_in(ro_input),
//     .R_out(ro_output)
     
//     );
     
endmodule
