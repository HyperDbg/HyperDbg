/**
 * @file
 *   exec.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Script execution engine
 * @details
 * @version 0.1
 * @date
 *   2024-05-07
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.script

import chisel3._
import chisel3.util._

import hwdbg.configs._
import hwdbg.stage._

class ScriptExecutionEngine(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation,
    bramAddrWidth: Int,
    bramDataWidth: Int
) extends Module {

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Script stage configuration signals
    //
    val moveToNextStage = Input(Bool()) // whether configuration finished configuring the current stage or not?
    val configureStage = Input(Bool()) // whether the configuration of stage should start or not?
    val targetOperator = Input(new HwdbgShortSymbol(instanceInfo.scriptVariableLength)) // Current operator to be configured

    //
    // Input/Output signals
    //
    val inputPin = Input(Vec(instanceInfo.numberOfPins, UInt(1.W))) // input pins
    val outputPin = Output(Vec(instanceInfo.numberOfPins, UInt(1.W))) // output pins
  })

  //
  // Output pins
  //
  val outputPin = Wire(Vec(instanceInfo.numberOfPins, UInt(1.W)))

  //
  // Stage registers
  //
  val stageRegs = Reg(Vec(instanceInfo.maximumNumberOfStages, new Stage(debug, instanceInfo)))

  //
  // Stage configuration registers
  //
  val configStageNumber = RegInit(0.U(log2Ceil(instanceInfo.maximumNumberOfStages).W))

  // -----------------------------------------------------------------------
  //
  // *** Configure stage buffers ***
  //
  when (io.configureStage === true.B) {

    when (io.moveToNextStage === true.B){
      //
      // Move to the configuration for the next stage
      //
      configStageNumber := configStageNumber + 1.U
    }.otherwise {
      //
      // Configure the current stage
      //
      stageRegs(configStageNumber).stageSymbol := io.targetOperator
    }
  }.otherwise {

    //
    // Not configuring anymore, reset the stage number
    //
    configStageNumber := 0.U
  }

  // -----------------------------------------------------------------------
  //
  // *** Move each register (input vector) to the next stage at each clock ***
  //
  for (i <- 0 until instanceInfo.maximumNumberOfStages) {

    if (i == 0) {

      //
      // At the first stage, the input registers should be passed to the
      // first registers set of the stage registers
      //
      stageRegs(i).pinValues := io.inputPin

      //
      // Each pin start initially start from 0th target stage
      //
      stageRegs(i).targetStage := 0.U

    } else if (i == (instanceInfo.maximumNumberOfStages - 1)) {

      //
      // At the last stage, the state registers should be passed to the output
      // Note: At this stage script symbol is useless
      //
      outputPin := stageRegs(i).pinValues

    } else {

      //
      // Check if this stage should be ignored (passed to the next stage) or be evaluated
      //
      when(i.U === stageRegs(i).targetStage) {

        //
        // *** Based on target stage, this stage needs evaluation ***
        //
        
        //
        // Instantiate an eval engine for this stage
        //
        val (
          nextStage,
          outputPin
        ) = ScriptEngineEval(
          debug,
          instanceInfo
        )(
          io.en,
          stageRegs(i)
        )

        //
        // At the normal (middle) stage, the result of state registers should be passed to
        // the next level of stage registers
        //
        stageRegs(i + 1).pinValues := outputPin

        //
        // Pass the target stage symbol number to the next stage
        //
        stageRegs(i + 1).targetStage := nextStage

      }.otherwise {

        //
        // *** Based on target stage, this stage should be ignore ***
        //

        //
        // Just pass all the values to the next stage
        //
        stageRegs(i + 1).pinValues := stageRegs(i).pinValues
        stageRegs(i + 1).targetStage := stageRegs(i).targetStage

      }
    }
  }

  // -----------------------------------------------------------------------

  //
  // Connect the output signals
  //
  io.outputPin := outputPin

}

object ScriptExecutionEngine {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation,
      bramAddrWidth: Int,
      bramDataWidth: Int
  )(
      en: Bool,
      moveToNextStage: Bool,
      configureStage: Bool,
      targetOperator: HwdbgShortSymbol,
      inputPin: Vec[UInt]
  ): (Vec[UInt]) = {

    val scriptExecutionEngineModule = Module(
      new ScriptExecutionEngine(
        debug,
        instanceInfo,
        bramAddrWidth,
        bramDataWidth
      )
    )

    val outputPin = Wire(Vec(instanceInfo.numberOfPins, UInt(1.W)))

    //
    // Configure the input signals
    //
    scriptExecutionEngineModule.io.en := en
    scriptExecutionEngineModule.io.moveToNextStage := moveToNextStage
    scriptExecutionEngineModule.io.configureStage := configureStage
    scriptExecutionEngineModule.io.targetOperator := targetOperator
    scriptExecutionEngineModule.io.inputPin := inputPin

    //
    // Configure the output signals
    //
    outputPin := scriptExecutionEngineModule.io.outputPin

    //
    // Return the output result
    //
    (outputPin)
  }
}
