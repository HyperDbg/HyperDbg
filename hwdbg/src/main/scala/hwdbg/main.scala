/**
 * @file
 *   main.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   hwdbg's main debugger module
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

import hwdbg.configs._
import hwdbg.types._
import hwdbg.utils._
import hwdbg.script._
import hwdbg.communication._
import hwdbg.communication.interpreter._

class DebuggerMain(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    numberOfPins: Int,
    maximumNumberOfStages: Int,
    maximumNumberOfSupportedScriptOperators: Int,
    scriptVariableLength: Int,
    bramAddrWidth: Int,
    bramDataWidth: Int,
    portsConfiguration: Map[Int, Int]
) extends Module {

  //
  // Ensure sum of input port values equals numberOfPins (NUMBER_OF_PINS)
  //
  require(
    portsConfiguration.values.sum == numberOfPins,
    "err, the sum of the portsConfiguration (PORT_PINS_MAP) values must equal the numberOfPins (NUMBER_OF_PINS)."
  )

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
  // *** Create an instance of the debugger ***
  //
  val debuggerInstance = HwdbgInstanceInformation.createInstanceInformation(
                              version = Version.getEncodedVersion,
                              maximumNumberOfStages = maximumNumberOfStages,
                              scriptVariableLength = scriptVariableLength,
                              numberOfPins = numberOfPins,
                              numberOfPorts = portsConfiguration.size,
                              enabledCapabilities = ScriptEngineConfigurations.SCRIPT_ENGINE_EVAL_CAPABILITIES
    )

  //
  // Wire signals for the synchronizer
  //
  val requestedActionOfThePacketOutput = Wire(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W))
  val requestedActionOfThePacketOutputValid = Wire(Bool())
  val dataValidOutput = Wire(Bool())
  val receivingData = Wire(UInt(bramDataWidth.W))
  val sendWaitForBuffer = Wire(Bool())

  // -----------------------------------------------------------------------
  // Create instance from script execution engine
  //
  val (outputPin) =
    ScriptExecutionEngine(
      debug,
      numberOfPins,
      maximumNumberOfStages,
      maximumNumberOfSupportedScriptOperators,
      scriptVariableLength,
      bramAddrWidth,
      bramDataWidth,
      portsConfiguration
    )(
      io.en,
      io.inputPin
    )

  // -----------------------------------------------------------------------
  // Create instance from interpreter
  //
  val (
    noNewDataReceiver,
    readNextData,
    beginSendingBuffer,
    noNewDataSender,
    dataValidInterpreterOutput,
    requestedActionOfThePacketInterpreterOutput,
    sendingData
  ) =
    DebuggerPacketInterpreter(
      debug,
      bramAddrWidth,
      bramDataWidth,
      portsConfiguration
    )(
      io.en,
      requestedActionOfThePacketOutput,
      requestedActionOfThePacketOutputValid,
      dataValidOutput,
      receivingData,
      sendWaitForBuffer
    )

  // -----------------------------------------------------------------------
  // Create instance from synchronizer
  //
  val (
    psOutInterrupt,
    rdWrAddr,
    wrEna,
    wrData,
    outRequestedActionOfThePacketOutput,
    outRequestedActionOfThePacketOutputValid,
    outDataValidOutput,
    outReceivingData,
    outSendWaitForBuffer
  ) =
    SendReceiveSynchronizer(
      debug,
      bramAddrWidth,
      bramDataWidth
    )(
      io.en,
      io.plInSignal,
      io.rdData,
      noNewDataReceiver,
      readNextData,
      beginSendingBuffer,
      noNewDataSender,
      dataValidInterpreterOutput,
      requestedActionOfThePacketInterpreterOutput,
      sendingData
    )

  // -----------------------------------------------------------------------
  // Connect synchronizer signals to wires
  //
  requestedActionOfThePacketOutput := outRequestedActionOfThePacketOutput
  requestedActionOfThePacketOutputValid := outRequestedActionOfThePacketOutputValid
  dataValidOutput := outDataValidOutput
  receivingData := outReceivingData
  sendWaitForBuffer := outSendWaitForBuffer

  // -----------------------------------------------------------------------
  // Configure the output signals
  //
  io.wrEna := wrEna
  io.wrData := wrData
  io.rdWrAddr := rdWrAddr

  io.outputPin := outputPin

  io.psOutInterrupt := psOutInterrupt

}

object DebuggerMain {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      numberOfPins: Int,
      maximumNumberOfStages: Int,
      maximumNumberOfSupportedScriptOperators: Int,
      scriptVariableLength: Int,
      bramAddrWidth: Int,
      bramDataWidth: Int,
      portsConfiguration: Map[Int, Int]
  )(
      en: Bool,
      inputPin: Vec[UInt],
      plInSignal: Bool,
      rdData: UInt
  ): (Vec[UInt], Bool, UInt, Bool, UInt) = {

    val debuggerMainModule = Module(
      new DebuggerMain(
        debug,
        numberOfPins,
        maximumNumberOfStages,
        maximumNumberOfSupportedScriptOperators,
        scriptVariableLength,
        bramAddrWidth,
        bramDataWidth,
        portsConfiguration
      )
    )

    val outputPin = Wire(Vec(numberOfPins, UInt(1.W)))
    val psOutInterrupt = Wire(Bool())
    val rdWrAddr = Wire(UInt(bramAddrWidth.W))
    val wrEna = Wire(Bool())
    val wrData = Wire(UInt(bramDataWidth.W))

    //
    // Configure the input signals
    //
    debuggerMainModule.io.en := en
    debuggerMainModule.io.inputPin := inputPin
    debuggerMainModule.io.plInSignal := plInSignal
    debuggerMainModule.io.rdData := rdData

    //
    // Configure the output signals
    //
    outputPin := debuggerMainModule.io.outputPin
    psOutInterrupt := debuggerMainModule.io.psOutInterrupt
    rdWrAddr := debuggerMainModule.io.rdWrAddr
    wrEna := debuggerMainModule.io.wrEna
    wrData := debuggerMainModule.io.wrData

    //
    // Return the output result
    //
    (outputPin, psOutInterrupt, rdWrAddr, wrEna, wrData)
  }
}
