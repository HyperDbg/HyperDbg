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

class ScriptEngineEval(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation
) extends Module {

  //
  // Import operators enum
  //
  import hwdbg.script.ScriptEvalFunc.ScriptOperators
  import hwdbg.script.ScriptEvalFunc.ScriptOperators._

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Evaluation operator symbol
    //
    val operators = Input(Vec(instanceInfo.maximumNumberOfSupportedScriptOperators, new Symbol))

    val currentStage = Input(UInt(log2Ceil(instanceInfo.maximumNumberOfStages).W))
    val nextStage = Output(UInt(log2Ceil(instanceInfo.maximumNumberOfStages).W))

    //
    // Input/Output signals
    //
    val inputPin = Input(Vec(instanceInfo.numberOfPins, UInt(1.W))) // input pins
    val outputPin = Output(Vec(instanceInfo.numberOfPins, UInt(1.W))) // output pins
  })

  //-------------------------------------------------------------------------
  // Get value module
  //
  val getValueModuleOutput = Wire(Vec(instanceInfo.maximumNumberOfSupportedScriptOperators - 1, UInt(instanceInfo.scriptVariableLength.W)))

  for (i <- 0 until instanceInfo.maximumNumberOfSupportedScriptOperators - 1) {

      getValueModuleOutput(i) := ScriptEngineGetValue(
        debug,
        instanceInfo
    )(
        io.en,
        io.operators(i),
        io.inputPin
    )
  }
  
  //-------------------------------------------------------------------------
  // Set value module
  //
  val setValueModuleInput = Wire(UInt(instanceInfo.scriptVariableLength.W)) // operators should be applied to this wire
  setValueModuleInput := 0. U ////////////////////// Test should be removed

  io.outputPin := ScriptEngineSetValue(
        debug,
        instanceInfo
    )(
        io.en,
        io.operators(instanceInfo.maximumNumberOfSupportedScriptOperators - 1), // last operator is for set value
        setValueModuleInput
    )

  //-------------------------------------------------------------------------
  // Output pins
  //
  // val outputPin = Wire(Vec(instanceInfo.numberOfPins, UInt(1.W)))
  val nextStage = WireInit(0.U(log2Ceil(instanceInfo.maximumNumberOfStages).W))

  //
  // Assign operator value (split the signal into only usable part)
  //
  LogInfo(debug)("Usable size of Value in the SYMBOL: " + ScriptOperators().getWidth)
  val mainOperatorValue = io.operators(0).Value(ScriptOperators().getWidth - 1, 0).asTypeOf(ScriptOperators())

  //-------------------------------------------------------------------------
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
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      operators: Vec[Symbol],
      currentStage: UInt,
      inputPin: Vec[UInt]
  ): (UInt, Vec[UInt]) = {

    val scriptEngineEvalModule = Module(
      new ScriptEngineEval(
        debug,
        instanceInfo
      )
    )

    val outputPin = Wire(Vec(instanceInfo.numberOfPins, UInt(1.W)))
    val nextStage = Wire(UInt(log2Ceil(instanceInfo.maximumNumberOfStages).W))

    //
    // Configure the input signals
    //
    scriptEngineEvalModule.io.en := en
    scriptEngineEvalModule.io.operators := operators
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
