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
    // Stage configuration signals
    //
    val stageConfig = Input(new Stage(debug, instanceInfo))
    val nextStage = Output(
      UInt(
        log2Ceil(
          instanceInfo.maximumNumberOfStages * (instanceInfo.maximumNumberOfSupportedGetScriptOperators + instanceInfo.maximumNumberOfSupportedSetScriptOperators + 1)
        ).W
      )
    )

    //
    // Output signals
    //
    val outputPin = Output(Vec(instanceInfo.numberOfPins, UInt(1.W))) // output pins
    val resultingLocalGlobalVariables = Output(
      Vec(instanceInfo.numberOfSupportedLocalAndGlobalVariables, UInt(instanceInfo.scriptVariableLength.W))
    ) // output of local (and global) variables
    val resultingTempVariables =
      Output(Vec(instanceInfo.numberOfSupportedTemporaryVariables, UInt(instanceInfo.scriptVariableLength.W))) // output of temporary variables
  })

  //
  // Output pins
  //
  val nextStage = WireInit(
    0.U(
      log2Ceil(
        instanceInfo.maximumNumberOfStages * (instanceInfo.maximumNumberOfSupportedGetScriptOperators + instanceInfo.maximumNumberOfSupportedSetScriptOperators + 1)
      ).W
    )
  )

  //
  // Assign operator value (split the signal into only usable part)
  //
  LogInfo(debug)("Usable size of Value in the SYMBOL: " + ScriptOperators().getWidth)
  val mainOperatorValue = io.stageConfig.stageSymbol.Value(ScriptOperators().getWidth - 1, 0).asTypeOf(ScriptOperators())

  // -------------------------------------------------------------------------
  // Get value module
  //
  val getValueModuleOutput = Wire(Vec(instanceInfo.maximumNumberOfSupportedGetScriptOperators, UInt(instanceInfo.scriptVariableLength.W)))

  for (i <- 0 until instanceInfo.maximumNumberOfSupportedGetScriptOperators) {

    getValueModuleOutput(i) := ScriptEngineGetValue(
      debug,
      instanceInfo
    )(
      io.en,
      io.stageConfig.getOperatorSymbol(i),
      io.stageConfig.localGlobalVariables,
      io.stageConfig.tempVariables,
      io.stageConfig.pinValues
    )
  }

  // -------------------------------------------------------------------------
  // *** Implementing the evaluation engine ***
  //
  //
  val srcVal = WireInit(VecInit(Seq.fill(instanceInfo.maximumNumberOfSupportedGetScriptOperators)(0.U(instanceInfo.scriptVariableLength.W))))
  val desVal = WireInit(VecInit(Seq.fill(instanceInfo.maximumNumberOfSupportedSetScriptOperators)(0.U(instanceInfo.scriptVariableLength.W))))

  //
  // Apply the chip enable signal
  //
  when(io.en === true.B) {

    switch(mainOperatorValue) {

      is(sFuncOr) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_or) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) | srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncXor) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_xor) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) ^ srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncAnd) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_and) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) & srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncAsr) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_asr) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) >> srcVal(1)(log2Ceil(instanceInfo.scriptVariableLength), 0)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncAsl) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_asl) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) << srcVal(1)(log2Ceil(instanceInfo.scriptVariableLength), 0)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncAdd) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_add) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) + srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncSub) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_sub) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) - srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncMul) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_mul) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) * srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncDiv) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_div) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) / srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncMod) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_mod) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) % srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncGt) {
        if (
          HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_gt) == true &&
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) > srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncLt) {
        if (
          HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_lt) == true &&
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) < srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncEgt) {
        if (
          HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_egt) == true &&
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) >= srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncElt) {
        if (
          HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_egt) == true &&
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) <= srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncEqual) {
        if (
          HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_equal) == true &&
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) === srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncNeq) {
        if (
          HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_neq) == true &&
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) =/= srcVal(1)

          nextStage := io.stageConfig.stageIndex + 4.U // one main operator + two GET operators + one SET operator
        }
      }
      is(sFuncJmp) {
        if (
          HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_jmp) == true &&
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {

          srcVal(0) := getValueModuleOutput(0)
          nextStage := srcVal(0)
        }
      }
      is(sFuncJz) {
        if (
          HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_jz) == true &&
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          when(srcVal(1) === 0.U) {
            nextStage := srcVal(0)
          }.otherwise {
            nextStage := io.stageConfig.stageIndex + 3.U // one main operator + two GET operators
          }
        }
      }
      is(sFuncJnz) {
        if (
          HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_jnz) == true &&
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          when(srcVal(1) =/= 0.U) {
            nextStage := srcVal(0)
          }.otherwise {
            nextStage := io.stageConfig.stageIndex + 3.U // one main operator + two GET operators
          }

        }
      }
      is(sFuncMov) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_mov) == true) {

          srcVal(0) := getValueModuleOutput(0)

          desVal(0) := srcVal(0)

          nextStage := io.stageConfig.stageIndex + 3.U // one main operator + one GET operators + one SET operator
        }
      }
      is(sFuncPrintf) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_printf) == true) {

          //
          // To be implemented
          //
        }
      }
    }
  }

  // -------------------------------------------------------------------------
  // Set value module
  //
  val setValueModuleInput = Wire(Vec(instanceInfo.maximumNumberOfSupportedSetScriptOperators, Vec(instanceInfo.numberOfPins, UInt(1.W))))
  val outputLocalGlobalVariables = Wire(
    Vec(
      instanceInfo.maximumNumberOfSupportedSetScriptOperators,
      Vec(instanceInfo.numberOfSupportedLocalAndGlobalVariables, UInt(instanceInfo.scriptVariableLength.W))
    )
  )
  val outputTempVariables = Wire(
    Vec(
      instanceInfo.maximumNumberOfSupportedSetScriptOperators,
      Vec(instanceInfo.numberOfSupportedTemporaryVariables, UInt(instanceInfo.scriptVariableLength.W))
    )
  )

  for (i <- 0 until instanceInfo.maximumNumberOfSupportedSetScriptOperators) {

    val (
      outputPin,
      resultingLocalGlobalVariables,
      resultingTempVariables
    ) = ScriptEngineSetValue(
      debug,
      instanceInfo
    )(
      io.en,
      io.stageConfig.setOperatorSymbol(i),
      io.stageConfig.localGlobalVariables,
      io.stageConfig.tempVariables,
      desVal(i),
      io.stageConfig.pinValues
    )

    //
    // Connect SET output pins
    //
    setValueModuleInput(i) := outputPin
    outputLocalGlobalVariables(i) := resultingLocalGlobalVariables
    outputTempVariables(i) := resultingTempVariables
  }

  // ---------------------------------------------------------------------

  //
  // Connect the output signals
  //
  io.outputPin := setValueModuleInput(0)
  io.resultingLocalGlobalVariables := outputLocalGlobalVariables(0)
  io.resultingTempVariables := outputTempVariables(0)
  io.nextStage := nextStage
}

