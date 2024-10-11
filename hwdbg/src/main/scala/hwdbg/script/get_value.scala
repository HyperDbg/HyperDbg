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
    instanceInfo: HwdbgInstanceInformation
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
    val operator = Input(new HwdbgShortSymbol(instanceInfo.scriptVariableLength))

    //
    // Input variables
    //
    val localGlobalVariables =
      Input(Vec(instanceInfo.numberOfSupportedLocalAndGlobalVariables, UInt(instanceInfo.scriptVariableLength.W))) // Local (and Global) variables
    val tempVariables = Input(Vec(instanceInfo.numberOfSupportedTemporaryVariables, UInt(instanceInfo.scriptVariableLength.W))) // Temporary variables

    //
    // Input signals
    //
    val inputPin = Input(Vec(instanceInfo.numberOfPins, UInt(1.W))) // input pins

    //
    // Output value
    //
    val outputValue = Output(UInt(instanceInfo.scriptVariableLength.W)) // output value
  })

  val outputValue = WireInit(0.U(instanceInfo.scriptVariableLength.W))

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

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.assign_local_global_var) == true) {
          //
          // Set output to local (and global) variables
          //
          outputValue := io.localGlobalVariables(io.operator.Value)
        }
      }
      is(symbolLocalIdType) {

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.assign_local_global_var) == true) {
          //
          // Set output to local (and global) variables
          //
          outputValue := io.localGlobalVariables(io.operator.Value)
        }
      }
      is(symbolNumType) {

        //
        // Constant value
        //
        outputValue := io.operator.Value
      }
      is(symbolRegisterType) {

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.assign_registers) == true) {

          //
          // Registers are pins and ports
          //
          when(instanceInfo.numberOfPins.U > io.operator.Value) {

            //
            // *** Used for getting the pin value ***
            //
            outputValue := io.inputPin(io.operator.Value)
          }.otherwise {

            //
            // *** Used for getting the port value ***
            //

            //
            // Create a vector of wires
            //
            val ports = Wire(Vec(instanceInfo.numberOfPorts, UInt(instanceInfo.scriptVariableLength.W)))
            var currentPortIndex: Int = 0
            var currentPortNum: Int = 0

            //
            // Iterate based on port configuration
            //
            for (port <- instanceInfo.portsConfiguration) {

              LogInfo(debug)(f"connect port(${currentPortIndex}) to inputPin(${currentPortNum} to ${currentPortNum + port}) for SET")
              ports(currentPortIndex) := io.inputPin.asUInt(currentPortNum + port - 1, currentPortNum)

              currentPortNum += port
              currentPortIndex += 1
            }

            //
            // Set the output
            //
            outputValue := ports(io.operator.Value - instanceInfo.numberOfPins.U)
          }
        }
      }
      is(symbolPseudoRegType) {

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.assign_pseudo_registers) == true) {
          //
          // To be implemented
          //
          outputValue := 0.U
        }
      }
      is(symbolStackIndexType) {

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.stack_assignments) == true) {
          //
          // To be implemented
          //
          outputValue := 0.U // io.inputPin.asUInt
        }
      }
      is(symbolTempType) {

        if (
          HwdbgScriptCapabilities.isCapabilitySupported(
            instanceInfo.scriptCapabilities,
            HwdbgScriptCapabilities.conditional_statements_and_comparison_operators
          ) == true
        ) {
          //
          // Set output to temporary variables
          //
          outputValue := io.tempVariables(io.operator.Value)
        }
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
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      operator: HwdbgShortSymbol,
      localGlobalVariables: Vec[UInt],
      tempVariables: Vec[UInt],
      inputPin: Vec[UInt]
  ): (UInt) = {

    val scriptEngineGetValueModule = Module(
      new ScriptEngineGetValue(
        debug,
        instanceInfo
      )
    )

    val outputValue = Wire(UInt(instanceInfo.scriptVariableLength.W))

    //
    // Configure the input signals
    //
    scriptEngineGetValueModule.io.en := en
    scriptEngineGetValueModule.io.operator := operator
    scriptEngineGetValueModule.io.localGlobalVariables := localGlobalVariables
    scriptEngineGetValueModule.io.tempVariables := tempVariables
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
