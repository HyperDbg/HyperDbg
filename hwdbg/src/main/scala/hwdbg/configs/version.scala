/**
 * @file
 *   version.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Versioning details
 * @details
 * @version 0.1
 * @date
 *   2024-05-01
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.version

import chisel3._
import chisel3.util._

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
