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

import io.circe._
import io.circe.parser._
import io.circe.generic.auto._
import scala.io.Source

import chisel3._
import chisel3.util._

import hwdbg.utils._

/**
 * @brief
 *   Version of hwdbg (Definition and Default Values)
 * @warning
 *   will be checked with HyperDbg
 */
object Version {

  //
  // Constant version info
  //
  var VERSION_MAJOR: Int = 0
  var VERSION_MINOR: Int = 1
  var VERSION_PATCH: Int = 0

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
 *   Design constants (Definition and Default Values)
 */
object DebuggerConfigurations {

  //
  // whether to enable debug or not
  //
  var ENABLE_DEBUG: Boolean = true

  //
  // Number of input/output pins
  //
  var NUMBER_OF_PINS: Int = 32

  //
  // The configuration of ports and pins
  //
  // The following constant shows the key value object of the mappings
  // of pins to ports (used for inputs/outputs)
  //    For example,
  //                port 0 (in) -> contains 12 pins
  //                port 1 (in) -> contains 20 pins
  //
  // var PORT_PINS_MAP: Array[Int] = Array(12, 18, 2)
  // var PORT_PINS_MAP: Array[Int] = Array(12, 10, 5, 3, 2)
  var PORT_PINS_MAP: Array[Int] = Array(12, 20)
}

/**
 * @brief
 *   Design constants for script engine (Definition and Default Values)
 */
object ScriptEngineConfigurations {

  //
  // Maximum number of stages
  //
  var MAXIMUM_NUMBER_OF_STAGES: Int = 32

  //
  // Maximum number of stages
  //
  var MAXIMUM_NUMBER_OF_SUPPORTED_GET_SCRIPT_OPERATORS: Int = 2 // for get values

  //
  // Maximum number of stages
  //
  var MAXIMUM_NUMBER_OF_SUPPORTED_SET_SCRIPT_OPERATORS: Int = 1 // for get value

  //
  // Script variable length
  //
  var SCRIPT_VARIABLE_LENGTH: Int = 8

  //
  // Number supported of local and global variables
  //
  var NUMBER_OF_SUPPORTED_LOCAL_AND_GLOBAL_VARIABLES: Int = 2

  //
  // Number supported of temporary variables
  //
  var NUMBER_OF_SUPPORTED_TEMPORARY_VARIABLES: Int = 2

  //
  // Define the capabilities you want to enable
  //
  var SCRIPT_ENGINE_EVAL_CAPABILITIES = Seq(
    //
    // Statements and expressions
    //
    HwdbgScriptCapabilities.assign_local_global_var,
    HwdbgScriptCapabilities.assign_registers,
    // HwdbgScriptCapabilities.assign_pseudo_registers,
    HwdbgScriptCapabilities.conditional_statements_and_comparison_operators,
    HwdbgScriptCapabilities.stack_assignments,

    //
    // Operators
    //
    HwdbgScriptCapabilities.func_or,
    HwdbgScriptCapabilities.func_xor,
    HwdbgScriptCapabilities.func_and,
    HwdbgScriptCapabilities.func_asl,
    HwdbgScriptCapabilities.func_add,
    HwdbgScriptCapabilities.func_sub,
    HwdbgScriptCapabilities.func_mul,
    // HwdbgScriptCapabilities.func_div,
    // HwdbgScriptCapabilities.func_mod,
    HwdbgScriptCapabilities.func_gt,
    HwdbgScriptCapabilities.func_lt,
    HwdbgScriptCapabilities.func_egt,
    HwdbgScriptCapabilities.func_elt,
    HwdbgScriptCapabilities.func_equal,
    HwdbgScriptCapabilities.func_neq,
    HwdbgScriptCapabilities.func_jmp,
    HwdbgScriptCapabilities.func_jz,
    HwdbgScriptCapabilities.func_jnz,
    HwdbgScriptCapabilities.func_mov
    // HwdbgScriptCapabilities.func_printf,
  )
}

/**
 * @brief
 *   The constants for memory communication (Definition and Default Values)
 */
object MemoryCommunicationConfigurations {

  //
  // Address width of the Block RAM (BRAM)
  //
  var BLOCK_RAM_ADDR_WIDTH: Int = 13

  //
  // Data width of the Block RAM (BRAM)
  //
  var BLOCK_RAM_DATA_WIDTH: Int = 32

  //
  // Emulate block RAM by inferring a register to delay one clock cycle
  //
  var ENABLE_BLOCK_RAM_DELAY: Boolean = true

  //
  // Default number of bytes used in initialized SRAM memory
  //
  var DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE: Int = 8192 / 8 // 8 Kilobits

  //
  // Base address of PS to PL SRAM communication memory
  //
  var BASE_ADDRESS_OF_PS_TO_PL_COMMUNICATION: Int = 0

