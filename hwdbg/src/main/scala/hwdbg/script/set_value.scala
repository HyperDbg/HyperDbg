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
    val inputLocalGlobalVariables =
      Input(Vec(instanceInfo.numberOfSupportedLocalAndGlobalVariables, UInt(instanceInfo.scriptVariableLength.W))) // Local (and Global) variables
    val inputTempVariables =
      Input(Vec(instanceInfo.numberOfSupportedTemporaryVariables, UInt(instanceInfo.scriptVariableLength.W))) // Temporary variables

    //
    // Input value
    //
    val inputValue = Input(UInt(instanceInfo.scriptVariableLength.W)) // input value

    //
    // Output signals
    //
    val inputPin = Input(Vec(instanceInfo.numberOfPins, UInt(1.W))) // output pins
    val outputPin = Output(Vec(instanceInfo.numberOfPins, UInt(1.W))) // output pins

    //
    // Output variables
    //
    val outputLocalGlobalVariables =
      Output(Vec(instanceInfo.numberOfSupportedLocalAndGlobalVariables, UInt(instanceInfo.scriptVariableLength.W))) // Local (and Global) variables
    val outputTempVariables =
      Output(Vec(instanceInfo.numberOfSupportedTemporaryVariables, UInt(instanceInfo.scriptVariableLength.W))) // Temporary variables

  })

  //
  // Temp input
  //
  val inputLocalGlobalVariables = io.inputLocalGlobalVariables
  val inputTempVariables = io.inputTempVariables
  val inputPin = io.inputPin.asUInt

  //
  // Output pins
  //
  val outputPin = WireInit(0.U(instanceInfo.numberOfPins.W))
  val outputLocalGlobalVariables = WireInit(
    VecInit(Seq.fill(instanceInfo.numberOfSupportedLocalAndGlobalVariables)(0.U(instanceInfo.scriptVariableLength.W)))
  ) // Local (and Global) variables
  val outputTempVariables = WireInit(
    VecInit(Seq.fill(instanceInfo.numberOfSupportedTemporaryVariables)(0.U(instanceInfo.scriptVariableLength.W)))
  ) // Temporary variables

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

      is(symbolUndefined) {

        //
        // In case of undefined SET value, just pass every input to the next step
        //
        outputPin := inputPin
        outputLocalGlobalVariables := inputLocalGlobalVariables
        outputTempVariables := inputTempVariables

      }
      is(symbolGlobalIdType) {

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.assign_local_global_var) == true) {

          //
          // Set the local (and global) variables
          //
          outputPin := inputPin
          outputLocalGlobalVariables := inputLocalGlobalVariables
          outputTempVariables := inputTempVariables

          //
          // Set the target variable
          //
          outputLocalGlobalVariables(io.operator.Value) := io.inputValue
        }

      }
      is(symbolLocalIdType) {

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.assign_local_global_var) == true) {
          //
          // Set the target local/global variable
          //
          outputPin := inputPin
          outputLocalGlobalVariables := inputLocalGlobalVariables
          outputTempVariables := inputTempVariables

          //
          // Set the target local/global variable
          //
          outputLocalGlobalVariables(io.operator.Value) := io.inputValue
        }

      }
      is(symbolRegisterType) {

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.assign_registers) == true) {

          when(instanceInfo.numberOfPins.U > io.operator.Value) {

            //
            // *** Used for setting the pin value ***
            //

            //
            // Registers are pins (set the value based on less significant bit)
            //
            val tempShiftedBit = (1.U << io.operator.Value)

            when(io.inputValue(0) === 1.U) {
              outputPin := inputPin | tempShiftedBit; // Set the N-th bit to 1
            }.otherwise {
              outputPin := inputPin & ~tempShiftedBit; // Clear the N-th bit to 0
            }

          }.otherwise {

            //
            // *** Used for setting the port value ***
            //

            //
            // Iterate based on port configuration
            //
            var currentPortNum: Int = 0
            val numPorts = instanceInfo.portsConfiguration.length

            for ((port, index) <- instanceInfo.portsConfiguration.zipWithIndex) {

              LogInfo(debug)(f"========================= port assignment (${index} - port size: ${port}) =========================")

              when(io.operator.Value === (index + instanceInfo.numberOfPins).U) {

                //
                // If the current port's bit width is bigger than the script variable length,
                // we need to append zero
                //
                val targetInputValue = WireInit(0.U(port.W))

                if (port > instanceInfo.scriptVariableLength) {

                  //
                  // Since the port size is bigger than the variable size,
                  // we need to append zeros to the target value
                  //
                  LogInfo(debug)(
                    f"Appending zeros (${port - instanceInfo.scriptVariableLength}) to input variable (targetInputValue) to support port num: ${index}"
                  )
                  targetInputValue := Cat(io.inputValue, 0.U((port - instanceInfo.scriptVariableLength).W))
                } else {
                  //
                  // Since the variable size is bigger than the port size,
                  // we need only a portion of the input value
                  //
                  targetInputValue := io.inputValue(port - 1, 0)
                }

                //
                // Determine the range of bits to be modified
                //
                val high = currentPortNum + port - 1
                val low = currentPortNum

                // Create the modified outputPin based on whether it's the first, last or a middle port
                val modifiedOutputPin = if (index == 0) {

                  //
                  // First port: keep higher bits unchanged, set lower bits to input value
                  //
                  LogInfo(debug)(
                    f"Set connecting index=${index} - inputPin(${instanceInfo.numberOfPins - 1}, ${high + 1}) + targetInputValue(${port - 1}, 0)"
                  )
                  Cat(
                    inputPin(instanceInfo.numberOfPins - 1, high + 1), // Bits above the range to keep unchanged
                    targetInputValue // New value for the specified range
                  )
                } else if (index == numPorts - 1) {

                  //
                  // Last port: keep lower bits unchanged, set higher bits to input value
                  //
                  LogInfo(debug)(f"Set connecting index=${index} - targetInputValue(${port - 1}, 0) + inputPin(${low - 1}, 0)")
                  Cat(
                    targetInputValue, // New value for the specified range
                    inputPin(low - 1, 0) // Bits below the range to keep unchanged
                  )
                } else {

                  //
                  // Middle port: keep both higher and lower bits unchanged
                  //
                  LogInfo(debug)(
                    f"Set connecting index=${index} - inputPin(${instanceInfo.numberOfPins - 1}, ${high + 1}) + targetInputValue(${port - 1}, 0) + inputPin(${low - 1}, 0)"
                  )
                  Cat(
                    inputPin(instanceInfo.numberOfPins - 1, high + 1), // Bits above the range to keep unchanged
                    targetInputValue, // New value for the specified range
                    inputPin(low - 1, 0) // Bits below the range to keep unchanged
                  )
                }

                //
                // Assign the modified outputPin back to outputPin
                //
                outputPin := modifiedOutputPin
              }

              currentPortNum += port
            }
          }

          outputLocalGlobalVariables := inputLocalGlobalVariables
          outputTempVariables := inputTempVariables
        }
      }
      is(symbolPseudoRegType) {

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.assign_pseudo_registers) == true) {
          //
          // To be implemented
          //
          outputPin := 0.U
          outputLocalGlobalVariables := inputLocalGlobalVariables
          outputTempVariables := inputTempVariables
        }

      }
      is(symbolStackIndexType) {

        if (HwdbgScriptCapabilities.isCapabilitySupported(instanceInfo.scriptCapabilities, HwdbgScriptCapabilities.stack_assignments) == true) {
          //
          // To be implemented
          //
          outputPin := inputPin
          outputLocalGlobalVariables := inputLocalGlobalVariables
          outputTempVariables := inputTempVariables
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
          // Set the temporary variables
          //
          outputPin := inputPin
          outputLocalGlobalVariables := inputLocalGlobalVariables
          outputTempVariables := inputTempVariables

          //
          // Set the target temporary variable
          //
          outputTempVariables(io.operator.Value) := io.inputValue
        }
      }
    }
  }

  //
  // Connect the output signals
  //
  io.outputLocalGlobalVariables := outputLocalGlobalVariables
  io.outputTempVariables := outputTempVariables

  for (i <- 0 until instanceInfo.numberOfPins) {
    io.outputPin(i) := outputPin(i)
  }

}

