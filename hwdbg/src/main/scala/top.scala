/**
 * @file
 *   top.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   hwdbg's top module
 * @details
 * @version 0.1
 * @date
 *   2024-04-03
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg

import chisel3._
import circt.stage.ChiselStage

import hwdbg._
import hwdbg.configs._

class DebuggerModule(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    numberOfPins: Int,
    maximumNumberOfStages: Int,
    maximumNumberOfSupportedGetScriptOperators: Int,
    maximumNumberOfSupportedSetScriptOperators: Int,
    sharedMemorySize: Int,
    debuggerAreaOffset: Int,
    debuggeeAreaOffset: Int,
    scriptVariableLength: Int,
    numberOfSupportedLocalAndGlobalVariables: Int,
    numberOfSupportedTemporaryVariables: Int,
    scriptCapabilities: Seq[Long],
    bramAddrWidth: Int,
    bramDataWidth: Int,
    portsConfiguration: Array[Int]
) extends Module {
  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Input/Output signals
    //
    val inputPin = Input(Vec(numberOfPins, UInt(1.W))) // input pins
    val outputPin = Output(Vec(numberOfPins, UInt(1.W))) // output pins

    //
    // Interrupt signals (lines)
    //
    val plInSignal = Input(Bool()) // PS to PL signal
    val psOutInterrupt = Output(Bool()) // PL to PS interrupt

    //
    // BRAM (Block RAM) ports
    //
    val rdWrAddr = Output(UInt(bramAddrWidth.W)) // read/write address
    val rdData = Input(UInt(bramDataWidth.W)) // read data
    val wrEna = Output(Bool()) // enable writing
    val wrData = Output(UInt(bramDataWidth.W)) // write data

  })

  //
  // Instantiate the debugger's main module
  //
  val (outputPin, psOutInterrupt, rdWrAddr, wrEna, wrData) =
    DebuggerMain(
      debug,
      numberOfPins,
      maximumNumberOfStages,
      maximumNumberOfSupportedGetScriptOperators,
      maximumNumberOfSupportedSetScriptOperators,
      sharedMemorySize,
      debuggerAreaOffset,
      debuggeeAreaOffset,
      scriptVariableLength,
      numberOfSupportedLocalAndGlobalVariables,
      numberOfSupportedTemporaryVariables,
      scriptCapabilities,
      bramAddrWidth,
      bramDataWidth,
      portsConfiguration
    )(
      io.en,
      io.inputPin,
      io.plInSignal,
      io.rdData
    )

  io.outputPin := outputPin
  io.psOutInterrupt := psOutInterrupt
  io.rdWrAddr := rdWrAddr
  io.wrEna := wrEna
  io.wrData := wrData

}

object Main extends App {

  //
  // Load configuration from JSON file
  //
  LoadConfiguration.loadFromJson("src/main/scala/hwdbg/configs/config.json")

  //
  // Generate hwdbg verilog files
  //
  println(
    ChiselStage.emitSystemVerilog(
      new DebuggerModule(
        DebuggerConfigurations.ENABLE_DEBUG,
        DebuggerConfigurations.NUMBER_OF_PINS,
        ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_STAGES,
        ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_GET_SCRIPT_OPERATORS,
        ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_SET_SCRIPT_OPERATORS,
        MemoryCommunicationConfigurations.DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE,
        MemoryCommunicationConfigurations.BASE_ADDRESS_OF_PS_TO_PL_COMMUNICATION,
        MemoryCommunicationConfigurations.BASE_ADDRESS_OF_PL_TO_PS_COMMUNICATION,
        ScriptEngineConfigurations.SCRIPT_VARIABLE_LENGTH,
        ScriptEngineConfigurations.NUMBER_OF_SUPPORTED_LOCAL_AND_GLOBAL_VARIABLES,
        ScriptEngineConfigurations.NUMBER_OF_SUPPORTED_TEMPORARY_VARIABLES,
        ScriptEngineConfigurations.SCRIPT_ENGINE_EVAL_CAPABILITIES,
        MemoryCommunicationConfigurations.BLOCK_RAM_ADDR_WIDTH,
        MemoryCommunicationConfigurations.BLOCK_RAM_DATA_WIDTH,
        DebuggerConfigurations.PORT_PINS_MAP
      ),
      firtoolOpts = Array(
        "-disable-all-randomization",
        // "-strip-debug-info",
        "--split-verilog", // The intention for this argument (and next argument) is to separate generated files.
        "-o",
        "generated/"
      )
    )
  )
}