  //
  // Base address of PL to PS SRAM communication memory
  //
  var BASE_ADDRESS_OF_PL_TO_PS_COMMUNICATION: Int = DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE / 2
}

/**
 * @brief
 *   The structure of script capabilities information in hwdbg
 * @details
 *   Same as _HWDBG_INSTANCE_INFORMATION in HyperDbg
 */
case class HwdbgInstanceInformation(
    version: Int, // Target version of HyperDbg (same as hwdbg)
    maximumNumberOfStages: Int, // Number of stages that this instance of hwdbg supports (NumberOfSupportedStages == 0 means script engine is disabled)
    scriptVariableLength: Int, // Maximum length of variables (and other script elements)
    numberOfSupportedLocalAndGlobalVariables: Int, // Number of supported local (and global) variables
    numberOfSupportedTemporaryVariables: Int, // Number of supported temporary variables
    maximumNumberOfSupportedGetScriptOperators: Int, // Maximum supported GET operators in a single func
    maximumNumberOfSupportedSetScriptOperators: Int, // Maximum supported SET operators in a single func
    sharedMemorySize: Int, // Size of shared memory
    debuggerAreaOffset: Int, // The memory offset of debugger
    debuggeeAreaOffset: Int, // The memory offset of debuggee
    numberOfPins: Int, // Number of pins
    numberOfPorts: Int, // Number of ports
    scriptCapabilities: Long, // Capabilities bitmask
    bramAddrWidth: Int, // BRAM address width
    bramDataWidth: Int, // BRAM data width
    portsConfiguration: Array[Int] // Port arrangement
)

/**
 * @brief
 *   The script engine capabilities (Definition and Default Values)
 */
object HwdbgScriptCapabilities {

  //
  // Statements and expressions
  //
  val assign_local_global_var: Long = 1L << 0
  val assign_registers: Long = 1L << 1
  val assign_pseudo_registers: Long = 1L << 2
  val conditional_statements_and_comparison_operators: Long = 1L << 3
  val stack_assignments: Long = 1L << 4

  //
  // Operators
  //
  val func_or: Long = 1L << 5
  val func_xor: Long = 1L << 6
  val func_and: Long = 1L << 7
  val func_asr: Long = 1L << 8
  val func_asl: Long = 1L << 9
  val func_add: Long = 1L << 10
  val func_sub: Long = 1L << 11
  val func_mul: Long = 1L << 12
  val func_div: Long = 1L << 13
  val func_mod: Long = 1L << 14
  val func_gt: Long = 1L << 15
  val func_lt: Long = 1L << 16
  val func_egt: Long = 1L << 17
  val func_elt: Long = 1L << 18
  val func_equal: Long = 1L << 19
  val func_neq: Long = 1L << 20
  val func_jmp: Long = 1L << 21
  val func_jz: Long = 1L << 22
  val func_jnz: Long = 1L << 23
  val func_mov: Long = 1L << 24
  val func_printf: Long = 1L << 25

  def allCapabilities: Seq[Long] = Seq(
    assign_local_global_var,
    assign_registers,
    assign_pseudo_registers,
    conditional_statements_and_comparison_operators,
    stack_assignments,
    func_or,
    func_xor,
    func_and,
    func_asr,
    func_asl,
    func_add,
    func_sub,
    func_mul,
    func_div,
    func_mod,
    func_gt,
    func_lt,
    func_egt,
    func_elt,
    func_equal,
    func_neq,
    func_jmp,
    func_jz,
    func_jnz,
    func_mov,
    func_printf
  )

  //
  // Utility method to create a bitmask from a sequence of capabilities
  //
  def createCapabilitiesMask(capabilities: Seq[Long]): Long = {
    capabilities.foldLeft(0L)(_ | _)
  }

  //
  // Function to check if a capability is supported
  //
  def isCapabilitySupported(supportedCapabilities: Long, capability: Long): Boolean = {
    (supportedCapabilities & capability) != 0
  }

}

object HwdbgInstanceInformation {

