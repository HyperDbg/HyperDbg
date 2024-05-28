/**
 * @file
 *   mux_4_to_1_onehot.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Implementation of MUX 4 to 1 (One Hot)
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

class Mux4To1OneHot(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    width: Int = 32
) extends Module {

  val io = IO(new Bundle {

    val in0 = Input(UInt(width.W))
    val in1 = Input(UInt(width.W))
    val in2 = Input(UInt(width.W))
    val in3 = Input(UInt(width.W))
    val sel = Input(UInt(log2Ceil(width).W))
    val out = Output(UInt(width.W))

  })

  io.out := Mux1H(io.sel, Seq(io.in0, io.in1, io.in2, io.in3))

}

object Mux4To1OneHot {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      width: Int = 32
  )(
      in0: UInt,
      in1: UInt,
      in2: UInt,
      in3: UInt,
      sel: UInt
  ): (UInt) = {

    val mux4To1OneHot = Module(
      new Mux4To1OneHot(
        debug
      )
    )

    val out = Wire(UInt(width.W))

    //
    // Configure the input signals
    //
    mux4To1OneHot.io.in0 := in0
    mux4To1OneHot.io.in1 := in1
    mux4To1OneHot.io.in2 := in2
    mux4To1OneHot.io.in3 := in3
    mux4To1OneHot.io.sel := sel

    //
    // Configure the output signals
    //
    out := mux4To1OneHot.io.out

    //
    // Return the output result
    //
    out
  }
}
