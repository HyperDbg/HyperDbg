/**
 * @file
 *   receiver.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Remote debugger packet receiver module
 * @details
 * @version 0.1
 * @date
 *   2024-04-08
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.communication

import chisel3._
import chisel3.util.{switch, is}
import circt.stage.ChiselStage

import hwdbg.configs._
import hwdbg.types._
import hwdbg.utils._
import hwdbg.constants._

object DebuggerPacketReceiverEnums {
  object State extends ChiselEnum {
    val sIdle, sReadChecksum, sReadIndicator, sReadTypeOfThePacket, sReadRequestedActionOfThePacket, sRequestedActionIsValid, sWaitToReadActionBuffer,
        sReadActionBuffer, sDone = Value
  }
}

class DebuggerPacketReceiver(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation
) extends Module {

  //
  // Import state enum
  //
  import DebuggerPacketReceiverEnums.State
  import DebuggerPacketReceiverEnums.State._

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Interrupt signals (lines)
    // Note: Only PL input signal is received here,
    // a separate module will control the PS signal
    //
    val plInSignal = Input(Bool()) // PS to PL signal

    //
    // BRAM (Block RAM) ports
    //
    val rdWrAddr = Output(UInt(instanceInfo.bramAddrWidth.W)) // read/write address
    val rdData = Input(UInt(instanceInfo.bramDataWidth.W)) // read data

    //
    // Receiving signals
    //
    val requestedActionOfThePacketOutput = Output(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W)) // the requested action
    val requestedActionOfThePacketOutputValid = Output(Bool()) // whether data on the requested action is valid or not
    val noNewDataReceiver = Input(Bool()) // receive done or not?

    // this contains and edge-detection mechanism, which means reader should make it low after reading the data
    val readNextData = Input(Bool()) // whether the next data should be read or not?

    val dataValidOutput = Output(Bool()) // whether data on the receiving data line is valid or not?
    val receivingData = Output(UInt(instanceInfo.bramDataWidth.W)) // data to be sent to the reader

    val finishedReceivingBuffer = Output(Bool()) // Receiving is done or not?

  })

  //
  // State registers
  //
  val state = RegInit(sIdle)

  //
  // Output pins
  //
  val rdWrAddr = WireInit(0.U(instanceInfo.bramAddrWidth.W))
  val regRdWrAddr = RegInit(0.U(instanceInfo.bramAddrWidth.W))
  val finishedReceivingBuffer = WireInit(false.B)
  val regRequestedActionOfThePacketOutput = RegInit(0.U(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W))
  val regRequestedActionOfThePacketOutputValid = RegInit(false.B)
  val regDataValidOutput = RegInit(false.B)
  val regReceivingData = RegInit(0.U(instanceInfo.bramDataWidth.W))

  //
  // Rising-edge detector for start receiving signal
  //
  val risingEdgePlInSignal = io.plInSignal & !RegNext(io.plInSignal)

  //
  // Rising-edge detector for reading next data signal
  //
  val risingEdgeReadNextData = io.readNextData & !RegNext(io.readNextData)

  //
  // Structure (as wire) of the received packet buffer
  //
  val receivedPacketBuffer = WireInit(0.U.asTypeOf(new DebuggerRemotePacket())) // here the wire is not used

  //
  // Apply the chip enable signal
  //
  when(io.en === true.B) {

    switch(state) {

      is(sIdle) {

        //
        // Create logs from communication structure offsets
        //
        LogInfo(debug)(f"The offset of Checksum is 0x${receivedPacketBuffer.Offset.checksum}%x")
        LogInfo(debug)(f"The offset of Indicator is 0x${receivedPacketBuffer.Offset.indicator}%x")
        LogInfo(debug)(f"The offset of TypeOfThePacket is 0x${receivedPacketBuffer.Offset.typeOfThePacket}%x")
        LogInfo(debug)(f"The offset of requestedActionOfThePacketOutput is 0x${receivedPacketBuffer.Offset.requestedActionOfThePacket}%x")

        //
        // Check whether the interrupt from the PS is received or not
        //
        when(risingEdgePlInSignal === true.B) {
          state := sReadChecksum
        }

        //
        // Configure the output pins in case of sIdle
        //
        rdWrAddr := 0.U
        regRequestedActionOfThePacketOutput := 0.U
        regRequestedActionOfThePacketOutputValid := false.B
        regDataValidOutput := false.B
        regReceivingData := 0.U
        finishedReceivingBuffer := false.B

      }
      is(sReadChecksum) {

        //
        // Adjust address to read Checksum from BRAM (Not Used)
        //
        rdWrAddr := (instanceInfo.debuggerAreaOffset + receivedPacketBuffer.Offset.checksum).U

        //
        // Goes to the next section
        //
        state := sReadIndicator

      }
      is(sReadIndicator) {

        //
        // Adjust address to read Indicator from BRAM
        //
        rdWrAddr := (instanceInfo.debuggerAreaOffset + receivedPacketBuffer.Offset.indicator).U

        //
        // Goes to the next section
        //
        state := sReadTypeOfThePacket

      }
      is(sReadTypeOfThePacket) {

        //
        // Adjust address to read TypeOfThePacket from BRAM
        //
        rdWrAddr := (instanceInfo.debuggerAreaOffset + receivedPacketBuffer.Offset.typeOfThePacket).U

        //
        // Check whether the indicator is valid or not
        //
        LogInfo(debug)(
          f"Comparing first 0x${BitwiseFunction.getBitsInRange(HyperDbgSharedConstants.INDICATOR_OF_HYPERDBG_PACKET, 32, (32 + instanceInfo.bramDataWidth - 1))}%x number of the indicator (little-endian)"
        )

        when(
          io.rdData === BitwiseFunction
            .getBitsInRange(HyperDbgSharedConstants.INDICATOR_OF_HYPERDBG_PACKET, 32, (32 + instanceInfo.bramDataWidth - 1))
            .U
        ) {

          //
          // Indicator of packet is valid
          // (Goes to the next section)
          //
          state := sReadRequestedActionOfThePacket

        }.otherwise {

          //
          // Indicator of packet is not valid
          // (Receiving was done but not found a valid packet,
          // so, go to the idle state)
          //
          state := sDone
        }
      }
      is(sReadRequestedActionOfThePacket) {

        //
        // Adjust address to read RequestedActionOfThePacket from BRAM
        //
        rdWrAddr := (instanceInfo.debuggerAreaOffset + receivedPacketBuffer.Offset.requestedActionOfThePacket).U

        //
        // Save the address into a register
        //
        regRdWrAddr := (instanceInfo.debuggerAreaOffset + receivedPacketBuffer.Offset.requestedActionOfThePacket + (instanceInfo.bramDataWidth >> 3)).U

        //
        // Check whether the type of the packet is valid or not
        //
        val packetType: DebuggerRemotePacketType.Value = DebuggerRemotePacketType.DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL
        LogInfo(debug)(
          f"Check packet type with DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL (0x${packetType.id}%x)"
        )
        when(io.rdData === packetType.id.U) {

          //
          // Type of packet is valid
          // (Goes to the next section)
          //
          state := sRequestedActionIsValid

        }.otherwise {

          //
          // Type of packet is not valid
          // (Receiving was done but not found a valid packet,
          // so, go to the idle state)
          //
          state := sDone
        }

      }
      is(sRequestedActionIsValid) {

        //
        // Read the RequestedActionOfThePacket
        //
        regRequestedActionOfThePacketOutput := io.rdData

        //
        // The RequestedActionOfThePacketOutput is valid from now
        //
        regRequestedActionOfThePacketOutputValid := true.B

        //
        // Goes to the next section
        //
        state := sWaitToReadActionBuffer

      }
      is(sWaitToReadActionBuffer) {

        //
        // The value of the received buffer is valid here, however,
        // in order to make difference when a new value is read, the
        // following signals goes to the off state
        //
        regDataValidOutput := false.B

        //
        // Check if the caller needs to read the next part of
        // the block RAM or the receiving data should be finished
        //
        when(io.noNewDataReceiver === true.B) {

          //
          // No new data, the receiving is done
          //
          state := sDone

        }.elsewhen(risingEdgeReadNextData === true.B) {

          //
          // Adjust address to read next data to BRAM
          //
          rdWrAddr := regRdWrAddr
          regRdWrAddr := regRdWrAddr + (instanceInfo.bramDataWidth >> 3).U

          //
          // Read the next offset of the buffer
          //
          state := sReadActionBuffer

        }.otherwise {

          //
          // Stay at the same state
          //
          state := sWaitToReadActionBuffer
        }

      }
      is(sReadActionBuffer) {

        //
        // Data outputs are now valid
        //
        regDataValidOutput := true.B

        //
        // Adjust the read buffer data
        //
        regReceivingData := io.rdData

        when(io.noNewDataReceiver === true.B) {

          //
          // No new data, the receiving is done
          //
          state := sDone

        }.otherwise {

          //
          // Return to the previous state of action
          //
          state := sWaitToReadActionBuffer

        }

      }
      is(sDone) {

        //
        // Reset the temporary address holder
        //
        regRdWrAddr := 0.U

        //
        // Requested action buffer and the receiving buffer is no longer valid
        //
        regRequestedActionOfThePacketOutput := 0.U
        regRequestedActionOfThePacketOutputValid := false.B
        regReceivingData := 0.U
        regDataValidOutput := false.B

        //
        // The receiving is done at this stage, either
        // was successful of unsucessful, we'll release the
        // sharing bram resource by indicating that the receiving
        // module is no longer using the bram line
        //
        finishedReceivingBuffer := true.B

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
  io.rdWrAddr := rdWrAddr
  io.requestedActionOfThePacketOutput := regRequestedActionOfThePacketOutput
  io.requestedActionOfThePacketOutputValid := regRequestedActionOfThePacketOutputValid
  io.dataValidOutput := regDataValidOutput
  io.receivingData := regReceivingData
  io.finishedReceivingBuffer := finishedReceivingBuffer

}

object DebuggerPacketReceiver {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      plInSignal: Bool,
      rdData: UInt,
      noNewDataReceiver: Bool,
      readNextData: Bool
  ): (UInt, UInt, Bool, Bool, UInt, Bool) = {

    val debuggerPacketReceiver = Module(
      new DebuggerPacketReceiver(
        debug,
        instanceInfo
      )
    )

    val rdWrAddr = Wire(UInt(instanceInfo.bramAddrWidth.W))
    val requestedActionOfThePacketOutput = Wire(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W))
    val requestedActionOfThePacketOutputValid = Wire(Bool())
    val dataValidOutput = Wire(Bool())
    val receivingData = Wire(UInt(instanceInfo.bramDataWidth.W))
    val finishedReceivingBuffer = Wire(Bool())

    //
    // Configure the input signals
    //
    debuggerPacketReceiver.io.en := en
    debuggerPacketReceiver.io.plInSignal := plInSignal
    debuggerPacketReceiver.io.rdData := rdData
    debuggerPacketReceiver.io.noNewDataReceiver := noNewDataReceiver
    debuggerPacketReceiver.io.readNextData := readNextData

    //
    // Configure the output signals
    //
    rdWrAddr := debuggerPacketReceiver.io.rdWrAddr

    //
    // Configure the output signals related to received packets
    //
    requestedActionOfThePacketOutput := debuggerPacketReceiver.io.requestedActionOfThePacketOutput
    requestedActionOfThePacketOutputValid := debuggerPacketReceiver.io.requestedActionOfThePacketOutputValid
    dataValidOutput := debuggerPacketReceiver.io.dataValidOutput
    receivingData := debuggerPacketReceiver.io.receivingData
    finishedReceivingBuffer := debuggerPacketReceiver.io.finishedReceivingBuffer

    //
    // Return the output result
    //
    (
      rdWrAddr,
      requestedActionOfThePacketOutput,
      requestedActionOfThePacketOutputValid,
      dataValidOutput,
      receivingData,
      finishedReceivingBuffer
    )
  }
}
