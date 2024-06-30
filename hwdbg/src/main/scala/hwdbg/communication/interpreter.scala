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
import hwdbg.script._

object DebuggerPacketInterpreterEnums {
  object State extends ChiselEnum {
    val sIdle, sNewActionReceived, sSendResponse, sDone = Value
  }
}

class DebuggerPacketInterpreter(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation
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
    val receivingData = Input(UInt(instanceInfo.bramDataWidth.W)) // data to be received in interpreter

    //
    // Sending singals
    //
    val beginSendingBuffer = Output(Bool()) // should sender start sending buffers or not?
    val noNewDataSender = Output(Bool()) // should sender finish sending buffers or not?
    val dataValidOutput = Output(Bool()) // should sender send next buffer or not?

    val sendWaitForBuffer = Input(Bool()) // should the interpreter send next buffer or not?

    val requestedActionOfThePacketOutput = Output(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W)) // the requested action
    val sendingData = Output(UInt(instanceInfo.bramDataWidth.W)) // data to be sent to the debugger

    //
    // Script stage configuration signals
    //
    val finishedScriptConfiguration = Output(Bool()) // whether script configuration finished or not?
    val configureStage = Output(Bool()) // whether the configuration of stage should start or not?
    val targetOperator = Output(new HwdbgShortSymbol(instanceInfo.scriptVariableLength)) // Current operator to be configured
  })

  //
  // State registers
  //
  val state = RegInit(sIdle)

  //
  // Last error register
  //
  val lastSuccesOrErrorMessage = RegInit(0.U(instanceInfo.bramDataWidth.W))

  //
  // Last error register
  //
  val enablePinOfScriptBufferHandler = RegInit(false.B)

  //
  // Output pins
  //
  val noNewDataReceiver = WireInit(false.B)
  val readNextData = WireInit(false.B)

  val regBeginSendingBuffer = RegInit(false.B)
  val noNewDataSender = WireInit(false.B)
  val dataValidOutput = WireInit(false.B)
  val sendingData = WireInit(0.U(instanceInfo.bramDataWidth.W))

  val regRequestedActionOfThePacketOutput = RegInit(0.U(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W))

  val finishedScriptConfiguration = WireInit(false.B)
  val configureStage = WireInit(false.B)
  val initialSymbol = Wire(new HwdbgShortSymbol(instanceInfo.scriptVariableLength))
  initialSymbol.Type := 0.U
  initialSymbol.Value := 0.U
  val targetOperator = WireInit(initialSymbol)

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

        when(inputAction === HwdbgActionEnums.hwdbgActionSendInstanceInfo.id.U) {

          //
          // *** Configure sending instance info ***
          //

          //
          // Set the response packet type
          //
          regRequestedActionOfThePacketOutput := HwdbgResponseEnums.hwdbgResponseInstanceInfo.id.U

          //
          // This action needs a response
          //
          state := sSendResponse

        }.elsewhen(inputAction === HwdbgActionEnums.hwdbgActionConfigureScriptBuffer.id.U) {

          //
          // *** Configure the internal buffer with script ***
          //

          //
          // Enable the buffer config module
          //
          enablePinOfScriptBufferHandler := true.B

          val (
            moduleReadNextData,
            moduleFinishedScriptConfiguration,
            moduleConfigureStage,
            moduleTargetOperator
          ) =
            InterpreterScriptBufferHandler(
              debug,
              instanceInfo
            )(
              enablePinOfScriptBufferHandler,
              io.dataValidInput,
              io.receivingData
            )

          //
          // Connect the script stage configuration signals
          //
          readNextData := moduleReadNextData
          configureStage := moduleConfigureStage
          finishedScriptConfiguration := moduleFinishedScriptConfiguration
          targetOperator := moduleTargetOperator

          when(moduleFinishedScriptConfiguration === true.B) {

            //
            // *** Script stage buffer configuration finished! ***
            //

            //
            // Disable the buffer config module
            //
            enablePinOfScriptBufferHandler := false.B

            //
            // Set the response packet type
            //
            regRequestedActionOfThePacketOutput := HwdbgResponseEnums.hwdbgResponseSuccessOrErrorMessage.id.U

            //
            // Set the success message
            //
            lastSuccesOrErrorMessage := HwdbgSuccessOrErrorEnums.hwdbgOperationWasSuccessful.id.U

            //
            // This action needs a response
            //
            state := sSendResponse

          }.otherwise {

            //
            // *** Script stage buffer configuration NOT finished, read the buffer ***
            //

            //
            // Stay at the same state
            //
            state := sNewActionReceived
          }

        }.otherwise {

          //
          // *** Invalid action ***
          //

          //
          // Set the response packet type
          //
          regRequestedActionOfThePacketOutput := HwdbgResponseEnums.hwdbgResponseSuccessOrErrorMessage.id.U

          //
          // Set the latest error
          //
          lastSuccesOrErrorMessage := HwdbgSuccessOrErrorEnums.hwdbgErrorInvalidPacket.id.U

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
          when(regRequestedActionOfThePacketOutput === HwdbgResponseEnums.hwdbgResponseInstanceInfo.id.U) {

            //
            // *** Send instance information ***
            //

            //
            // Instantiate the instance info module
            //

            val (
              noNewDataSenderModule,
              dataValidOutputModule,
              sendingDataModule
            ) =
              InterpreterInstanceInfo(
                debug,
                instanceInfo
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

          }.elsewhen(regRequestedActionOfThePacketOutput === HwdbgResponseEnums.hwdbgResponseSuccessOrErrorMessage.id.U) {

            //
            // *** Send result of applying command (and errors) ***
            //

            //
            // Instantiate the invalid packet module
            //
            val (
              noNewDataSenderModule,
              dataValidOutputModule,
              sendingDataModule
            ) =
              InterpreterSendSuccessOrError(
                debug,
                instanceInfo
              )(
                io.sendWaitForBuffer, // send waiting for buffer as an activation signal to the module
                lastSuccesOrErrorMessage
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

  io.configureStage := configureStage
  io.finishedScriptConfiguration := finishedScriptConfiguration
  io.targetOperator := targetOperator
}

object DebuggerPacketInterpreter {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      requestedActionOfThePacketInput: UInt,
      requestedActionOfThePacketInputValid: Bool,
      dataValidInput: Bool,
      receivingData: UInt,
      sendWaitForBuffer: Bool
  ): (Bool, Bool, Bool, Bool, Bool, UInt, UInt, Bool, Bool, HwdbgShortSymbol) = {

    val debuggerPacketInterpreter = Module(
      new DebuggerPacketInterpreter(
        debug,
        instanceInfo
      )
    )

    val noNewDataReceiver = Wire(Bool())
    val readNextData = Wire(Bool())

    val beginSendingBuffer = Wire(Bool())
    val noNewDataSender = Wire(Bool())
    val dataValidOutput = Wire(Bool())

    val requestedActionOfThePacketOutput = Wire(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W))
    val sendingData = Wire(UInt(instanceInfo.bramDataWidth.W))

    val finishedScriptConfiguration = Wire(Bool())
    val configureStage = Wire(Bool())
    val targetOperator = Wire(new HwdbgShortSymbol(instanceInfo.scriptVariableLength))

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
    // Configure the output signals related to stage configuration
    //
    finishedScriptConfiguration := debuggerPacketInterpreter.io.finishedScriptConfiguration
    configureStage := debuggerPacketInterpreter.io.configureStage
    targetOperator := debuggerPacketInterpreter.io.targetOperator

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
      sendingData,
      finishedScriptConfiguration,
      configureStage,
      targetOperator
    )
  }
}
