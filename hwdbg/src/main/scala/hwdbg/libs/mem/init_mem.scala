/**
 * @file
 *   init_mem.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Initialize SRAM memory from a file
 * @details
 * @version 0.1
 * @date
 *   2024-04-03
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.libs.mem

import chisel3._
import chisel3.util.experimental.loadMemoryFromFileInline

import hwdbg.configs._

class InitMemInline(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
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

  val mem = SyncReadMem(size / width, UInt(width.W))

  //
  // Initialize memory
  //
  if (memoryFile.trim().nonEmpty) {
    loadMemoryFromFileInline(mem, memoryFile)
  }

  io.dataOut := DontCare

  when(io.enable) {
    val rdwrPort = mem(io.addr)
    when(io.write) {
      rdwrPort := io.dataIn
    }.otherwise {
      io.dataOut := rdwrPort
    }
  }
}

object InitMemInline {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
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

    val initMemInlineModule = Module(
      new InitMemInline(
        debug,
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
    initMemInlineModule.io.enable := enable
    initMemInlineModule.io.write := write
    initMemInlineModule.io.addr := addr
    initMemInlineModule.io.dataIn := dataIn

    //
    // Configure the output signals
    //
    dataOut := initMemInlineModule.io.dataOut

    //
    // Return the output result
    //
    dataOut
  }
}
