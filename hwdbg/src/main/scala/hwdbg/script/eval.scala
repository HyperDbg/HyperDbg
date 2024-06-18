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
    val nextStage = Output(UInt(log2Ceil(instanceInfo.maximumNumberOfStages).W))

    //
    // Output signals
    //
    val outputPin = Output(Vec(instanceInfo.numberOfPins, UInt(1.W))) // output pins
  })  
 

  //-------------------------------------------------------------------------
  // Output pins
  //
  val nextStage = WireInit(0.U(log2Ceil(instanceInfo.maximumNumberOfStages).W))

  //
  // Assign operator value (split the signal into only usable part)
  //
  LogInfo(debug)("Usable size of Value in the SYMBOL: " + ScriptOperators().getWidth)
  val mainOperatorValue = io.stageConfig.stageSymbol.Value(ScriptOperators().getWidth - 1, 0).asTypeOf(ScriptOperators())

  //-------------------------------------------------------------------------
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
        io.stageConfig.pinValues
    )
  }

  //-------------------------------------------------------------------------
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
        }        
      }
      is(sFuncXor) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_xor) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) ^ srcVal(1)
        }
      }
      is(sFuncAsl) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_asl) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) << srcVal(1)(log2Ceil(instanceInfo.scriptVariableLength), 0)
        }   
      }
      is(sFuncAdd) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_add) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) + srcVal(1)
        }
      }
      is(sFuncSub) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_sub) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) - srcVal(1)
        }
      }
      is(sFuncMul) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_mul) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) * srcVal(1)
        }
      }
      is(sFuncDiv) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_div) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) / srcVal(1)
        }
      }
      is(sFuncGt) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_gt) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) > srcVal(1)
        }
      }
      is(sFuncEgt) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_egt) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) >= srcVal(1)
        }        
      }
      is(sFuncElt) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_egt) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) <= srcVal(1)
        }         
      }
      is(sFuncEqual) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_equal) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) === srcVal(1)
        }        
      }
      is(sFuncNeq) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_neq) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)

          desVal(0) := srcVal(0) =/= srcVal(1)
        }         
      }
      is(sFuncJmp) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_jmp) == true) {

          srcVal(0) := getValueModuleOutput(0)

          nextStage := srcVal(0)
        }           
      }
      is(sFuncJz) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_jz) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)
          
          when (srcVal(1) === 0.U) { nextStage := srcVal(0) }
            
        }         
      }
      is(sFuncJnz) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_jnz) == true) {

          srcVal(0) := getValueModuleOutput(0)
          srcVal(1) := getValueModuleOutput(1)
          
          when (srcVal(1) =/= 0.U) { nextStage := srcVal(0) }
            
        }         
      }
      is(sFuncMov) {
        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.func_mov) == true) {

          srcVal(0) := getValueModuleOutput(0)

          desVal(0) := srcVal(0)
        }         
      }
    }
  }

  //-------------------------------------------------------------------------
  // Set value module
  //
  val setValueModuleInput = Wire(Vec(instanceInfo.maximumNumberOfSupportedSetScriptOperators, Vec(instanceInfo.numberOfPins, UInt(1.W))))

  for (i <- 0 until instanceInfo.maximumNumberOfSupportedSetScriptOperators) {

  setValueModuleInput(i) := ScriptEngineSetValue(
        debug,
        instanceInfo
    )(
        io.en,
        io.stageConfig.setOperatorSymbol(i),
        desVal(i),
        io.stageConfig.pinValues
    )
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
  io.outputPin := setValueModuleInput(0)

}

object ScriptEngineEval {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      stageConfig: Stage
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
    scriptEngineEvalModule.io.stageConfig := stageConfig

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
