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

        //
        // Registers are pins (set the value based on less significant bit)
        //
        inputPin.bitSet(io.operator.Value(log2Ceil(instanceInfo.numberOfPins), 0), io.inputValue(0).asBool)
        outputPin := inputPin
      }
      is(symbolPseudoRegType) { 

        //
        // Iterate based on port configuration
        //
        var currentPortIndex: Int = 0
        var currentPortNum: Int = 0
        val numPorts = instanceInfo.portsConfiguration.length

        for ((port, index) <- instanceInfo.portsConfiguration.zipWithIndex) {

            when(io.operator.Value === currentPortIndex.U) {

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
                    LogInfo(debug)(f"Set connecting index=${index} - inputPin(${instanceInfo.numberOfPins - 1}, ${high + 1}) + io.inputValue(${port - 1}, 0)")
                    Cat(
                        inputPin(instanceInfo.numberOfPins - 1, high + 1), // Bits above the range to keep unchanged
                        io.inputValue(port - 1, 0)// New value for the specified range
                    )
                } else if (index == numPorts - 1) {

                    //
                    // Last port: keep lower bits unchanged, set higher bits to input value
                    //
                    LogInfo(debug)(f"Set connecting index=${index} - io.inputValue(${port - 1}, 0) + inputPin(${low - 1}, 0)")
                    Cat(
                        io.inputValue(port - 1, 0),// New value for the specified range
                        inputPin(low - 1, 0)        // Bits below the range to keep unchanged
                    )
                } else {

                    //
                    // Middle port: keep both higher and lower bits unchanged
                    //
                    LogInfo(debug)(f"Set connecting index=${index} - inputPin(${instanceInfo.numberOfPins - 1}, ${high + 1}) + io.inputValue(${port - 1}, 0) + inputPin(${low - 1}, 0)")
                    Cat(
                        inputPin(instanceInfo.numberOfPins - 1, high + 1), // Bits above the range to keep unchanged
                        io.inputValue(port - 1, 0),// New value for the specified range
                        inputPin(low - 1, 0)                               // Bits below the range to keep unchanged
                    )
                }

                //
                // Assign the modified outputPin back to outputPin
                //
                outputPin := modifiedOutputPin
            }

            currentPortNum += port
            currentPortIndex += 1
        }
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
