/**
 * @file
 *   configs.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Configuration files
 * @details
 * @version 0.1
 * @date
 *   2024-04-03
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.configs

import chisel3._
import chisel3.util._

import hwdbg.utils._

/**
 * @brief
 *   Version of hwdbg
 * @warning
 *   will be checked with HyperDbg
 */
object Version {

  //
  // Constant version info
  //
  val VERSION_MAJOR: Int = 0
  val VERSION_MINOR: Int = 1
  val VERSION_PATCH: Int = 0

  def getEncodedVersion: Int = {
    (VERSION_MAJOR << 16) | (VERSION_MINOR << 8) | VERSION_PATCH
  }

  def extractMajor(encodedVersion: Int): Int = {
    encodedVersion >> 16
  }

  def extractMinor(encodedVersion: Int): Int = {
    (encodedVersion >> 8) & 0xff // Masking to get only the 8 bits
  }

  def extractPatch(encodedVersion: Int): Int = {
    encodedVersion & 0xff // Masking to get only the 8 bits
  }
}

/**
 * @brief
 *   The configuration of ports and pins
 */
object DebuggerPorts {

  //
  // The following constant shows the key value object of the mappings
  // of pins to ports (used for inputs/outputs)
  //    For example,
  //                port 0 (in) -> contains 12 pins
  //                port 1 (in) -> contains 9 pins
  //
  val PORT_PINS_MAP: Map[Int, Int] = Map(0 -> 12, 1 -> 9, 2 -> 11)

}

/**
 * @brief
 *   Design constants
 */
object DebuggerConfigurations {

  //
  // whether to enable debug or not
  //
  val ENABLE_DEBUG: Boolean = true

  //
  // Number of input/output pins
  //
  val NUMBER_OF_PINS: Int = 32

  //
  // Address width of the Block RAM (BRAM)
  //
  val BLOCK_RAM_ADDR_WIDTH: Int = 13

  //
  // Data width of the Block RAM (BRAM)
  //
  val BLOCK_RAM_DATA_WIDTH: Int = 32

}

/**
 * @brief
 *   Design constants for script engine
 */
object ScriptEngineConfigurations {

  //
  // Maximum number of stages
  //
  val MAXIMUM_NUMBER_OF_STAGES: Int = 10

  //
  // Maximum number of stages
  //
  val MAXIMUM_NUMBER_OF_SUPPORTED_OPERATORS: Int = 3 // 2 for get value and 1 for set value

  //
  // Script variable length
  //
  val SCRIPT_VARIABLE_LENGTH: Int = 64

  //  
  // Define the capabilities you want to enable
  //
    val SCRIPT_ENGINE_EVAL_CAPABILITIES = Seq(
      HwdbgScriptCapabilities.Inc,
      HwdbgScriptCapabilities.Dec,
      HwdbgScriptCapabilities.Or,
      HwdbgScriptCapabilities.Xor,
      HwdbgScriptCapabilities.And,
      HwdbgScriptCapabilities.Asl,
      HwdbgScriptCapabilities.Add,
      HwdbgScriptCapabilities.Sub,
      HwdbgScriptCapabilities.Mul,
      HwdbgScriptCapabilities.Div,
      HwdbgScriptCapabilities.Mod,
      HwdbgScriptCapabilities.Gt,
      HwdbgScriptCapabilities.Lt,
      HwdbgScriptCapabilities.Egt,
      HwdbgScriptCapabilities.Elt,
      HwdbgScriptCapabilities.Equal,
      HwdbgScriptCapabilities.Neq,
      HwdbgScriptCapabilities.Jmp,
      HwdbgScriptCapabilities.Jz,
      HwdbgScriptCapabilities.Jnz,
      HwdbgScriptCapabilities.Mov,
      HwdbgScriptCapabilities.Printf
    )
}

/**
 * @brief
 *   The constants for memory communication
 */
object MemoryCommunicationConfigurations {

  //
  // Emulate block RAM by inferring a register to delay one clock cycle
  //
  val ENABLE_BLOCK_RAM_DELAY: Boolean = true

