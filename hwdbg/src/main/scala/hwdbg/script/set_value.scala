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
    // Input value
    //
    val inputValue = Input(UInt(instanceInfo.scriptVariableLength.W)) // input value

    //
    // Output signals
    //
    val inputPin = Input(Vec(instanceInfo.numberOfPins, UInt(1.W))) // output pins
    val outputPin = Output(Vec(instanceInfo.numberOfPins, UInt(1.W))) // output pins

  })

  //
  // Temp input
  //
  val inputPin = io.inputPin.asUInt

  //
  // Output pins
  //
  val outputPin = WireInit(0.U(instanceInfo.numberOfPins.W))

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

      is(symbolGlobalIdType) {

        //
        // To be implemented (Global Variables)
        //
        outputPin := 0.U
      }
      is(symbolLocalIdType) {

        //
        // To be implemented (Local Variables)
        //
        outputPin := 0.U
      }
      is(symbolRegisterType) {

        when(instanceInfo.numberOfPins.U > io.operator.Value) {

          //
          // *** Used for setting the pin value ***
          //

          //
          // Registers are pins (set the value based on less significant bit)
          //
          val tempShiftedBit = (1.U << io.operator.Value)

          when (io.inputValue(0) === 1.U) {
            outputPin := inputPin | tempShiftedBit;  // Set the N-th bit to 1
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
                    LogInfo(debug)(f"Appending zeros (${port - instanceInfo.scriptVariableLength}) to input variable (targetInputValue) to support port num: ${index}")
                    targetInputValue := Cat(io.inputValue, 0.U((port - instanceInfo.scriptVariableLength).W))
                  }
                  else {
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
                      LogInfo(debug)(f"Set connecting index=${index} - inputPin(${instanceInfo.numberOfPins - 1}, ${high + 1}) + targetInputValue(${port - 1}, 0)")
                      Cat(
                          inputPin(instanceInfo.numberOfPins - 1, high + 1), // Bits above the range to keep unchanged
                          targetInputValue// New value for the specified range
                      )
                  } else if (index == numPorts - 1) {

                      //
                      // Last port: keep lower bits unchanged, set higher bits to input value
                      //
                      LogInfo(debug)(f"Set connecting index=${index} - targetInputValue(${port - 1}, 0) + inputPin(${low - 1}, 0)")
                      Cat(
                          targetInputValue,// New value for the specified range
                          inputPin(low - 1, 0)        // Bits below the range to keep unchanged
                      )
                  } else {

                      //
                      // Middle port: keep both higher and lower bits unchanged
                      //
                      LogInfo(debug)(f"Set connecting index=${index} - inputPin(${instanceInfo.numberOfPins - 1}, ${high + 1}) + targetInputValue(${port - 1}, 0) + inputPin(${low - 1}, 0)")
                      Cat(
                          inputPin(instanceInfo.numberOfPins - 1, high + 1), // Bits above the range to keep unchanged
                          targetInputValue,// New value for the specified range
                          inputPin(low - 1, 0)                               // Bits below the range to keep unchanged
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
      }
      is(symbolPseudoRegType) {

        //
        // To be implemented
        //
        outputPin := 0.U
        
      }
      is(symbolTempType) {

        //
        // To be implemented
        //
        outputPin := 0.U
      }
      is(symbolStackTempType) {

        //
        // To be implemented
        //
        outputPin := 0.U
      }
      is(symbolFunctionParameterIdType) {

        //
        // To be implemented
        //
        outputPin := 0.U
      }
    }
  }

  //
  // Connect the output signals
  //
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
      inputValue: UInt,
      inputPin: Vec[UInt],
  ): (Vec[UInt]) = {

    val scriptEngineSetValueModule = Module(
      new ScriptEngineSetValue(
        debug,
        instanceInfo,
      )
    )

    val outputPin = Wire(Vec(instanceInfo.numberOfPins, UInt(1.W)))

    //
    // Configure the input signals
    //
    scriptEngineSetValueModule.io.en := en
    scriptEngineSetValueModule.io.operator := operator
    scriptEngineSetValueModule.io.inputValue := inputValue
    scriptEngineSetValueModule.io.inputPin := inputPin

    //
    // Configure the output signal
    //
    outputPin := scriptEngineSetValueModule.io.outputPin

    //
    // Return the output result
    //
    (outputPin)
  }
}
