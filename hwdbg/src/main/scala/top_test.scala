/**
 * @file
 *   top_test.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   hwdbg's top module (with BRAM) for testing
 * @details
 * @version 0.1
 * @date
 *   2024-04-04
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg

import chisel3._
import circt.stage.ChiselStage

import hwdbg._
import hwdbg.configs._
import hwdbg.libs.mem._

class DebuggerModuleTestingBRAM(
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
    // *** BRAM (Block RAM) ports are initialized from an external file ***
    //

  })

  val bramEn = WireInit(false.B)
  val bramWrite = WireInit(false.B)
  val bramAddr = WireInit(0.U(bramAddrWidth.W))
  val bramDataIn = WireInit(0.U(bramDataWidth.W))

  val bramDataOut = WireInit(0.U(bramDataWidth.W))

  //
  // Instantiate the BRAM memory initializer module
  //
  val dataOut =
    InitRegMemFromFile(
      debug,
      MemoryCommunicationConfigurations.ENABLE_BLOCK_RAM_DELAY,
      TestingConfigurations.BRAM_INITIALIZATION_FILE_PATH,
      bramAddrWidth,
      bramDataWidth,
      MemoryCommunicationConfigurations.DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE
    )(
      bramEn,
      bramWrite,
      bramAddr,
      bramDataIn
    )

  bramDataOut := dataOut

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
      bramDataOut
    )

  //
  // Connect BRAM Pins
  //
  bramEn := io.en // enable BRAM when the main chip enabled
  bramAddr := rdWrAddr
  bramWrite := wrEna
  bramDataIn := wrData

  //
  // Connect I/O pins
  //
  io.outputPin := outputPin
  io.psOutInterrupt := psOutInterrupt

}

object MainWithInitializedBRAM extends App {

  //
  // Load configuration from JSON file
  //
  LoadConfiguration.loadFromJson("src/main/scala/hwdbg/configs/config.json")

  //
  // Generate hwdbg verilog files
  //
  println(
    ChiselStage.emitSystemVerilog(
      new DebuggerModuleTestingBRAM(
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
        "--lowering-options=disallowLocalVariables", // because icarus doesn't support 'automatic logic', this option prevents such logics
        "--split-verilog", // The intention for this argument (and next argument) is to separate generated files.
        "-o",
        "generated/"
      )
    )
  )
}
