/**
 * @file
 *   eval.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Script execution engine
 * @details
 * @version 0.1
 * @date
 *   2024-05-17
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

object ScriptEvalFunc {
  object ScriptOperators extends ChiselEnum {
    val sFuncInc, sFuncDec, sFuncReference, sFuncDereference, sFuncOr, sFuncXor, sFuncAnd, sFuncAsr, sFuncAsl, sFuncAdd, sFuncSub, sFuncMul, sFuncDiv, sFuncMod, sFuncGt, sFuncLt, sFuncEgt, sFuncElt, sFuncEqual, sFuncNeq, sFuncStart_of_if, sFuncJmp, sFuncJz, sFuncJnz, sFuncJmp_to_end_and_jzcompleted, sFuncEnd_of_if, sFuncStart_of_while, sFuncEnd_of_while, sFuncVargstart, sFuncMov, sFuncStart_of_do_while, sFunc, sFuncStart_of_do_while_commands, sFuncEnd_of_do_while, sFuncStart_of_for, sFuncFor_inc_dec, sFuncStart_of_for_ommands, sFuncIgnore_lvalue, sFuncEnd_of_user_defined_function, sFuncReturn_of_user_defined_function_with_value, sFuncReturn_of_user_defined_function_without_value, sFuncCall_user_defined_function_parameter, sFuncEnd_of_calling_user_defined_function_without_returning_value, sFuncEnd_of_calling_user_defined_function_with_returning_value, sFuncCall_user_defined_function, sFuncStart_of_user_defined_function, sFuncMov_return_value, sFuncVoid, sFuncBool, sFuncChar, sFuncShort, sFuncInt, sFuncLong, sFuncUnsigned, sFuncSigned, sFuncFloat, sFuncDouble, sFuncPrint, sFuncFormats, sFuncEvent_enable, sFuncEvent_disable, sFuncEvent_clear, sFuncTest_statement, sFuncSpinlock_lock, sFuncSpinlock_unlock, sFuncEvent_sc, sFuncPrintf, sFuncPause, sFuncFlush, sFuncEvent_trace_step, sFuncEvent_trace_step_in, sFuncEvent_trace_step_out, sFuncEvent_trace_instrumentation_step, sFuncEvent_trace_instrumentation_step_in, sFuncSpinlock_lock_custom_wait, sFuncEvent_inject, sFuncPoi, sFuncDb, sFuncDd, sFuncDw, sFuncDq, sFuncNeg, sFuncHi, sFuncLow, sFuncNot, sFuncCheck_address, sFuncDisassemble_len, sFuncDisassemble_len32, sFuncDisassemble_len64, sFuncInterlocked_increment, sFuncInterlocked_decrement, sFuncPhysical_to_virtual, sFuncVirtual_to_physical, sFuncEd, sFuncEb, sFuncEq, sFuncInterlocked_exchange, sFuncInterlocked_exchange_add, sFuncInterlocked_compare_exchange, sFuncStrlen, sFuncStrcmp, sFuncMemcmp, sFuncWcslen, sFuncWcscmp, sFuncEvent_inject_error_code, sFuncMemcpy = Value
  }
} 

class ScriptEngineEval(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    numberOfPins: Int = DebuggerConfigurations.NUMBER_OF_PINS,
    maximumNumberOfStages: Int = ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_STAGES,
    maximumNumberOfSupportedScriptOperators: Int = ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_OPERATORS,
    portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
) extends Module {

  //
  // Import state enum
  //
  import ScriptEvalFunc.ScriptOperators
  import ScriptEvalFunc.ScriptOperators._

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Evaluation operator symbol
    //
    val operator = Input(Vec(maximumNumberOfSupportedScriptOperators, new SYMBOL))

    val currentStage = Input(UInt(log2Ceil(maximumNumberOfStages).W))
    val nextStage = Output(UInt(log2Ceil(maximumNumberOfStages).W))

    //
    // Input/Output signals
    //
    val inputPin = Input(Vec(numberOfPins, UInt(1.W))) // input pins
    val outputPin = Output(Vec(numberOfPins, UInt(1.W))) // output pins
  })

  //
  // Output pins
  //
  // val outputPin = Wire(Vec(numberOfPins, UInt(1.W)))
  val nextStage = WireInit(0.U(log2Ceil(maximumNumberOfStages).W))

  //
  // Assign operator value (split the signal into only usable part)
  //
  LogInfo(debug)("Usable size of Value in the SYMBOL: " + ScriptOperators().getWidth)
  val mainOperatorValue = io.operator(0).Value(ScriptOperators().getWidth - 1, 0).asTypeOf(ScriptOperators())

  //
  // *** Implementing the evaluation engine ***
  //

  //
  // Apply the chip enable signal
  //
  when(io.en === true.B) {

    switch(mainOperatorValue) {

      is(sFuncInc) {
        nextStage := io.currentStage + 1.U
      }
      is(sFuncDec) {
        nextStage := io.currentStage + 2.U
      }
    }
  }

  // ---------------------------------------------------------------------

  //
  // Increment the stage
  //
  // nextStage := io.currentStage + 1.U

  //
  // Connect the output signals
  //
  io.nextStage := nextStage
  // io.outputPin := outputPin
  io.outputPin := io.inputPin

}

object ScriptEngineEval {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      numberOfPins: Int = DebuggerConfigurations.NUMBER_OF_PINS,
      maximumNumberOfStages: Int = ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_STAGES,
      maximumNumberOfSupportedScriptOperators: Int = ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_OPERATORS,
      portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
  )(
      en: Bool,
      operator: Vec[SYMBOL],
      currentStage: UInt,
      inputPin: Vec[UInt]
  ): (UInt, Vec[UInt]) = {

    val scriptEngineEvalModule = Module(
      new ScriptEngineEval(
        debug,
        numberOfPins,
        maximumNumberOfStages,
        maximumNumberOfSupportedScriptOperators,
        portsConfiguration
      )
    )

    val outputPin = Wire(Vec(numberOfPins, UInt(1.W)))
    val nextStage = Wire(UInt(log2Ceil(maximumNumberOfStages).W))

    //
    // Configure the input signals
    //
    scriptEngineEvalModule.io.en := en
    scriptEngineEvalModule.io.operator := operator
    scriptEngineEvalModule.io.currentStage := currentStage
    scriptEngineEvalModule.io.inputPin := inputPin

    //
    // Configure the output signals
    //
    nextStage := scriptEngineEvalModule.io.nextStage
    outputPin := scriptEngineEvalModule.io.outputPin

    //
    // Return the output result
    //
    (nextStage, outputPin)
  }
}