  //
  // Function to create an instance of HwdbgInstanceInformation
  //
  def createInstanceInformation(
      version: Int,
      maximumNumberOfStages: Int,
      scriptVariableLength: Int,
      numberOfSupportedLocalAndGlobalVariables: Int,
      numberOfSupportedTemporaryVariables: Int,
      maximumNumberOfSupportedGetScriptOperators: Int,
      maximumNumberOfSupportedSetScriptOperators: Int,
      sharedMemorySize: Int,
      debuggerAreaOffset: Int,
      debuggeeAreaOffset: Int,
      numberOfPins: Int,
      numberOfPorts: Int,
      enabledCapabilities: Seq[Long],
      bramAddrWidth: Int,
      bramDataWidth: Int,
      portsConfiguration: Array[Int]
  ): HwdbgInstanceInformation = {

    val capabilitiesMask = HwdbgScriptCapabilities.createCapabilitiesMask(enabledCapabilities)

    //
    // Printing the versioning info
    //
    LogInfo(true)("=======================================================================")
    LogInfo(true)(
      s"Generating code for hwdbg v${Version.extractMajor(version)}.${Version.extractMinor(version)}.${Version.extractPatch(version)} ($version)"
    )
    LogInfo(true)("Please visit https://hwdbg.hyperdbg.org/docs for more information...")
    LogInfo(true)("hwdbg is released under the GNU Public License v3 (GPLv3).")
    LogInfo(true)("=======================================================================")

    HwdbgInstanceInformation(
      version = version,
      maximumNumberOfStages = maximumNumberOfStages,
      scriptVariableLength = scriptVariableLength,
      numberOfSupportedLocalAndGlobalVariables = numberOfSupportedLocalAndGlobalVariables,
      numberOfSupportedTemporaryVariables = numberOfSupportedTemporaryVariables,
      maximumNumberOfSupportedGetScriptOperators = maximumNumberOfSupportedGetScriptOperators,
      maximumNumberOfSupportedSetScriptOperators = maximumNumberOfSupportedSetScriptOperators,
      sharedMemorySize = sharedMemorySize,
      debuggerAreaOffset = debuggerAreaOffset,
      debuggeeAreaOffset = debuggeeAreaOffset,
      numberOfPins = numberOfPins,
      numberOfPorts = numberOfPorts,
      scriptCapabilities = capabilitiesMask,
      bramAddrWidth = bramAddrWidth,
      bramDataWidth = bramDataWidth,
      portsConfiguration = portsConfiguration
    )
  }
}

object ConfigLoader {

  //
  // Case classes for each configuration section
  //
  case class VersionConfig(
    VERSION_MAJOR: Int,
    VERSION_MINOR: Int,
    VERSION_PATCH: Int
    )
  case class DebuggerConfigurationsConfig(
    ENABLE_DEBUG: Boolean,
    NUMBER_OF_PINS: Int,
    PORT_PINS_MAP: Array[Int]
    )
  case class ScriptEngineConfigurationsConfig(
      MAXIMUM_NUMBER_OF_STAGES: Int,
      MAXIMUM_NUMBER_OF_SUPPORTED_GET_SCRIPT_OPERATORS: Int,
      MAXIMUM_NUMBER_OF_SUPPORTED_SET_SCRIPT_OPERATORS: Int,
      SCRIPT_VARIABLE_LENGTH: Int,
      NUMBER_OF_SUPPORTED_LOCAL_AND_GLOBAL_VARIABLES: Int,
      NUMBER_OF_SUPPORTED_TEMPORARY_VARIABLES: Int,
      SCRIPT_ENGINE_EVAL_CAPABILITIES: Seq[String]
  )
  case class MemoryCommunicationConfigurationsConfig(
      BLOCK_RAM_ADDR_WIDTH: Int,
      BLOCK_RAM_DATA_WIDTH: Int,
      ENABLE_BLOCK_RAM_DELAY: Boolean,
      DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE: Int,
      BASE_ADDRESS_OF_PS_TO_PL_COMMUNICATION: Int,
      BASE_ADDRESS_OF_PL_TO_PS_COMMUNICATION: Int
  )
  case class FullConfig(
      Version: VersionConfig,
      DebuggerConfigurations: DebuggerConfigurationsConfig,
      ScriptEngineConfigurations: ScriptEngineConfigurationsConfig,
      MemoryCommunicationConfigurations: MemoryCommunicationConfigurationsConfig
  )

  //
  // Function to load configuration from a JSON file
  //
  def loadConfig(filePath: String): Option[FullConfig] = {
    val source = Source.fromFile(filePath)
    val jsonString = try source.getLines().mkString("\n") finally source.close()

    decode[FullConfig](jsonString) match {
      case Right(config) => Some(config)
      case Left(error) =>
        println(s"Failed to parse JSON configuration: $error")
        None
    }
  }
}

object LoadConfiguration {

