/**
 * @file
 *   send_success_or_error.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Send an indication of invalid packet error or success message (in the interpreter)
 * @details
 * @version 0.1
 * @date
 *   2024-05-04
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.communication.interpreter

import chisel3._
import chisel3.util.{switch, is}
import circt.stage.ChiselStage

import hwdbg.configs._

class InterpreterSendSuccessOrError(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation
) extends Module {

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal
    val lastSuccessOrError = Input(UInt(instanceInfo.bramDataWidth.W)) // input last error

    //
    // Sending singals
    //
    val noNewDataSender = Output(Bool()) // should sender finish sending buffers or not?
    val dataValidOutput = Output(Bool()) // should sender send next buffer or not?
    val sendingData = Output(UInt(instanceInfo.bramDataWidth.W)) // data to be sent to the debugger

  })

  //
  // Output pins
  //
  val noNewDataSender = WireInit(false.B)
  val dataValidOutput = WireInit(false.B)
  val sendingData = WireInit(0.U(instanceInfo.bramDataWidth.W))

  //
  // Apply the chip enable signal
  //
  when(io.en === true.B) {

    //
    // Set the version
    //
    sendingData := io.lastSuccessOrError

    //
    // Sending the version in one clock cycle
    //
    noNewDataSender := true.B
    dataValidOutput := true.B

  }

  // ---------------------------------------------------------------------

  //
  // Connect output pins
  //
  io.noNewDataSender := noNewDataSender
  io.dataValidOutput := dataValidOutput
  io.sendingData := sendingData

}

object InterpreterSendSuccessOrError {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      lastSuccessOrError: UInt
  ): (Bool, Bool, UInt) = {

    val interpreterSendSuccessOrError = Module(
      new InterpreterSendSuccessOrError(
        debug,
        instanceInfo
      )
    )

    val noNewDataSender = Wire(Bool())
    val dataValidOutput = Wire(Bool())
    val sendingData = Wire(UInt(instanceInfo.bramDataWidth.W))

    //
    // Configure the input signals
    //
    interpreterSendSuccessOrError.io.en := en
    interpreterSendSuccessOrError.io.lastSuccessOrError := lastSuccessOrError

    //
    // Configure the output signals
    //
    noNewDataSender := interpreterSendSuccessOrError.io.noNewDataSender
    dataValidOutput := interpreterSendSuccessOrError.io.dataValidOutput
    sendingData := interpreterSendSuccessOrError.io.sendingData

    //
    // Return the output result
    //
    (
      noNewDataSender,
      dataValidOutput,
      sendingData
    )
  }
}
