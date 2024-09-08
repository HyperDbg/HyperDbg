`timescale 1ns/1ps


module ro(
//   input ck_io41,
//   output ck_io33
  input wire R_in,
  output wire R_out);
wire a /* synthesis keep */;
wire b /* synthesis keep */;


assign b = a & R_in;
//assign b = a ;

wire [2:0] x /* synthesis keep */; 

not_gate_simple not0(
  .X(b),
  .Y(x[0]));

not_gate_simple not1(
  .X(x[0]),
  .Y(x[1]));

not_gate_simple not2(
  .X(x[1]),
  .Y(x[2]));

//not_gate_simple not3(
//  .X(x[2]),
//  .Y(x[3]));

//not_gate_simple not4(
//  .X(x[3]),
//  .Y(x[4]));

//not_gate_simple not5(
//  .X(x[4]),
//  .Y(x[5]));
  
//not_gate_simple not6(
//  .X(x[5]),
//  .Y(x[6]));
  
//not_gate_simple not7(
//  .X(x[6]),
//  .Y(x[7]));
  
//not_gate_simple not8(
//  .X(x[7]),
//  .Y(x[8]));
  
//not_gate_simple not9(
//  .X(x[8]),
//  .Y(x[9]));
 
//not_gate_simple not10(
//  .X(x[9]),
//  .Y(x[10]));
  
  
  
  
assign R_out = x[2];
assign a = x[2] & R_in;



endmodule






