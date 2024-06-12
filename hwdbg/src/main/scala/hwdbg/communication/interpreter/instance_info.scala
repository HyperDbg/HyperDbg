/**
 * @file
 *   port_information.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Send port information (in interpreter)
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
import chisel3.util.{switch, is, log2Ceil}
import circt.stage.ChiselStage

import hwdbg.configs._
import hwdbg.utils._

object InterpreterInstanceInfoEnums {
  object State extends ChiselEnum {
    val sIdle, sSendVersion, sSendMaximumNumberOfStages, sSendScriptVariableLength, 
    sSendMaximumNumberOfSupportedScriptOperators, sSendDebuggerAreaOffset, sSendDebuggeeAreaOffset, 
    sSendNumberOfPins, sSendNumberOfPorts, sSendScriptCapabilities1, sSendScriptCapabilities2, 
    sSendPortsConfiguration, sDone = Value
  }
}

class InterpreterInstanceInfo(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation,
    bramDataWidth: Int
) extends Module {

  //
  // Import state enum
  //
  import InterpreterInstanceInfoEnums.State
  import InterpreterInstanceInfoEnums.State._

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Sending singals
    //
    val noNewDataSender = Output(Bool()) // should sender finish sending buffers or not?
    val dataValidOutput = Output(Bool()) // should sender send next buffer or not?
    val sendingData = Output(UInt(bramDataWidth.W)) // data to be sent to the debugger

  })

  //
  // State registers
  //
  val state = RegInit(sIdle)

  //
  // Get number of input/output ports
  //
  val numberOfPorts = instanceInfo.portsConfiguration.size

  //
  // Convert input port pins into vector
  //
  // val pinsVec = RegInit(VecInit(Seq.fill(numberOfPorts)(0.U(bramDataWidth.W))))
  val pinsVec = VecInit(instanceInfo.portsConfiguration.map(_.U))

  //
  // Determine the width for numberOfSentPins
  //
  val numberOfSentPinsWidth = log2Ceil(numberOfPorts)

  //
  // Registers for keeping track of sent pin details
  //
  val numberOfSentPins = RegInit(0.U(numberOfSentPinsWidth.W))

  //
  // Output pins
  //
  val noNewDataSender = WireInit(false.B)
  val dataValidOutput = WireInit(false.B)
  val sendingData = WireInit(0.U(bramDataWidth.W))

  //
  // Apply the chip enable signal
  //
  when(io.en === true.B) {

    switch(state) {

      is(sIdle) {

        //
        // Going to the next state (sending the version of the debugger)
        //
        state := sSendVersion
      }
      is(sSendVersion) {

        //
        // Set the version
        //
        sendingData := instanceInfo.version.U

        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendMaximumNumberOfStages

      }
      is(sSendMaximumNumberOfStages) {

        //
        // Set the maximum number of stages supported by this instance of the debugger
        //
        sendingData := instanceInfo.maximumNumberOfStages.U

        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendScriptVariableLength

      }
      is(sSendScriptVariableLength) {

        //
        // Set the script variable length of this instance of the debugger
        //
        sendingData := instanceInfo.scriptVariableLength.U

        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendMaximumNumberOfSupportedScriptOperators

      }
      is(sSendMaximumNumberOfSupportedScriptOperators) {

        //
        // Set the maximum number of supported operators by this instance of the 
        // debugger in the script engine
        //
        sendingData := instanceInfo.maximumNumberOfSupportedScriptOperators.U

        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendDebuggerAreaOffset

      }
      is(sSendDebuggerAreaOffset) {

        //
        // Set the start offset of the debugger to the debuggee memory for this
        // instance of the debugger
        //
        sendingData := instanceInfo.debuggerAreaOffset.U

        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendDebuggeeAreaOffset

      }
      is(sSendDebuggeeAreaOffset) {

        //
        // Set the start offset of the debuggee to the debugger memory for this
        // instance of the debugger
        //
        sendingData := instanceInfo.debuggeeAreaOffset.U

        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendNumberOfPins

      }
      is(sSendNumberOfPins) {

        //
        // Set the number of pins in this instance of the debugger
        //
        sendingData := instanceInfo.numberOfPins.U

        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendNumberOfPorts

      }
      is(sSendNumberOfPorts) {

        //
        // Set the number of ports in this instance of the debugger
        //
        sendingData := instanceInfo.numberOfPorts.U

        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendScriptCapabilities1

      }
      is(sSendScriptCapabilities1) {

        //
        // Set the first bits (most significant) of the supported operators capabilities of this instance 
        // of the debugger
        //
        sendingData := BitwiseFunction.getBitsInRange(instanceInfo.scriptCapabilities, bramDataWidth, bramDataWidth + bramDataWidth - 1).U


        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendScriptCapabilities2

      }
      is(sSendScriptCapabilities2) {

        //
        // Set the second bits (least significant) of the supported operators capabilities of this instance 
        // of the debugger
        //
        sendingData := BitwiseFunction.getBitsInRange(instanceInfo.scriptCapabilities, 0, bramDataWidth - 1).U

        //
        // The output is valid
        //
        dataValidOutput := true.B

        state := sSendPortsConfiguration

      }
      is(sSendPortsConfiguration) {

        //
        // Send input port items
        //

        //
        // Adjust data
        //
        sendingData := pinsVec(numberOfSentPins)

        //
        // Data is valid
        //
        dataValidOutput := true.B

        when(numberOfSentPins === (numberOfPorts - 1).U) {

          //
          // Reset the pins sent for sending details
          //
          numberOfSentPins := 0.U

          state := sDone

        }.otherwise {

          //
          // Send next index
          //
          numberOfSentPins := numberOfSentPins + 1.U

          //
          // Stay at the same state
          //
          state := sSendPortsConfiguration
        }
      }
      is(sDone) {

        //
        // Indicate that sending data is done
        //
        noNewDataSender := true.B

        //
        // Goto the idle state
        //
        state := sIdle
      }
    }

  }

  // ---------------------------------------------------------------------

  //
  // Connect output pins
  //
  io.noNewDataSender := noNewDataSender
  io.dataValidOutput := dataValidOutput
  io.sendingData := sendingData

}

object InterpreterInstanceInfo {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation,
      bramDataWidth: Int,
  )(
      en: Bool
  ): (Bool, Bool, UInt) = {

    val interpreterInstanceInfo = Module(
      new InterpreterInstanceInfo(
        debug,
        instanceInfo,
        bramDataWidth
      )
    )

    val noNewDataSender = Wire(Bool())
    val dataValidOutput = Wire(Bool())
    val sendingData = Wire(UInt(bramDataWidth.W))

    //
    // Configure the input signals
    //
    interpreterInstanceInfo.io.en := en

    //
    // Configure the output signals
    //
    noNewDataSender := interpreterInstanceInfo.io.noNewDataSender
    dataValidOutput := interpreterInstanceInfo.io.dataValidOutput
    sendingData := interpreterInstanceInfo.io.sendingData

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
