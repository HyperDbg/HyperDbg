/**
 * @file
 *   mux_2_to_1_io.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Implementation of MUX 2 to 1 (I/O)
 * @details
 * @version 0.1
 * @date
 *   2024-05-05
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.libs.mux

import chisel3._

import hwdbg.configs._

class Mux2To1IO(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG
) extends Module {

  val io = IO(new Bundle {

    val a = Input(Bool())
    val b = Input(Bool())
    val select = Input(Bool())
    val out = Output(Bool())

  })
  io.out := io.a & io.select | io.b & (~io.select)

}

object Mux2To1IO {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG
  )(
      a: Bool,
      b: Bool,
      select: Bool
  ): (Bool) = {

    val mux2To1IO = Module(
      new Mux2To1IO(
        debug
      )
    )

    val out = Wire(Bool())

    //
    // Configure the input signals
    //
    mux2To1IO.io.a := a
    mux2To1IO.io.b := b
    mux2To1IO.io.select := select

    //
    // Configure the output signals
    //
    out := mux2To1IO.io.out

    //
    // Return the output result
    //
    out
  }
}
