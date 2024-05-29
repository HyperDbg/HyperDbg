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

  //
  // Import script data types enum
  //
  import hwdbg.script.ScriptConstantTypes.ScriptDataTypes
  import hwdbg.script.ScriptConstantTypes.ScriptDataTypes._

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
  // Assign operator type (split the signal into only usable part)
  //
  LogInfo(debug)("Usable size of Type in the SYMBOL: " + ScriptDataTypes().getWidth)
  val mainOperatorType = io.operator.Type(ScriptDataTypes().getWidth - 1, 0).asTypeOf(ScriptDataTypes())

  //
  // *** Implementing the setting data logic ***
  //

  //
  // Apply the chip enable signal
  //
  when(io.en === true.B) {

    switch(mainOperatorType) {

      is(symbolGlobalIdType) {
        //
        // To be implemented
        //
       }
      is(symbolLocalIdType) { 
        //
        // To be implemented
        //
      }
      is(symbolTempType) { 
        //
        // To be implemented
        //
      }
      is(symbolRegisterType) { 
        //
        // To be implemented
        //
      }
      is(symbolStackTempType) { 
        //
        // To be implemented
        //
      }
      is(symbolFunctionParameterIdType) { 
        //
        // To be implemented
        //
      }
    }
  }

  //
  // Connect the output signals
  //
  for (i <- 0 until numberOfPins) {
    io.outputPin(i) := 0.U
  }

}

object ScriptEngineSetValue {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      numberOfPins: Int = DebuggerConfigurations.NUMBER_OF_PINS,
      scriptVariableLength: Int = ScriptEngineConfigurations.SCRIPT_VARIABLE_LENGTH,
      portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
  )(
      en: Bool,
      operator: Symbol,
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
