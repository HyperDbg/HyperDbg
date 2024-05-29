/**
 * @file
 *   set_value.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Script engine set value
 * @details
 * @version 0.1
 * @date
 *   2024-05-29
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.script

import chisel3._
import chisel3.util._

import hwdbg.configs._
import hwdbg.utils._
import hwdbg.stage._

class ScriptEngineSetValue(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    numberOfPins: Int = DebuggerConfigurations.NUMBER_OF_PINS,
    scriptVariableLength: Int = ScriptEngineConfigurations.SCRIPT_VARIABLE_LENGTH,
    portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
) extends Module {

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Evaluation operator symbol
    //
    val operator = Input(new Symbol)

    //
    // Input value
    //
    val inputValue = Input(UInt(scriptVariableLength.W)) // input value

    //
    // Input signals
    //
    val outputPin = Output(Vec(numberOfPins, UInt(1.W))) // output pins

  })

  //
  // Output pins
  //
  // val outputPin = Wire(Vec(numberOfPins, UInt(1.W)))


  //
  // Connect the output signals
  //
  // io.outputPin := outputPin
  //io.outputValue := 0.U

}

object ScriptEngineSetValue {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      numberOfPins: Int = DebuggerConfigurations.NUMBER_OF_PINS,
      scriptVariableLength: Int = ScriptEngineConfigurations.SCRIPT_VARIABLE_LENGTH,
      portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
  )(
      en: Bool,
      operator: Vec[Symbol],
      inputValue: UInt
  ): (Vec[UInt]) = {

    val scriptEngineSetValueModule = Module(
      new ScriptEngineSetValue(
        debug,
        numberOfPins,
        scriptVariableLength,
        portsConfiguration
      )
    )

    val outputPin = Wire(Vec(numberOfPins, UInt(1.W)))

    //
    // Configure the input signals
    //
    scriptEngineSetValueModule.io.en := en
    scriptEngineSetValueModule.io.operator := operator
    scriptEngineSetValueModule.io.inputValue := inputValue

    //
    // Configure the output signal
    //
    outputPin := scriptEngineSetValueModule.io.outputPin

    //
    // Return the output result
    //
    (outputPin)
  }
}
