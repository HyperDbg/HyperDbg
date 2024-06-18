/**
 * @file
 *   send_receive_synchronizer.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Send and receive synchronizer module
 * @details
 * @version 0.1
 * @date
 *   2024-04-17
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

object SendReceiveSynchronizerEnums {
  object State extends ChiselEnum {
    val sIdle, sReceiver, sSender = Value
  }
}

class SendReceiveSynchronizer(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation
) extends Module {

  //
  // Import state enum
  //
  import SendReceiveSynchronizerEnums.State
  import SendReceiveSynchronizerEnums.State._

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Interrupt signals (lines)
    //
    val plInSignal = Input(Bool()) // PS to PL signal
    val psOutInterrupt = Output(Bool()) // PL to PS interrupt

    //
    // BRAM (Block RAM) ports
    //
    val rdWrAddr = Output(UInt(instanceInfo.bramAddrWidth.W)) // read/write address
    val rdData = Input(UInt(instanceInfo.bramDataWidth.W)) // read data
    val wrEna = Output(Bool()) // enable writing
    val wrData = Output(UInt(instanceInfo.bramDataWidth.W)) // write data

    //
    // Receiver ports
    //
    val requestedActionOfThePacketOutput = Output(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W)) // the requested action
    val requestedActionOfThePacketOutputValid = Output(Bool()) // whether data on the requested action is valid or not

    val noNewDataReceiver = Input(Bool()) // receive done or not?
    val readNextData = Input(Bool()) // whether the next data should be read or not?

    val dataValidOutput = Output(Bool()) // whether data on the receiving data line is valid or not?
    val receivingData = Output(UInt(instanceInfo.bramDataWidth.W)) // data to be sent to the reader

    //
    // Sender ports
    //
    val beginSendingBuffer = Input(Bool()) // should sender start sending buffers or not?
    val noNewDataSender = Input(Bool()) // should sender finish sending buffers or not?
    val dataValidInput = Input(Bool()) // should sender send next buffer or not?

    val sendWaitForBuffer = Output(Bool()) // should the external module send next buffer or not?

    val requestedActionOfThePacketInput = Input(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W)) // the requested action
    val sendingData = Input(UInt(instanceInfo.bramDataWidth.W)) // data to be sent to the debugger

  })

  //
  // State registers
  //
  val state = RegInit(sIdle)

  //
  // Saving state of the controlling pins
  //
  val regPlInSignal = RegInit(false.B)
  val regBeginSendingBuffer = RegInit(false.B)

  //
  // Shared BRAM pins
  //
  val sharedRdWrAddr = WireInit(0.U(instanceInfo.bramAddrWidth.W)) // read/write address
  val sharedRdData = WireInit(0.U(instanceInfo.bramDataWidth.W)) // read data
  val sharedWrEna = WireInit(false.B) // enable writing
  val sharedWrData = WireInit(0.U(instanceInfo.bramDataWidth.W)) // write data

  //
  // Instantiate the packet receiver module
  //
  val (
    receiverRdWrAddr,
    requestedActionOfThePacketOutput,
    requestedActionOfThePacketOutputValid,
    dataValidOutput,
    receivingData,
    finishedReceivingBuffer
  ) =
    DebuggerPacketReceiver(
      debug,
      instanceInfo
    )(
      io.en,
      regPlInSignal,
      io.rdData,
      io.noNewDataReceiver,
      io.readNextData
    )

  //
  // Instantiate the packet sender module
  //
  val (
    psOutInterrupt,
    senderRdWrAddr,
    wrEna,
    wrData,
    sendWaitForBuffer,
    finishedSendingBuffer
  ) =
    DebuggerPacketSender(
      debug,
      instanceInfo
    )(
      io.en,
      regBeginSendingBuffer,
      io.noNewDataSender,
      io.dataValidInput,
      io.requestedActionOfThePacketInput,
      io.sendingData
    )

  //
  // Apply the chip enable signal
  //
  when(io.en === true.B) {

    switch(state) {

      is(sIdle) {

        //
        // Peform the resource sepration of shared BRAM
        // and apply priority to receive over send
        //
        when(io.plInSignal === true.B) {

          //
          // Activate the receiver module
          //
          regPlInSignal := true.B

          //
          // Go to the receiver state
          //
          state := sReceiver

        }.elsewhen(io.beginSendingBuffer === true.B && io.plInSignal === false.B) {

          //
          // Activate the sender module
          //
          regBeginSendingBuffer := true.B

          //
          // Go to the sender state
          //
          state := sSender

        }.otherwise {

          //
          // Stay at the same state as there is no communication
          //
          state := sIdle

        }

      }
      is(sReceiver) {

        //
        // Check whether the receiving is finished
        //
        when(finishedReceivingBuffer === true.B) {

          //
          // No longer in the receiver state
          //
          regPlInSignal := false.B

          //
          // Go to the idle state
          //
          state := sIdle

        }.otherwise {

          //
          // Connect the address of BRAM reader to the receiver address
          //
          sharedRdWrAddr := receiverRdWrAddr

          //
          // On the receiver, writing is not allowed
          //
          sharedWrEna := false.B

          //
          // Stay at the same state
          //
          state := sReceiver

        }
      }
      is(sSender) {

        //
        // Check whether sending data is finished
        //
        when(finishedSendingBuffer === true.B) {

          //
          // No longer in the sender state
          //
          regBeginSendingBuffer := false.B

          //
          // Go to the idle state
          //
          state := sIdle

        }.otherwise {

          //
          // Connect shared BRAM signals to the sender
          //
          sharedRdWrAddr := senderRdWrAddr
          sharedWrEna := wrEna
          sharedWrData := wrData

          //
          // Stay at the same state
          //
          state := sSender

        }
      }
    }
  }

  // ---------------------------------------------------------------------

  //
  // Connect output pins (Interrupt)
  //
  io.psOutInterrupt := psOutInterrupt

  //
  // Connect output pins (BRAM)
  //
  io.rdWrAddr := sharedRdWrAddr
  io.wrEna := wrEna
  io.wrData := wrData

  //
  // Connect output pins (Receiver)
  //
  io.requestedActionOfThePacketOutput := requestedActionOfThePacketOutput
  io.requestedActionOfThePacketOutputValid := requestedActionOfThePacketOutputValid
  io.dataValidOutput := dataValidOutput
  io.receivingData := receivingData

  //
  // Connect output pins (Sender)
  //
  io.sendWaitForBuffer := sendWaitForBuffer

}

object SendReceiveSynchronizer {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      plInSignal: Bool,
      rdData: UInt,
      noNewDataReceiver: Bool,
      readNextData: Bool,
      beginSendingBuffer: Bool,
      noNewDataSender: Bool,
      dataValidInput: Bool,
      requestedActionOfThePacketInput: UInt,
      sendingData: UInt
  ): (Bool, UInt, Bool, UInt, UInt, Bool, Bool, UInt, Bool) = {

    val sendReceiveSynchronizerModule = Module(
      new SendReceiveSynchronizer(
        debug,
        instanceInfo
      )
    )

    val psOutInterrupt = Wire(Bool())

    val rdWrAddr = Wire(UInt(instanceInfo.bramAddrWidth.W))
    val wrEna = Wire(Bool())
    val wrData = Wire(UInt(instanceInfo.bramDataWidth.W))

    val requestedActionOfThePacketOutput = Wire(UInt(new DebuggerRemotePacket().RequestedActionOfThePacket.getWidth.W))
    val requestedActionOfThePacketOutputValid = Wire(Bool())
    val dataValidOutput = Wire(Bool())
    val receivingData = Wire(UInt(instanceInfo.bramDataWidth.W))

    val sendWaitForBuffer = Wire(Bool())

    //
    // Configure the input signals
    //
    sendReceiveSynchronizerModule.io.en := en
    sendReceiveSynchronizerModule.io.plInSignal := plInSignal
    sendReceiveSynchronizerModule.io.rdData := rdData
    sendReceiveSynchronizerModule.io.noNewDataReceiver := noNewDataReceiver
    sendReceiveSynchronizerModule.io.readNextData := readNextData
    sendReceiveSynchronizerModule.io.beginSendingBuffer := beginSendingBuffer
    sendReceiveSynchronizerModule.io.noNewDataSender := noNewDataSender
    sendReceiveSynchronizerModule.io.dataValidInput := dataValidInput
    sendReceiveSynchronizerModule.io.requestedActionOfThePacketInput := requestedActionOfThePacketInput
    sendReceiveSynchronizerModule.io.sendingData := sendingData

    //
    // Configure the output signals
    //
    psOutInterrupt := sendReceiveSynchronizerModule.io.psOutInterrupt
    rdWrAddr := sendReceiveSynchronizerModule.io.rdWrAddr
    wrEna := sendReceiveSynchronizerModule.io.wrEna
    wrData := sendReceiveSynchronizerModule.io.wrData

    requestedActionOfThePacketOutput := sendReceiveSynchronizerModule.io.requestedActionOfThePacketOutput
    requestedActionOfThePacketOutputValid := sendReceiveSynchronizerModule.io.requestedActionOfThePacketOutputValid
    dataValidOutput := sendReceiveSynchronizerModule.io.dataValidOutput
    receivingData := sendReceiveSynchronizerModule.io.receivingData

    sendWaitForBuffer := sendReceiveSynchronizerModule.io.sendWaitForBuffer

    //
    // Return the output result
    //
    (
      psOutInterrupt,
      rdWrAddr,
      wrEna,
      wrData,
      requestedActionOfThePacketOutput,
      requestedActionOfThePacketOutputValid,
      dataValidOutput,
      receivingData,
      sendWaitForBuffer
    )
  }
}