object ScriptEngineSetValue {

  def apply(
      debug: Boolean = DebuggerConfigurations.ENABLE_DEBUG,
      instanceInfo: HwdbgInstanceInformation
  )(
      en: Bool,
      operator: HwdbgShortSymbol,
      inputLocalGlobalVariables: Vec[UInt],
      inputTempVariables: Vec[UInt],
      inputValue: UInt,
      inputPin: Vec[UInt]
  ): (Vec[UInt], Vec[UInt], Vec[UInt]) = {

    val scriptEngineSetValueModule = Module(
      new ScriptEngineSetValue(
        debug,
        instanceInfo
      )
    )

    val outputPin = Wire(Vec(instanceInfo.numberOfPins, UInt(1.W)))
    val outputLocalGlobalVariables = Wire(Vec(instanceInfo.numberOfSupportedLocalAndGlobalVariables, UInt(instanceInfo.scriptVariableLength.W)))
    val outputTempVariables = Wire(Vec(instanceInfo.numberOfSupportedTemporaryVariables, UInt(instanceInfo.scriptVariableLength.W)))

    //
    // Configure the input signals
    //
    scriptEngineSetValueModule.io.en := en
    scriptEngineSetValueModule.io.operator := operator
    scriptEngineSetValueModule.io.inputLocalGlobalVariables := inputLocalGlobalVariables
    scriptEngineSetValueModule.io.inputTempVariables := inputTempVariables
    scriptEngineSetValueModule.io.inputValue := inputValue
    scriptEngineSetValueModule.io.inputPin := inputPin

    //
    // Configure the output signal
    //
    outputPin := scriptEngineSetValueModule.io.outputPin
    outputLocalGlobalVariables := scriptEngineSetValueModule.io.outputLocalGlobalVariables
    outputTempVariables := scriptEngineSetValueModule.io.outputTempVariables

    //
    // Return the output result
    //
    (
      outputPin,
      outputLocalGlobalVariables,
      outputTempVariables
    )
  }
}
