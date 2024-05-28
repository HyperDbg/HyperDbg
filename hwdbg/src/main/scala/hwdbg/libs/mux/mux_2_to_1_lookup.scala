/**
 * @file
 *   mux_2_to_1_lookup.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Implementation of MUX 2 to 1 (Mux-Lookup)
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
import chisel3.util._

import hwdbg.configs._

class Mux2To1Lookup(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG
) extends Module {

  val io = IO(new Bundle {

    val a = Input(Bool())
    val b = Input(Bool())
    val select = Input(Bool())
    val out = Output(Bool())

  })

  val inputs = Array(
    false.B -> io.a,
    true.B -> io.b
  )

  io.out := MuxLookup(io.select, io.a)(inputs)

}

object Mux2To1Lookup {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG
  )(
      a: Bool,
      b: Bool,
      select: Bool
  ): (Bool) = {

    val mux2To1Lookup = Module(
      new Mux2To1Lookup(
        debug
      )
    )

    val out = Wire(Bool())

    //
    // Configure the input signals
    //
    mux2To1Lookup.io.a := a
    mux2To1Lookup.io.b := b
    mux2To1Lookup.io.select := select

    //
    // Configure the output signals
    //
    out := mux2To1Lookup.io.out

    //
    // Return the output result
    //
    out
  }
}
