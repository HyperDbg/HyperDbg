/**
 * @file
 *   script_buffer_handler.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Configures the script stages from shared memory
 * @details
 * @version 0.1
 * @date
 *   2024-06-14
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.communication.interpreter

import chisel3._
import chisel3.util.{switch, is}
import circt.stage.ChiselStage

import hwdbg.configs._
import hwdbg.types._
import hwdbg.script._

class InterpreterScriptBufferHandler(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation,
    bramDataWidth: Int
) extends Module {

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Receiving signals
    //
    val readNextData = Output(Bool()) // whether the next data should be read or not?

    val dataValidInput = Input(Bool()) // whether data on the receiving data line is valid or not?
    val receivingData = Input(UInt(bramDataWidth.W)) // data to be received in interpreter

    //
    // Script stage configuration signals
    //
    val finishedConfiguration = Output(Bool()) // whether configuration finished or not?
    val moveToNextStage = Output(Bool()) // whether configuration finished configuring the current stage or not?
    val targetOperator = Output(new HwdbgShortSymbol(instanceInfo.scriptVariableLength)) // Current operator to be configured
  })

  // ---------------------------------------------------------------------

  //
  // Connect output pins
  //

}

object InterpreterScriptBufferHandler {

  def apply(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation,
    bramDataWidth: Int
  )(
      en: Bool,
      dataValidInput: Bool,
      receivingData: UInt
  ): (Bool, Bool, Bool, HwdbgShortSymbol) = {

    val interpreterScriptBufferHandler = Module(
      new InterpreterScriptBufferHandler(
        debug,
        instanceInfo,
        bramDataWidth
      )
    )

    val readNextData = Wire(Bool())

    val finishedConfiguration = Wire(Bool())
    val moveToNextStage = Wire(Bool())
    val targetOperator = Wire(new HwdbgShortSymbol(instanceInfo.scriptVariableLength))

    //
    // Configure the input signals
    //
    interpreterScriptBufferHandler.io.en := en

    //
    // Configure the input signals related to the receiving signals
    //
    interpreterScriptBufferHandler.io.dataValidInput := dataValidInput
    interpreterScriptBufferHandler.io.receivingData := receivingData

    //
    // Configure the output signals
    //
    readNextData := interpreterScriptBufferHandler.io.readNextData

    //
    // Configure the output signals related to configuring stage operators
    //
    finishedConfiguration := interpreterScriptBufferHandler.io.finishedConfiguration
    moveToNextStage := interpreterScriptBufferHandler.io.moveToNextStage
    targetOperator := interpreterScriptBufferHandler.io.targetOperator

    //
    // Return the output result
    //
    (
      readNextData,
      finishedConfiguration,
      moveToNextStage,
      targetOperator
    )
  }
}
