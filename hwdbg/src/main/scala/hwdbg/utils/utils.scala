/**
 * @file
 *   utils.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Different utilities and functionalities
 * @details
 * @version 0.1
 * @date
 *   2024-04-12
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.utils

import chisel3._
import chisel3.util._

/**
 * @brief
 *   Create logs and debug messages
 */
object LogInfo {

  def apply(debug: Boolean)(message: String): Unit = {
    if (debug) {
      println("[*] debug msg: " + message)
    }
  }
}

object BitwiseFunction {

  def getFirstNBits(num: Long, n: Int): Long = {

    val mask = (1L << n) - 1 // Create a bitmask with the first 'n' bits set to 1
    val shifted = num >>> (java.lang.Long.SIZE - n) // Shift the bits to the right to keep the first 'n' bits
    val firstNBits = shifted & mask // Extract the first 'n' bits by performing a bitwise AND operation

    firstNBits
  }

  def getBitsInRange(num: Long, start: Int, end: Int): Long = {

    require(start >= 0 && start <= 63, "Starting point must be between 0 and 63")
    require(end >= 0 && end <= 63, "Ending point must be between 0 and 63")
    require(start <= end, "Starting point must be less than or equal to ending point")

    val numBits = end - start + 1 // Number of bits in the range
    val mask = (1L << numBits) - 1 // Create a bitmask with 'numBits' bits set to 1
    val shifted = num >>> (java.lang.Long.SIZE - end - 1) // Shift the bits to the right to align the range with the rightmost position
    val bitsInRange = shifted & mask // Extract the bits within the specified range

    bitsInRange
  }

}