object ScriptEngineEval {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      stageConfig: Stage
  ): (UInt, Vec[UInt], Vec[UInt], Vec[UInt]) = {

    val scriptEngineEvalModule = Module(
      new ScriptEngineEval(
        debug,
        instanceInfo
      )
    )

    val outputPin = Wire(Vec(instanceInfo.numberOfPins, UInt(1.W)))
    val resultingLocalGlobalVariables = Wire(Vec(instanceInfo.numberOfSupportedLocalAndGlobalVariables, UInt(instanceInfo.scriptVariableLength.W)))
    val resultingTempVariables = Wire(Vec(instanceInfo.numberOfSupportedTemporaryVariables, UInt(instanceInfo.scriptVariableLength.W)))
    val nextStage = Wire(
      UInt(
        log2Ceil(
          instanceInfo.maximumNumberOfStages * (instanceInfo.maximumNumberOfSupportedGetScriptOperators + instanceInfo.maximumNumberOfSupportedSetScriptOperators + 1)
        ).W
      )
    )

    //
    // Configure the input signals
    //
    scriptEngineEvalModule.io.en := en
    scriptEngineEvalModule.io.stageConfig := stageConfig

    //
    // Configure the output signals
    //
    nextStage := scriptEngineEvalModule.io.nextStage
    outputPin := scriptEngineEvalModule.io.outputPin
    resultingLocalGlobalVariables := scriptEngineEvalModule.io.resultingLocalGlobalVariables
    resultingTempVariables := scriptEngineEvalModule.io.resultingTempVariables

    //
    // Return the output result
    //
    (
      nextStage,
      outputPin,
      resultingLocalGlobalVariables,
      resultingTempVariables
    )
  }
}
