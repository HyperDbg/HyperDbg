/**
 * @file
 *   get_value.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Script engine get value
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

class ScriptEngineGetValue(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    numberOfPins: Int,
    scriptVariableLength: Int,
    portsConfiguration: Map[Int, Int]
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
    // Input signals
    //
    val inputPin = Input(Vec(numberOfPins, UInt(1.W))) // input pins

    //
    // Output value
    //
    val outputValue = Output(UInt(scriptVariableLength.W)) // output value
  })

  val outputValue = WireInit(0.U(scriptVariableLength.W))

  //
  // Assign operator type (split the signal into only usable part)
  //
  LogInfo(debug)("Usable size of Type in the SYMBOL: " + ScriptDataTypes().getWidth)
  val mainOperatorType = io.operator.Type(ScriptDataTypes().getWidth - 1, 0).asTypeOf(ScriptDataTypes())

  //
  // *** Implementing the getting data logic ***
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
      is(symbolNumType) {
        
        outputValue := io.operator.Value
      }
      is(symbolRegisterType) {
        //
        // To be implemented
        //
      }
      is(symbolPseudoRegType) { 
        //
        // To be implemented
        //
      }
      is(symbolTempType) {
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
  io.outputValue := outputValue

}

object ScriptEngineGetValue {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      numberOfPins: Int = DebuggerConfigurations.NUMBER_OF_PINS,
      scriptVariableLength: Int = ScriptEngineConfigurations.SCRIPT_VARIABLE_LENGTH,
      portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
  )(
      en: Bool,
      operator: Symbol,
      inputPin: Vec[UInt]
  ): (UInt) = {

    val scriptEngineGetValueModule = Module(
      new ScriptEngineGetValue(
        debug,
        numberOfPins,
        scriptVariableLength,
        portsConfiguration
      )
    )

    val outputValue = Wire(UInt(scriptVariableLength.W))

    //
    // Configure the input signals
    //
    scriptEngineGetValueModule.io.en := en
    scriptEngineGetValueModule.io.operator := operator
    scriptEngineGetValueModule.io.inputPin := inputPin

    //
    // Configure the output signal
    //
    outputValue := scriptEngineGetValueModule.io.outputValue

    //
    // Return the output result
    //
    (outputValue)
  }
}
