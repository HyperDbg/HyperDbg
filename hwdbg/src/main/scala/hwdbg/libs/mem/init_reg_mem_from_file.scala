/**
 * @file
 *   init_reg_mem_from_file.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Initialize registers from a file
 * @details
 * @version 0.1
 * @date
 *   2024-04-14
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.libs.mem

import scala.collection.mutable.ArrayBuffer
import scala.io.Source

import chisel3._

import hwdbg.utils._
import hwdbg.configs._

object InitRegMemFromFileTools {
  def readmemh(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      path: String,
      width: Int
  ): Seq[UInt] = {

    var counter: Int = 0
    val buffer = new ArrayBuffer[UInt]
    for (line <- Source.fromFile(path).getLines()) {

      val tokens: Array[String] = line.split("(//)").map(_.trim)

      if (tokens.nonEmpty && tokens.head != "" && tokens.head.split(";")(0).trim != "") {

        val i = Integer.parseInt(tokens.head.split(";")(0).trim, 16)

        LogInfo(debug)(
          f"Initialize memory [${counter}%x]: 0x${i}%x"
        )

        counter = counter + 4
        buffer.append(i.U(width.W))
      }
    }

    buffer.toSeq

  }
}

class InitRegMemFromFile(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    emulateBlockRamDelay: Boolean,
    memoryFile: String,
    addrWidth: Int,
    width: Int,
    size: Int
) extends Module {

  val io = IO(new Bundle {
    val enable = Input(Bool())
    val write = Input(Bool())
    val addr = Input(UInt(addrWidth.W))
    val dataIn = Input(UInt(width.W))
    val dataOut = Output(UInt(width.W))
  })

  //
  // Not needed to show the BRAM information
  //
  val mem = RegInit(VecInit(InitRegMemFromFileTools.readmemh(false, memoryFile, width)))

  val actualAddr = Wire(UInt(addrWidth.W))
  val actualData = Wire(UInt(width.W))
  val actualWrite = Wire(Bool())

  //
  // This because the address of the saved registers are using 4 bytes granularities
  // E.g., 4 Rsh 2 = 1 | 8 Rsh 2 = 2 | 12 Rsh 2 = 3
  //
  if (emulateBlockRamDelay) {
    //
    // In case, if it is an emulation of BRAM, a one clock delay is injected
    //
    actualAddr := RegNext(io.addr >> 2)
    actualData := RegNext(io.dataIn)
    actualWrite := RegNext(io.write)
  } else {
    actualAddr := io.addr >> 2
    actualData := io.dataIn
    actualWrite := io.write
  }

  when(io.enable) {
    val rdwrPort = mem(actualAddr)
    io.dataOut := rdwrPort

    when(actualWrite) {
      mem(actualAddr) := actualData
    }
  }.otherwise {
    io.dataOut := 0.U
  }
}

object InitRegMemFromFile {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      emulateBlockRamDelay: Boolean,
      memoryFile: String,
      addrWidth: Int,
      width: Int,
      size: Int
  )(
      enable: Bool,
      write: Bool,
      addr: UInt,
      dataIn: UInt
  ): UInt = {

    val initRegMemFromFileModule = Module(
      new InitRegMemFromFile(
        debug,
        emulateBlockRamDelay,
        memoryFile,
        addrWidth,
        width,
        size
      )
    )

    val dataOut = Wire(UInt(width.W))

    //
    // Configure the input signals
    //
    initRegMemFromFileModule.io.enable := enable
    initRegMemFromFileModule.io.write := write
    initRegMemFromFileModule.io.addr := addr
    initRegMemFromFileModule.io.dataIn := dataIn

    //
    // Configure the output signals
    //
    dataOut := initRegMemFromFileModule.io.dataOut

    //
    // Return the output result
    //
    dataOut
  }
}
