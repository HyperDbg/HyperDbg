/**
 * @file
 *   stage.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Data types related to stage registers
 * @details
 * @version 0.1
 * @date
 *   2024-05-07
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.stage

import chisel3._
import chisel3.util.log2Ceil

import hwdbg.configs._
import hwdbg.script._

class Stage(
    debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
    instanceInfo: HwdbgInstanceInformation
) extends Bundle {

  val pinValues = Vec(
    instanceInfo.numberOfPins,
    UInt(1.W)
  ) // The value of each pin in each stage (should be passed to the next stage)

  val stageSymbol = new HwdbgShortSymbol(
    instanceInfo.scriptVariableLength
  ) // Interpreted script symbol for the target stage (should NOT be passed to the next stage)

  val getOperatorSymbol = Vec(
    instanceInfo.maximumNumberOfSupportedGetScriptOperators,
    new HwdbgShortSymbol(instanceInfo.scriptVariableLength)
  ) // GET symbol operand (should NOT be passed to the next stage)

  val setOperatorSymbol = Vec(
    instanceInfo.maximumNumberOfSupportedSetScriptOperators,
    new HwdbgShortSymbol(instanceInfo.scriptVariableLength)
  ) // SET symbol operand (should NOT be passed to the next stage)

  val targetStage = UInt(
    log2Ceil(
      instanceInfo.maximumNumberOfStages * (instanceInfo.maximumNumberOfSupportedGetScriptOperators + instanceInfo.maximumNumberOfSupportedSetScriptOperators + 1)
    ).W
  ) // Target stage that needs to be executed for the current pin values (should be passed to the next stage)

  val tempVariables = Vec(
    instanceInfo.numberOfSupportedTemporaryVariables,
    UInt(instanceInfo.scriptVariableLength.W)
  ) // Temporary variables

  val localGlobalVariables = Vec(
    instanceInfo.numberOfSupportedLocalAndGlobalVariables,
    UInt(instanceInfo.scriptVariableLength.W)
  ) // Local (and Global) variables

  val stageIndex = UInt(
    log2Ceil(
      instanceInfo.maximumNumberOfStages * (instanceInfo.maximumNumberOfSupportedGetScriptOperators + instanceInfo.maximumNumberOfSupportedSetScriptOperators + 1)
    ).W
  ) // Target stage index of the current stage (configured with script configuration and remains constant during execution)

  val stageEnable = Bool() // Target stage is enabled (configured) or not
}
