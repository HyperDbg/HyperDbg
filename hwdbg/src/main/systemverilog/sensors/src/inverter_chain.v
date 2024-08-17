`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 09/11/2023 11:01:41 AM
// Design Name: 
// Module Name: inverter_chain
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


module Chain(
    chain_en,
    in_data,
    out_data
    );
    parameter N = 2000; //This is the number of inverters in chain
    input wire chain_en; 
    
    input wire in_data; // This has to be connected to the input pin of the design according to the XDC file
    
    output wire out_data; // make sure to connect it to one of the output pin of the circuit according to the XDC file
    (* dont_touch = "yes" *) wire [N-1:0] intermadiate_value; //For D-flip-flop implementation, make it reg instead of wire!
    (* dont_touch = "yes" *)   inverter inverter_in (.in_data(in_data&chain_en),.out_data(intermadiate_value[0])); // this is just for initiation, for loop chain, you can remove it.
    generate 
        genvar i;
        for (i=0; i<N-1 ; i = i + 1) begin
            (* dont_touch = "yes" *)   inverter inverter_i (.in_data(intermadiate_value[i]),.out_data(intermadiate_value[i+1]));
        end
    endgenerate
    (* dont_touch = "yes" *)   inverter inverter_out (.in_data(intermadiate_value[N-1]),.out_data(out_data)); // this is just for output connection, for loop chain, you can remove it.
endmodule
