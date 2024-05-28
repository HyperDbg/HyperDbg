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
    numberOfPins: Int = DebuggerConfigurations.NUMBER_OF_PINS,
    maximumNumberOfStages: Int = ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_STAGES,
    maximumNumberOfSupportedScriptOperators: Int = ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_OPERATORS,
    bramAddrWidth: Int = DebuggerConfigurations.BLOCK_RAM_ADDR_WIDTH,
    bramDataWidth: Int = DebuggerConfigurations.BLOCK_RAM_DATA_WIDTH,
    portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
) extends Module {

  val io = IO(new Bundle {

    //
    // Chip signals
    //
    val en = Input(Bool()) // chip enable signal

    //
    // Input/Output signals
    //
    val inputPin = Input(Vec(numberOfPins, UInt(1.W))) // input pins
    val outputPin = Output(Vec(numberOfPins, UInt(1.W))) // output pins
  })

  //
  // Output pins
  //
  val outputPin = Wire(Vec(numberOfPins, UInt(1.W)))

  //
  // Stage registers
  //
  val stageRegs = Reg(Vec(maximumNumberOfStages, new StageRegisters(debug, numberOfPins, maximumNumberOfStages)))

  // -----------------------------------------------------------------------
  //
  // *** Move each register (input vector) to the next stage at each clock ***
  //
  for (i <- 0 until maximumNumberOfStages) {

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

    } else if (i == (maximumNumberOfStages - 1)) {

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
        // Create a Vec containing script symbol elements
        //
        val scriptSymbols = Wire(Vec(maximumNumberOfSupportedScriptOperators, new SYMBOL))

        for (j <- 0 until maximumNumberOfSupportedScriptOperators) {
          scriptSymbols(j) := stageRegs(i + j).scriptSymbol
        }

        //
        // Instantiate an eval engine for this stage
        //
        val (
          nextStage,
          outputPin
        ) = ScriptEngineEval(
          debug,
          numberOfPins,
          maximumNumberOfStages,
          maximumNumberOfSupportedScriptOperators,
          portsConfiguration
        )(
          io.en,
          scriptSymbols,
          stageRegs(i).targetStage,
          stageRegs(i).pinValues
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
      numberOfPins: Int = DebuggerConfigurations.NUMBER_OF_PINS,
      maximumNumberOfStages: Int = ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_STAGES,
      maximumNumberOfSupportedScriptOperators: Int = ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_OPERATORS,
      bramAddrWidth: Int = DebuggerConfigurations.BLOCK_RAM_ADDR_WIDTH,
      bramDataWidth: Int = DebuggerConfigurations.BLOCK_RAM_DATA_WIDTH,
      portsConfiguration: Map[Int, Int] = DebuggerPorts.PORT_PINS_MAP
  )(
      en: Bool,
      inputPin: Vec[UInt]
  ): (Vec[UInt]) = {

    val scriptExecutionEngineModule = Module(
      new ScriptExecutionEngine(
        debug,
        numberOfPins,
        maximumNumberOfStages,
        maximumNumberOfSupportedScriptOperators,
        bramAddrWidth,
        bramDataWidth,
        portsConfiguration
      )
    )

    val outputPin = Wire(Vec(numberOfPins, UInt(1.W)))

    //
    // Configure the input signals
    //
    scriptExecutionEngineModule.io.en := en
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
