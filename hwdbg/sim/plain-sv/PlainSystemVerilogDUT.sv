//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: Sina Karvandi (sina@hyperdbg.org)
// 
// Create Date: 09/18/2024
// Design Name: 
// Module Name: PlainSystemVerilogDUT
// Project Name: PlainSystemVerilogDUT
// Target Devices: 
// Tool Versions: 
// Description: This code is used for plain simulation of the top-level 
// design of hwdbg debugger
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

module PlainSystemVerilogDUT;

  //
  // Parameters
  //
  parameter CLK_PERIOD = 10;  // Clock period

  //
  // Signal Declarations
  //
  logic clock;
  logic reset;
  logic io_en;
  logic [31:0] io_inputPin;    // Array for input pins
  logic [31:0] io_outputPin;   // Array for output pins
  logic io_plInSignal;
  logic io_psOutInterrupt;

  //
  // Instantiate the DUT (Device Under Test)
  //
  DebuggerModuleTestingBRAM uut (
    .clock(clock),
    .reset(reset),
    .io_en(io_en),
    .io_inputPin_0(io_inputPin[0]),
    .io_inputPin_1(io_inputPin[1]),
    .io_inputPin_2(io_inputPin[2]),
    .io_inputPin_3(io_inputPin[3]),
    .io_inputPin_4(io_inputPin[4]),
    .io_inputPin_5(io_inputPin[5]),
    .io_inputPin_6(io_inputPin[6]),
    .io_inputPin_7(io_inputPin[7]),
    .io_inputPin_8(io_inputPin[8]),
    .io_inputPin_9(io_inputPin[9]),
    .io_inputPin_10(io_inputPin[10]),
    .io_inputPin_11(io_inputPin[11]),
    .io_inputPin_12(io_inputPin[12]),
    .io_inputPin_13(io_inputPin[13]),
    .io_inputPin_14(io_inputPin[14]),
    .io_inputPin_15(io_inputPin[15]),
    .io_inputPin_16(io_inputPin[16]),
    .io_inputPin_17(io_inputPin[17]),
    .io_inputPin_18(io_inputPin[18]),
    .io_inputPin_19(io_inputPin[19]),
    .io_inputPin_20(io_inputPin[20]),
    .io_inputPin_21(io_inputPin[21]),
    .io_inputPin_22(io_inputPin[22]),
    .io_inputPin_23(io_inputPin[23]),
    .io_inputPin_24(io_inputPin[24]),
    .io_inputPin_25(io_inputPin[25]),
    .io_inputPin_26(io_inputPin[26]),
    .io_inputPin_27(io_inputPin[27]),
    .io_inputPin_28(io_inputPin[28]),
    .io_inputPin_29(io_inputPin[29]),
    .io_inputPin_30(io_inputPin[30]),
    .io_inputPin_31(io_inputPin[31]),
    .io_outputPin_0(io_outputPin[0]),
    .io_outputPin_1(io_outputPin[1]),
    .io_outputPin_2(io_outputPin[2]),
    .io_outputPin_3(io_outputPin[3]),
    .io_outputPin_4(io_outputPin[4]),
    .io_outputPin_5(io_outputPin[5]),
    .io_outputPin_6(io_outputPin[6]),
    .io_outputPin_7(io_outputPin[7]),
    .io_outputPin_8(io_outputPin[8]),
    .io_outputPin_9(io_outputPin[9]),
    .io_outputPin_10(io_outputPin[10]),
    .io_outputPin_11(io_outputPin[11]),
    .io_outputPin_12(io_outputPin[12]),
    .io_outputPin_13(io_outputPin[13]),
    .io_outputPin_14(io_outputPin[14]),
    .io_outputPin_15(io_outputPin[15]),
    .io_outputPin_16(io_outputPin[16]),
    .io_outputPin_17(io_outputPin[17]),
    .io_outputPin_18(io_outputPin[18]),
    .io_outputPin_19(io_outputPin[19]),
    .io_outputPin_20(io_outputPin[20]),
    .io_outputPin_21(io_outputPin[21]),
    .io_outputPin_22(io_outputPin[22]),
    .io_outputPin_23(io_outputPin[23]),
    .io_outputPin_24(io_outputPin[24]),
    .io_outputPin_25(io_outputPin[25]),
    .io_outputPin_26(io_outputPin[26]),
    .io_outputPin_27(io_outputPin[27]),
    .io_outputPin_28(io_outputPin[28]),
    .io_outputPin_29(io_outputPin[29]),
    .io_outputPin_30(io_outputPin[30]),
    .io_outputPin_31(io_outputPin[31]),
    .io_plInSignal(io_plInSignal),
    .io_psOutInterrupt(io_psOutInterrupt)
  );

  //
  // Clock generation
  //
  initial begin
    clock = 0;
    forever #(CLK_PERIOD / 2) clock = ~clock;
  end

  //
  // Test procedure
  //
  initial begin
    // Initialize signals
    reset = 1;
    io_en = 1;
    io_plInSignal = 0;
    io_inputPin = 32'h0;  // Initialize all input pins to 0
    
    #100; // Hold reset for 100 ns

    // Apply reset
    @(posedge clock);
    reset = 0;  // Release reset
    @(posedge clock);
    
    //
    // Example input stimulus
    //
    io_inputPin = 32'hFFFF_FFFF;  // Apply some input pattern

    //
    // Enable PL input signal
    //
    io_plInSignal = 1;
    
    //
    // Wait for a few clock cycles
    //
    repeat (10000) @(posedge clock);

    //
    // Finish the simulation after some time
    //
    repeat (20) @(posedge clock);
    $finish;
  end

endmodule
