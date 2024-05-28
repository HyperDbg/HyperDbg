/**
 * @file
 *   constants.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Constant values
 * @details
 * @version 0.1
 * @date
 *   2024-04-16
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.constants

import chisel3._
import chisel3.util._

/**
 * @brief
 *   Shared value with HyperDbg
 * @warning
 *   used in HyperDbg
 */
object HyperDbgSharedConstants {

  //
  // Constant indicator of a HyperDbg packet
  //
  val INDICATOR_OF_HYPERDBG_PACKET: Long = 0x4859504552444247L // HYPERDBG = 0x4859504552444247

}

/**
 * @brief
 *   Enumeration for different packet types in HyperDbg packets (DEBUGGER_REMOTE_PACKET_TYPE)
 * @warning
 *   Used in HyperDbg
 */
object DebuggerRemotePacketType extends Enumeration {

  //
  // Debugger to debuggee (vmx-root)
  //
  val DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT = Value(1)

  //
  // Debugger to debuggee (user-mode)
  //
  val DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_USER_MODE = Value(2)

  //
  // Debuggee to debugger (user-mode and kernel-mode, vmx-root mode)
  //
  val DEBUGGEE_TO_DEBUGGER = Value(3)

  //
  // Debugger to debuggee (hardware), used in hwdbg
  //
  val DEBUGGER_TO_DEBUGGEE_HARDWARE_LEVEL = Value(4)

  //
  // Debuggee to debugger (hardware), used in hwdbg
  //
  val DEBUGGEE_TO_DEBUGGER_HARDWARE_LEVEL = Value(5)
}