  def loadFromJson(configPath: String): Unit = {
    
    val configOpt = ConfigLoader.loadConfig(configPath)

    configOpt.foreach { config =>

      //
      // *** Set the values in the respective objects ***
      //

      //
      // Read the version
      //
      Version.VERSION_MAJOR = config.Version.VERSION_MAJOR
      Version.VERSION_MINOR = config.Version.VERSION_MINOR
      Version.VERSION_PATCH = config.Version.VERSION_PATCH

      //
      // Read the debugger configurations
      //
      DebuggerConfigurations.ENABLE_DEBUG = config.DebuggerConfigurations.ENABLE_DEBUG
      DebuggerConfigurations.NUMBER_OF_PINS = config.DebuggerConfigurations.NUMBER_OF_PINS
      DebuggerConfigurations.PORT_PINS_MAP = config.DebuggerConfigurations.PORT_PINS_MAP

      //
      // Read the script engine configurations
      //
      ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_STAGES = config.ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_STAGES
      ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_GET_SCRIPT_OPERATORS = config.ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_GET_SCRIPT_OPERATORS
      ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_SET_SCRIPT_OPERATORS = config.ScriptEngineConfigurations.MAXIMUM_NUMBER_OF_SUPPORTED_SET_SCRIPT_OPERATORS
      ScriptEngineConfigurations.SCRIPT_VARIABLE_LENGTH = config.ScriptEngineConfigurations.SCRIPT_VARIABLE_LENGTH
      ScriptEngineConfigurations.NUMBER_OF_SUPPORTED_LOCAL_AND_GLOBAL_VARIABLES = config.ScriptEngineConfigurations.NUMBER_OF_SUPPORTED_LOCAL_AND_GLOBAL_VARIABLES
      ScriptEngineConfigurations.NUMBER_OF_SUPPORTED_TEMPORARY_VARIABLES = config.ScriptEngineConfigurations.NUMBER_OF_SUPPORTED_TEMPORARY_VARIABLES
      
      //
      // Convert string capability names to the corresponding values in HwdbgScriptCapabilities
      //
      ScriptEngineConfigurations.SCRIPT_ENGINE_EVAL_CAPABILITIES = config.ScriptEngineConfigurations.SCRIPT_ENGINE_EVAL_CAPABILITIES.flatMap {
        case "assign_local_global_var" => Some(HwdbgScriptCapabilities.assign_local_global_var)
        case "assign_registers" => Some(HwdbgScriptCapabilities.assign_registers)
        case "conditional_statements_and_comparison_operators" => Some(HwdbgScriptCapabilities.conditional_statements_and_comparison_operators)
        case "stack_assignments" => Some(HwdbgScriptCapabilities.stack_assignments)
        case "func_or" => Some(HwdbgScriptCapabilities.func_or)
        case "func_xor" => Some(HwdbgScriptCapabilities.func_xor)
        case "func_and" => Some(HwdbgScriptCapabilities.func_and)
        case "func_asl" => Some(HwdbgScriptCapabilities.func_asl)
        case "func_add" => Some(HwdbgScriptCapabilities.func_add)
        case "func_sub" => Some(HwdbgScriptCapabilities.func_sub)
        case "func_mul" => Some(HwdbgScriptCapabilities.func_mul)
        case "func_gt" => Some(HwdbgScriptCapabilities.func_gt)
        case "func_lt" => Some(HwdbgScriptCapabilities.func_lt)
        case "func_egt" => Some(HwdbgScriptCapabilities.func_egt)
        case "func_elt" => Some(HwdbgScriptCapabilities.func_elt)
        case "func_equal" => Some(HwdbgScriptCapabilities.func_equal)
        case "func_neq" => Some(HwdbgScriptCapabilities.func_neq)
        case "func_jmp" => Some(HwdbgScriptCapabilities.func_jmp)
        case "func_jz" => Some(HwdbgScriptCapabilities.func_jz)
        case "func_jnz" => Some(HwdbgScriptCapabilities.func_jnz)
        case "func_mov" => Some(HwdbgScriptCapabilities.func_mov)
        case _ => None
      }

      //
      // Read memory communication configurations
      //
      MemoryCommunicationConfigurations.BLOCK_RAM_ADDR_WIDTH = config.MemoryCommunicationConfigurations.BLOCK_RAM_ADDR_WIDTH
      MemoryCommunicationConfigurations.BLOCK_RAM_DATA_WIDTH = config.MemoryCommunicationConfigurations.BLOCK_RAM_DATA_WIDTH
      MemoryCommunicationConfigurations.ENABLE_BLOCK_RAM_DELAY = config.MemoryCommunicationConfigurations.ENABLE_BLOCK_RAM_DELAY
      MemoryCommunicationConfigurations.DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE = config.MemoryCommunicationConfigurations.DEFAULT_CONFIGURATION_INITIALIZED_MEMORY_SIZE
      MemoryCommunicationConfigurations.BASE_ADDRESS_OF_PS_TO_PL_COMMUNICATION = config.MemoryCommunicationConfigurations.BASE_ADDRESS_OF_PS_TO_PL_COMMUNICATION
      MemoryCommunicationConfigurations.BASE_ADDRESS_OF_PL_TO_PS_COMMUNICATION = config.MemoryCommunicationConfigurations.BASE_ADDRESS_OF_PL_TO_PS_COMMUNICATION
    }
  }
}