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
