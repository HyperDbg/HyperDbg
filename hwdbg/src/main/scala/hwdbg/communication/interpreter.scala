/**
 * @file
 *   interpreter.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Remote debugger packet interpreter module
 * @details
 * @version 0.1
 * @date
 *   2024-04-19
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

object DebuggerPacketInterpreterEnums {
  object State extends ChiselEnum {
    val sIdle, sNewActionReceived, sSendResponse, sDone = Value
  }
}

class DebuggerPacketInterpreter(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    bramAddrWidth: Int = DebuggerConfigurations.BLOCK_RAM_ADDR_WIDTH,
    bramDataWidth: Int = DebuggerConfigurations.BLOCK_RAM_DATA_WIDTH
) extends Module {

  //
  // Import state enum
  //
  import DebuggerPacketInterpreterEnums.State
  import DebuggerPacketInterpreterEnums.State._

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Receiving signals
    //
    val requestedActionOfThePacketInput = Input(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W)) // the requested action
    val requestedActionOfThePacketInputValid = Input(Bool()) // whether data on the requested action is valid or not

    val noNewDataReceiver = Output(Bool()) // are interpreter expects more data?
    val readNextData = Output(Bool()) // whether the next data should be read or not?

    val dataValidInput = Input(Bool()) // whether data on the receiving data line is valid or not?
    val receivingData = Input(UInt(bramDataWidth.W)) // data to be received in interpreter

    //
    // Sending singals
    //
    val beginSendingBuffer = Output(Bool()) // should sender start sending buffers or not?
    val noNewDataSender = Output(Bool()) // should sender finish sending buffers or not?
    val dataValidOutput = Output(Bool()) // should sender send next buffer or not?

    val sendWaitForBuffer = Input(Bool()) // should the interpreter send next buffer or not?

    val requestedActionOfThePacketOutput = Output(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W)) // the requested action
    val sendingData = Output(UInt(bramDataWidth.W)) // data to be sent to the debugger

  })

  //
  // State registers
  //
  val state = RegInit(sIdle)

  //
  // Last error register
  //
  val lastError = RegInit(0.U(bramDataWidth.W))

  //
  // Output pins
  //
  val noNewDataReceiver = WireInit(false.B)
  val readNextData = WireInit(false.B)

  val regBeginSendingBuffer = RegInit(false.B)
  val noNewDataSender = WireInit(false.B)
  val dataValidOutput = WireInit(false.B)
  val sendingData = WireInit(0.U(bramDataWidth.W))

  val regRequestedActionOfThePacketOutput = RegInit(0.U(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W))

  //
  // Apply the chip enable signal
  //
  when(io.en === true.B) {

    switch(state) {

      is(sIdle) {

        //
        // Check if the debugger need a new action (a new command is received)
        //
        when(io.requestedActionOfThePacketInputValid) {

          //
          // An action is received
          //
          state := sNewActionReceived

        }.otherwise {

          //
          // Remain at the same state (no action)
          //
          state := sIdle
        }

      }
      is(sNewActionReceived) {

        // -------------------------------------------------------------------------
        // Now, the action needs to be dispatched
        //
        val inputAction = io.requestedActionOfThePacketInput

        when(inputAction === HwdbgActionEnums.hwdbgActionSendVersion.id.U) {

          //
          // *** Configure sending version ***
          //

          //
          // Set the response packet type
          //
          regRequestedActionOfThePacketOutput := HwdbgResponseEnums.hwdbgResponseVersion.id.U

          //
          // This action needs a response
          //
          state := sSendResponse

        }.elsewhen(inputAction === HwdbgActionEnums.hwdbgActionSendPinInformation.id.U) {

          //
          // *** Configure sending pin information ***
          //

          //
          // Set the response packet type
          //
          regRequestedActionOfThePacketOutput := HwdbgResponseEnums.hwdbgResponsePinInformation.id.U

          //
          // This action needs a response
          //
          state := sSendResponse

        }.elsewhen(inputAction === HwdbgActionEnums.hwdbgActionConfigureScriptBuffer.id.U) {

          //
          // *** Configure the internal buffer with script ***
          //

          //
          // Set the response packet type
          //
          regRequestedActionOfThePacketOutput := HwdbgResponseEnums.hwdbgResponseScriptBufferConfigurationResult.id.U

          //
          // This action needs a response
          //
          state := sSendResponse

        }.otherwise {

          //
          // *** Invalid action ***
          //

          //
          // Set the response packet type
          //
          regRequestedActionOfThePacketOutput := HwdbgResponseEnums.hwdbgResponseInvalidPacketOrError.id.U

          //
          // Set the latest error
          //
          lastError := HwdbgErrorEnums.hwdbgErrorInvalidPacket.id.U

          //
          // This action needs a response
          //
          state := sSendResponse

        }

        //
        // -------------------------------------------------------------------------
        //

      }
      is(sSendResponse) {

        //
        // Finish the receiving
        //
        noNewDataReceiver := true.B

        //
        // Begin sending response
        //
        regBeginSendingBuffer := true.B

        //
        // Wait until the sender module is reading to send data
        //
        when(io.sendWaitForBuffer === true.B) {

          // -------------------------------------------------------------------------
          // Now, the response needs to be sent
          //
          when(regRequestedActionOfThePacketOutput === HwdbgResponseEnums.hwdbgResponseVersion.id.U) {

            //
            // *** Send version ***
            //

            //
            // Instantiate the version sender module
            //
            val (
              noNewDataSenderModule,
              dataValidOutputModule,
              sendingDataModule
            ) =
              InterpreterSendVersion(
                debug,
                bramDataWidth
              )(
                io.sendWaitForBuffer // send waiting for buffer as an activation signal to the module
              )

            //
            // Set data validity
            //
            dataValidOutput := dataValidOutputModule

            //
            // Set data
            //
            sendingData := sendingDataModule

            //
            // Once sending data is done, we'll go to the Done state
            //
            when(noNewDataSenderModule === true.B) {
              state := sDone
            }

          }.elsewhen(regRequestedActionOfThePacketOutput === HwdbgResponseEnums.hwdbgResponsePinInformation.id.U) {

            //
            // *** Send pin information ***
            //

            //
            // Instantiate the port information module
            //

            val (
              noNewDataSenderModule,
              dataValidOutputModule,
              sendingDataModule
            ) =
              InterpreterPortInformation(
                debug,
                bramDataWidth
              )(
                io.sendWaitForBuffer // send waiting for buffer as an activation signal to the module
              )

            //
            // Set data validity
            //
            dataValidOutput := dataValidOutputModule

            //
            // Set data
            //
            sendingData := sendingDataModule

            //
            // Once sending data is done, we'll go to the Done state
            //
            when(noNewDataSenderModule === true.B) {
              state := sDone
            }

          }.elsewhen(regRequestedActionOfThePacketOutput === HwdbgResponseEnums.hwdbgResponseScriptBufferConfigurationResult.id.U) {

            //
            // *** Send result of applying script ***
            //

            //
            // TODO: To be implemented
            //
            state := sDone

          }.otherwise {

            //
            // *** Invalid (packet) response ***
            // This will happen in case of 'HwdbgResponseEnums.hwdbgResponseInvalidPacketOrError'
            //

            //
            // Instantiate the invalid packet module
            //
            val (
              noNewDataSenderModule,
              dataValidOutputModule,
              sendingDataModule
            ) =
              InterpreterSendError(
                debug,
                bramDataWidth
              )(
                io.sendWaitForBuffer, // send waiting for buffer as an activation signal to the module
                lastError
              )

            //
            // Set data validity
            //
            dataValidOutput := dataValidOutputModule

            //
            // Set data
            //
            sendingData := sendingDataModule

            //
            // Once sending data is done, we'll go to the Done state
            //
            when(noNewDataSenderModule === true.B) {
              state := sDone
            }
          }

          //
          // -------------------------------------------------------------------------
          //

        }.otherwise {

          //
          // The sender module is not ready for sending buffer
          // so, we need to stay at this state
          //
          state := sSendResponse
        }
      }
      is(sDone) {

        //
        // Finish the receiving, sending communication
        //
        noNewDataReceiver := true.B
        noNewDataSender := true.B

        //
        // No longer need to begin sending data
        //
        regBeginSendingBuffer := false.B

        //
        // Go to the idle state
        //
        state := sIdle
      }
    }
  }

  // ---------------------------------------------------------------------

  //
  // Connect output pins
  //
  io.noNewDataReceiver := noNewDataReceiver
  io.readNextData := readNextData

  io.beginSendingBuffer := regBeginSendingBuffer
  io.noNewDataSender := noNewDataSender
  io.dataValidOutput := dataValidOutput
  io.requestedActionOfThePacketOutput := regRequestedActionOfThePacketOutput
  io.sendingData := sendingData

}

object DebuggerPacketInterpreter {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      bramAddrWidth: Int = DebuggerConfigurations.BLOCK_RAM_ADDR_WIDTH,
      bramDataWidth: Int = DebuggerConfigurations.BLOCK_RAM_DATA_WIDTH
  )(
      en: Bool,
      requestedActionOfThePacketInput: UInt,
      requestedActionOfThePacketInputValid: Bool,
      dataValidInput: Bool,
      receivingData: UInt,
      sendWaitForBuffer: Bool
  ): (Bool, Bool, Bool, Bool, Bool, UInt, UInt) = {

    val debuggerPacketInterpreter = Module(
      new DebuggerPacketInterpreter(
        debug,
        bramAddrWidth,
        bramDataWidth
      )
    )

    val noNewDataReceiver = Wire(Bool())
    val readNextData = Wire(Bool())

    val beginSendingBuffer = Wire(Bool())
    val noNewDataSender = Wire(Bool())
    val dataValidOutput = Wire(Bool())

    val requestedActionOfThePacketOutput = Wire(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W))
    val sendingData = Wire(UInt(bramDataWidth.W))

    //
    // Configure the input signals
    //
    debuggerPacketInterpreter.io.en := en

    //
    // Configure the input signals related to the receiving signals
    //
    debuggerPacketInterpreter.io.requestedActionOfThePacketInput := requestedActionOfThePacketInput
    debuggerPacketInterpreter.io.requestedActionOfThePacketInputValid := requestedActionOfThePacketInputValid
    debuggerPacketInterpreter.io.dataValidInput := dataValidInput
    debuggerPacketInterpreter.io.receivingData := receivingData

    //
    // Configure the input signals related to the sending signals
    //
    debuggerPacketInterpreter.io.sendWaitForBuffer := sendWaitForBuffer

    //
    // Configure the output signals
    //
    noNewDataReceiver := debuggerPacketInterpreter.io.noNewDataReceiver
    readNextData := debuggerPacketInterpreter.io.readNextData

    //
    // Configure the output signals related to sending packets
    //
    beginSendingBuffer := debuggerPacketInterpreter.io.beginSendingBuffer
    noNewDataSender := debuggerPacketInterpreter.io.noNewDataSender
    dataValidOutput := debuggerPacketInterpreter.io.dataValidOutput

    //
    // Configure the output signals related to received packets
    //
    requestedActionOfThePacketOutput := debuggerPacketInterpreter.io.requestedActionOfThePacketOutput
    sendingData := debuggerPacketInterpreter.io.sendingData

    //
    // Return the output result
    //
    (
      noNewDataReceiver,
      readNextData,
      beginSendingBuffer,
      noNewDataSender,
      dataValidOutput,
      requestedActionOfThePacketOutput,
      sendingData
    )
  }
}
