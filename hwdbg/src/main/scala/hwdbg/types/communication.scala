/**
 * @file
 *   communication.scala
 * @author
 *   Sina Karvandi (sina@hyperdbg.org)
 * @brief
 *   Data types for the communication
 * @details
 * @version 0.1
 * @date
 *   2024-04-08
 *
 * @copyright
 *   This project is released under the GNU Public License v3.
 */
package hwdbg.types

import chisel3._

// -----------------------------------------------------------------------

//
// Structure in C:
//
// typedef struct _DEBUGGER_REMOTE_PACKET
// {
//     BYTE                                    Checksum;
//     UINT64                                  Indicator; /* Shows the type of the packet */
//     DEBUGGER_REMOTE_PACKET_TYPE             TypeOfThePacket;
//     DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION RequestedActionOfThePacket;
//
// } DEBUGGER_REMOTE_PACKET, *PDEBUGGER_REMOTE_PACKET;
//

/**
 * @brief
 *   The packet used for communication with the remote debugger
 */
class DebuggerRemotePacket() extends Bundle {

  //
  // Structure fields
  //
  val Checksum = UInt(8.W) // 1 bytes
  val Alignment0 = UInt((64 - 8).W) // 7 bytes
  val Indicator = UInt(64.W) // 8 bytes
  val TypeOfThePacket = UInt(32.W) // 4 bytes
  val RequestedActionOfThePacket = UInt(32.W) // 4 bytes

  //
  // Offset of structure fields
  //
  object Offset {

    val checksum = (0) / 8

    val indicator = (Checksum.getWidth + Alignment0.getWidth) / 8

    val typeOfThePacket = (Checksum.getWidth + Alignment0.getWidth + Indicator.getWidth) / 8

    val requestedActionOfThePacket = (Checksum.getWidth + Alignment0.getWidth + Indicator.getWidth + TypeOfThePacket.getWidth) / 8

    val startOfDataBuffer =
      (Checksum.getWidth + Alignment0.getWidth + Indicator.getWidth + TypeOfThePacket.getWidth + RequestedActionOfThePacket.getWidth) / 8
  }
}

// -----------------------------------------------------------------------

//
// Structure in C:
//
// typedef struct _HWDBG_PORT_INFORMATION
// {
//     UINT32 CountOfPorts;
//
//     /*
//
//     Here the pin information details will be available.
//
//         UINT32
//         Port Size
//
//     */
//
// } HWDBG_PORT_INFORMATION, *PHWDBG_PORT_INFORMATION;

/**
 * @brief
 *   The structure of port information in hwdbg
 */
class HwdbgPortInformation() extends Bundle {

  //
  // Structure fields
  //
  val CountOfPorts = UInt(32.W) // 4 bytes

  //
  // Offset of structure fields
  //
  object Offset {

    val countOfPorts = (0) / 8
  }
}

// -----------------------------------------------------------------------

//
// Structure in C:
//
// typedef struct _HWDBG_PORT_INFORMATION_ITEMS
// {
//     UINT32 PortIndex;
//
// } HWDBG_PORT_INFORMATION_ITEMS, *PHWDBG_PORT_INFORMATION_ITEMS;

/**
 * @brief
 *   The structure of port information (each item) in hwdbg
 */
class HwdbgPortInformationItems() extends Bundle {

  //
  // Structure fields
  //
  val PortSize = UInt(32.W) // 4 bytes

  //
  // Offset of structure fields
  //
  object Offset {

    val portSize = (0) / 8
  }
}

// -----------------------------------------------------------------------

/**
 * @brief
 *   Different action of hwdbg (SHARED WITH HYPERDBG) (HWDBG_ACTION_ENUMS)
 * @warning
 *   Used in HyperDbg
 */
object HwdbgActionEnums extends Enumeration {

  val hwdbgActionSendInstanceInfo = Value(1)
  val hwdbgActionConfigureScriptBuffer = Value(2)

}

// -----------------------------------------------------------------------

/**
 * @brief
 *   Different responses of hwdbg (SHARED WITH HYPERDBG) (HWDBG_RESPONSE_ENUMS)
 * @warning
 *   Used in HyperDbg
 */
object HwdbgResponseEnums extends Enumeration {

  val hwdbgResponseSuccessOrErrorMessage = Value(1)
  val hwdbgResponseInstanceInfo = Value(2)

}

// -----------------------------------------------------------------------

/**
 * @brief
 *   Different responses of hwdbg (SHARED WITH HYPERDBG) (HWDBG_ERROR_ENUMS)
 * @warning
 */
object HwdbgSuccessOrErrorEnums extends Enumeration {

  val hwdbgOperationWasSuccessful = Value(0x7fffffff)
  val hwdbgErrorInvalidPacket = Value(1)

}

// -----------------------------------------------------------------------