  //
  // Default number of bytes used in initialized SRAM memory
  //
  val DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE: Int = 8192 / 8 // 8 Kilobits

  //
  // Base address of PS to PL SRAM communication memory
  //
  val BASE_ADDRESS_OF_PS_TO_PL_COMMUNICATION: Int = 0

  //
  // Base address of PL to PS SRAM communication memory
  //
  val BASE_ADDRESS_OF_PL_TO_PS_COMMUNICATION: Int = DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE / 2
}

/**
 * @brief The structure of script capabilities information in hwdbg
 * @details Same as _HWDBG_INSTANCE_INFORMATION in HyperDbg
 */
case class HwdbgInstanceInformation(
  version: Int,                 // Target version of HyperDbg (same as hwdbg)
  maximumNumberOfStages: Int,   // Number of stages that this instance of hwdbg supports (NumberOfSupportedStages == 0 means script engine is disabled)
  scriptVariableLength: Int, // Maximum length of variables (and other script elements)
  maximumNumberOfSupportedScriptOperators: Int, // Maximum supported operators in a single func
  numberOfPins: Int,            // Number of pins
  numberOfPorts: Int,           // Number of ports
  scriptCapabilities: Long            // Capabilities bitmask
)

object HwdbgScriptCapabilities {
  val Inc: Long = 1L << 0
  val Dec: Long = 1L << 1
  val Or: Long = 1L << 2
  val Xor: Long = 1L << 3
  val And: Long = 1L << 4
  val Asr: Long = 1L << 5
  val Asl: Long = 1L << 6
  val Add: Long = 1L << 7
  val Sub: Long = 1L << 8
  val Mul: Long = 1L << 9
  val Div: Long = 1L << 10
  val Mod: Long = 1L << 11
  val Gt: Long = 1L << 12
  val Lt: Long = 1L << 13
  val Egt: Long = 1L << 14
  val Elt: Long = 1L << 15
  val Equal: Long = 1L << 16
  val Neq: Long = 1L << 17
  val Jmp: Long = 1L << 18
  val Jz: Long = 1L << 19
  val Jnz: Long = 1L << 20
  val Mov: Long = 1L << 21
  val Printf: Long = 1L << 22

  def allCapabilities: Seq[Long] = Seq(
    Inc, Dec, Or, Xor, And, Asr, Asl, Add, Sub, Mul, Div, Mod, Gt, Lt,
    Egt, Elt, Equal, Neq, Jmp, Jz, Jnz, Mov, Printf
  )
}

object HwdbgInstanceInformation {

  //
  // Utility method to create a bitmask from a sequence of capabilities
  //
  def createCapabilitiesMask(capabilities: Seq[Long]): Long = {
    capabilities.foldLeft(0L)(_ | _)
  }

  //
  // Function to create an instance of HwdbgInstanceInformation
  //
  def createInstanceInformation(
    version: Int,
    maximumNumberOfStages: Int,
    scriptVariableLength: Int,
    maximumNumberOfSupportedScriptOperators: Int,
    numberOfPins: Int,
    numberOfPorts: Int,
    enabledCapabilities: Seq[Long]
  ): HwdbgInstanceInformation = {

    val capabilitiesMask = createCapabilitiesMask(enabledCapabilities)

    //
    // Printing the versioning info
    //
    LogInfo(true)("=======================================================================")
    LogInfo(true)(s"Generating code for hwdbg v${Version.extractMajor(version)}.${Version.extractMinor(version)}.${Version.extractPatch(version)} ($version)")
    LogInfo(true)("Please visit https://hwdbg.hyperdbg.org/docs for more information...")
    LogInfo(true)("hwdbg is released under the GNU Public License v3 (GPLv3).")
    LogInfo(true)("=======================================================================")


    HwdbgInstanceInformation(
      version = version,
      maximumNumberOfStages = maximumNumberOfStages,
      scriptVariableLength = scriptVariableLength,
      maximumNumberOfSupportedScriptOperators = maximumNumberOfSupportedScriptOperators,
      numberOfPins = numberOfPins,
      numberOfPorts = numberOfPorts,
      scriptCapabilities = capabilitiesMask
    )
  }
}