/**
 * @file
 *   sender.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Remote debugger packet sender module
 * @details
 * @version 0.1
 * @date
 *   2024-04-16
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.communication

import chisel3._
import chisel3.util.{switch, is, log2Ceil}
import circt.stage.ChiselStage

import hwdbg.configs._
import hwdbg.types._
import hwdbg.constants._

object DebuggerPacketSenderEnums {
  object State extends ChiselEnum {
    val sIdle, sWriteChecksum, sWriteIndicator, sWriteTypeOfThePacket, sWriteRequestedActionOfThePacket, sWaitToGetData, sSendData, sDone = Value
  }
}

class DebuggerPacketSender(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation
) extends Module {

  //
  // Import state enum
  //
  import DebuggerPacketSenderEnums.State
  import DebuggerPacketSenderEnums.State._

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Interrupt signals (lines)
    // Note: Only PS output signal is exported here,
    // a separate module will control the PL signal
    //
    val psOutInterrupt = Output(Bool()) // PL to PS interrupt

    //
    // BRAM (Block RAM) ports
    //
    val rdWrAddr = Output(UInt(instanceInfo.bramAddrWidth.W)) // read/write address
    val wrEna = Output(Bool()) // enable writing
    val wrData = Output(UInt(instanceInfo.bramDataWidth.W)) // write data

    //
    // Sending signals
    //
    val beginSendingBuffer = Input(Bool()) // should sender start sending buffers or not?
    val noNewDataSender = Input(Bool()) // should sender finish sending buffers or not?
    val dataValidInput = Input(Bool()) // should sender send next buffer or not?

    val sendWaitForBuffer = Output(Bool()) // should the external module send next buffer or not?
    val finishedSendingBuffer = Output(Bool()) // indicate that the sender finished sending buffers and ready to send next packet

    val requestedActionOfThePacketInput = Input(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W)) // the requested action
    val sendingData = Input(UInt(instanceInfo.bramDataWidth.W)) // data to be sent to the debugger

  })

  //
  // State registers
  //
  val state = RegInit(sIdle)

  //
  // Output pins
  //
  val psOutInterrupt = WireInit(false.B)
  val wrEna = WireInit(false.B)
  val wrData = WireInit(0.U(instanceInfo.bramDataWidth.W))
  val sendWaitForBuffer = WireInit(false.B)
  val finishedSendingBuffer = WireInit(false.B)
  val rdWrAddr = WireInit(0.U(instanceInfo.bramAddrWidth.W))

  //
  // Temporary address and data holder (register)
  //
  val regRdWrAddr = RegInit(0.U(instanceInfo.bramAddrWidth.W))
  val regDataToSend = RegInit(0.U(instanceInfo.bramDataWidth.W))

  //
  // Rising-edge detector for start sending signal
  //
  val risingEdgeBeginSendingBuffer = io.beginSendingBuffer & !RegNext(io.beginSendingBuffer)

  //
  // Keeping the state of whether sending data has been started or not
  // Means that if the sender is in the middle of sending the headers
  // of the packet or the actual data
  //
  val regIsSendingDataStarted = RegInit(false.B)

  //
  // Used to hold the transferred length of the indicator
  //
  val lengthOfIndicator: Int = new DebuggerRemotePacket().Indicator.getWidth
  val regTransferredIndicatorLength = RegInit(0.U((log2Ceil(lengthOfIndicator) + 1).W))

  //
  // Structure (as wire) of the received packet buffer
  //
  val sendingPacketBuffer = WireInit(0.U.asTypeOf(new DebuggerRemotePacket()))

  //
  // Apply the chip enable signal
  //
  when(io.en === true.B) {

    switch(state) {

      is(sIdle) {

        //
        // Check whether the interrupt from the PS is received or not
        //
        when(risingEdgeBeginSendingBuffer === true.B) {
          state := sWriteChecksum
        }

        //
        // Configure the outputs in case of sIdle
        //
        psOutInterrupt := false.B
        rdWrAddr := 0.U
        regRdWrAddr := 0.U
        wrEna := false.B
        wrData := 0.U
        sendWaitForBuffer := false.B
        finishedSendingBuffer := false.B

        //
        // Sending data has not been started
        //
        regIsSendingDataStarted := false.B

      }
      is(sWriteChecksum) {

        //
        // Enable writing to the BRAM
        //
        wrEna := true.B

        //
        // Adjust address to write Checksum to BRAM (Not Used)
        //
        rdWrAddr := (instanceInfo.debuggeeAreaOffset + sendingPacketBuffer.Offset.checksum).U

        //
        // Adjust data to write Checksum
        //
        wrData := 0.U // Checksum is ignored

        //
        // Reset the transferred bytes of the indicator
        //
        regTransferredIndicatorLength := 0.U

        //
        // Goes to the next section
        //
        state := sWriteIndicator
      }
      is(sWriteIndicator) {

        if (instanceInfo.bramDataWidth >= lengthOfIndicator) {

          //
          // Enable writing to the BRAM
          //
          wrEna := true.B

          //
          // Adjust address to write Indicator to BRAM
          //
          rdWrAddr := (instanceInfo.debuggeeAreaOffset + sendingPacketBuffer.Offset.indicator).U

          //
          // Adjust data to write Indicator
          //
          wrData := HyperDbgSharedConstants.INDICATOR_OF_HYPERDBG_PACKET.U

          //
          // Goes to the next section
          //
          state := sWriteTypeOfThePacket

        } else {

          //
          // Enable writing to the BRAM
          //
          wrEna := true.B

          //
          // Adjust address to write Indicator to BRAM (Address granularity is in the byte format so,
          // it'll be divided by 8 or shift to right by 3)
          //
          rdWrAddr := (instanceInfo.debuggeeAreaOffset + sendingPacketBuffer.Offset.indicator).U + (regTransferredIndicatorLength >> 3)

          //
          // Adjust data to write Indicator
          //
          wrData := HyperDbgSharedConstants.INDICATOR_OF_HYPERDBG_PACKET.U >> regTransferredIndicatorLength

          //
          // Add to the length transfered
          //
          regTransferredIndicatorLength := regTransferredIndicatorLength + instanceInfo.bramDataWidth.U

          when(regTransferredIndicatorLength >= lengthOfIndicator.U) {

            //
            // Disable writing to the BRAM
            //
            wrEna := false.B

            //
            // Goes to the next section
            //
            state := sWriteTypeOfThePacket

          }.otherwise {

            //
            // Stay at the same state
            //
            state := sWriteIndicator
          }
        }
      }
      is(sWriteTypeOfThePacket) {

        //
        // Enable writing to the BRAM
        //
        wrEna := true.B

        //
        // Adjust address to write type of packet to BRAM
        //
        rdWrAddr := (instanceInfo.debuggeeAreaOffset + sendingPacketBuffer.Offset.typeOfThePacket).U

        //
        // Adjust data to write type of packet
        //
        val packetType: DebuggerRemotePacketType.Value = DebuggerRemotePacketType.DEBUGGEE_TO_DEBUGGER_HARDWARE_LEVEL
        wrData := packetType.id.U

        //
        // Goes to the next section
        //
        state := sWriteRequestedActionOfThePacket

      }
      is(sWriteRequestedActionOfThePacket) {

        //
        // Enable writing to the BRAM
        //
        wrEna := true.B

        //
        // Adjust address to write requested action of packet to BRAM
        //
        rdWrAddr := (instanceInfo.debuggeeAreaOffset + sendingPacketBuffer.Offset.requestedActionOfThePacket).U

        //
        // Adjust data to write requested action of packet
        //
        wrData := io.requestedActionOfThePacketInput

        //
        // Goes to the next section
        //
        state := sWaitToGetData

      }
      is(sWaitToGetData) {

        //
        // Disable writing to the BRAM
        //
        wrEna := false.B

        //
        // Indicate that the module is waiting for data
        //
        sendWaitForBuffer := true.B

        //
        // Check whether sending actual data already started or not
        //
        when(regIsSendingDataStarted === false.B) {

          //
          // It's not yet started, so we adjust the address to the start
          // of the buffer after the last field of the header
          //
          regRdWrAddr := (instanceInfo.debuggeeAreaOffset + sendingPacketBuffer.Offset.startOfDataBuffer).U

          //
          // Indicate that sending data already started
          //
          regIsSendingDataStarted := true.B
        }

        //
        // Wait to receive the data or check whether sending was done at this state
        // (Two states will go us to the 'done' state)
        //
        when(io.noNewDataSender === true.B) {

          //
          // Sending data was done
          //
          state := sDone

        }.elsewhen(io.dataValidInput === true.B) {

          //
          // Store the data to send in a register
          //
          regDataToSend := io.sendingData

          //
          // The data is valid, so let's send it
          //
          state := sSendData

        }.otherwise {

          //
          // Stay in the same state as the data is not ready (valid)
          //
          state := sWaitToGetData
        }

      }
      is(sSendData) {

        //
        // Not waiting for the buffer at this state
        //
        sendWaitForBuffer := false.B

        //
        // Enable writing to the BRAM
        //
        wrEna := true.B

        //
        // Adjust address to write next data to BRAM (Address granularity is in the byte format so,
        // it'll be divided by 8 or shift to right by 3)
        //
        rdWrAddr := regRdWrAddr
        regRdWrAddr := regRdWrAddr + (instanceInfo.bramDataWidth >> 3).U

        //
        // Adjust data to write as the sending data
        //
        wrData := regDataToSend

        //
        // Check whether sending was done at this state (Two states will go us to the 'done' state)
        //
        when(io.noNewDataSender === true.B) {

          //
          // Sending data was done
          //
          state := sDone

        }.otherwise {

          //
          // Again go to the state for waiting for new data
          //
          state := sWaitToGetData

        }

      }
      is(sDone) {

        //
        // Adjust the output bits
        //
        finishedSendingBuffer := true.B

        //
        // Interrupt the PS
        //
        psOutInterrupt := true.B

        //
        // Go to the idle state
        //
        state := sIdle
      }
    }
  }

  // ---------------------------------------------------------------------

  //
  // Connect output pins to internal registers
  //
  io.psOutInterrupt := psOutInterrupt
  io.rdWrAddr := rdWrAddr
  io.wrEna := wrEna
  io.wrData := wrData
  io.sendWaitForBuffer := sendWaitForBuffer
  io.finishedSendingBuffer := finishedSendingBuffer

}

object DebuggerPacketSender {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      beginSendingBuffer: Bool,
      noNewDataSender: Bool,
      dataValidInput: Bool,
      requestedActionOfThePacketInput: UInt,
      sendingData: UInt
  ): (Bool, UInt, Bool, UInt, Bool, Bool) = {

    val debuggerPacketSender = Module(
      new DebuggerPacketSender(
        debug,
        instanceInfo
      )
    )

    val psOutInterrupt = Wire(Bool())
    val rdWrAddr = Wire(UInt(instanceInfo.bramAddrWidth.W))
    val wrEna = Wire(Bool())
    val wrData = Wire(UInt(instanceInfo.bramDataWidth.W))
    val sendWaitForBuffer = Wire(Bool())
    val finishedSendingBuffer = Wire(Bool())

    //
    // Configure the input signals
    //
    debuggerPacketSender.io.en := en
    debuggerPacketSender.io.beginSendingBuffer := beginSendingBuffer
    debuggerPacketSender.io.noNewDataSender := noNewDataSender
    debuggerPacketSender.io.dataValidInput := dataValidInput
    debuggerPacketSender.io.requestedActionOfThePacketInput := requestedActionOfThePacketInput
    debuggerPacketSender.io.sendingData := sendingData

    //
    // Configure the output signals
    //
    psOutInterrupt := debuggerPacketSender.io.psOutInterrupt
    rdWrAddr := debuggerPacketSender.io.rdWrAddr
    wrEna := debuggerPacketSender.io.wrEna
    wrData := debuggerPacketSender.io.wrData

    //
    // Configure the output signals related to sending packets
    //
    sendWaitForBuffer := debuggerPacketSender.io.sendWaitForBuffer
    finishedSendingBuffer := debuggerPacketSender.io.finishedSendingBuffer

    //
    // Return the output result
    //
    (psOutInterrupt, rdWrAddr, wrEna, wrData, sendWaitForBuffer, finishedSendingBuffer)
  }
}
