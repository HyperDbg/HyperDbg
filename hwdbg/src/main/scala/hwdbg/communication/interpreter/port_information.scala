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

import hwdbg.version._
import hwdbg.configs._
import hwdbg.utils._

object InterpreterPortInformationEnums {
  object State extends ChiselEnum {
    val sIdle, sSendCountOfPorts, sSendPortItems, sDone = Value
  }
}

class InterpreterPortInformation(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    bramDataWidth: Int = DebuggerConfigurations.BLOCK_RAM_DATA_WIDTH,
    portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
) extends Module {

  //
  // Import state enum
  //
  import InterpreterPortInformationEnums.State
  import InterpreterPortInformationEnums.State._

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
  val numberOfPorts = portsConfiguration.size

  //
  // Convert input port pins into vector
  //
  // val pinsVec = VecInit(portsConfiguration.values.toSeq.map(_.U))
  val pinsVec = RegInit(VecInit(Seq.fill(numberOfPorts)(0.U(bramDataWidth.W))))

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
        // Going to the next state (sending count of input ports)
        //
        state := sSendCountOfPorts
      }
      is(sSendCountOfPorts) {

        //
        // Send count of input.output ports
        //
        LogInfo(debug)("Number of ports (PORT_PINS_MAP): " + numberOfPorts)

        sendingData := numberOfPorts.U

        //
        // Data is valid
        //
        dataValidOutput := true.B

        //
        // Fill the port info
        //
        LogInfo(debug)("Iterating over input pins:")

        portsConfiguration.foreach { case (port, pins) =>
          LogInfo(debug)(s"Port $port has $pins pins")
          pinsVec(port) := pins.U
        }

        //
        // Going to the next state (sending count of input ports)
        //
        state := sSendPortItems

      }
      is(sSendPortItems) {

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
          state := sSendPortItems
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

object InterpreterPortInformation {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      bramDataWidth: Int = DebuggerConfigurations.BLOCK_RAM_DATA_WIDTH,
      portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
  )(
      en: Bool
  ): (Bool, Bool, UInt) = {

    val interpreterPortInformation = Module(
      new InterpreterPortInformation(
        debug,
        bramDataWidth,
        portsConfiguration
      )
    )

    val noNewDataSender = Wire(Bool())
    val dataValidOutput = Wire(Bool())
    val sendingData = Wire(UInt(bramDataWidth.W))

    //
    // Configure the input signals
    //
    interpreterPortInformation.io.en := en

    //
    // Configure the output signals
    //
    noNewDataSender := interpreterPortInformation.io.noNewDataSender
    dataValidOutput := interpreterPortInformation.io.dataValidOutput
    sendingData := interpreterPortInformation.io.sendingData

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
