/**
 * @file listening.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Listening for remote connections
 * @details
 * @version 0.1
 * @date 2020-12-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Check if the remote debugger needs to pause the system
 *
 * @param SerialHandle
 * @return BOOLEAN
 */
BOOLEAN ListeningSerialPortInDebuggee(HANDLE SerialHandle) {

StartAgain:

  BOOL Status;                 /* Status */
  char SerialBuffer[64] = {0}; /* Buffer to send and receive data */
  DWORD EventMask = 0;         /* Event mask to trigger */
  char ReadData = NULL;        /* temperory Character */
  DWORD NoBytesRead = 0;       /* Bytes read by ReadFile() */
  UINT32 Loop = 0;
  PDEBUGGER_REMOTE_PACKET TheActualPacket =
      (PDEBUGGER_REMOTE_PACKET)SerialBuffer;

  //
  // Setting Receive Mask
  //
  Status = SetCommMask(SerialHandle, EV_RXCHAR);
  if (Status == FALSE) {
    ShowMessages("err, in setting CommMask\n");
    return FALSE;
  }

  //
  // Setting WaitComm() Event
  //
  Status = WaitCommEvent(SerialHandle, &EventMask,
                         NULL); /* Wait for the character to be received */

  if (Status == FALSE) {
    ShowMessages("err,in setting WaitCommEvent()\n");
    return FALSE;
  }

  //
  // Read data and store in a buffer
  //
  do {
    Status =
        ReadFile(SerialHandle, &ReadData, sizeof(ReadData), &NoBytesRead, NULL);
    SerialBuffer[Loop] = ReadData;

    if (KdCheckForTheEndOfTheBuffer(&Loop, (BYTE *)SerialBuffer)) {
      break;
    }

    ++Loop;
  } while (NoBytesRead > 0);

  //
  // Get actual length of received data
  //
  // ShowMessages("Number of bytes received = %d\n", Loop);

  for (size_t i = 0; i < Loop; i++) {
    ShowMessages("%x ", SerialBuffer[i]);
  }
  ShowMessages("\n");

  if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKER) {

    //
    // Check if the packet type is correct
    //
    if (TheActualPacket->TypeOfThePacket !=
        DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE) {
      //
      // sth wrong happened, the packet is not belonging to use
      // nothing to do, just wait again
      //
      ShowMessages("err, unknown packet received from the debugger.\n");
      goto StartAgain;
    }

    //
    // It's a HyperDbg packet
    //
    switch (TheActualPacket->RequestedActionOfThePacket) {
    case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_PAUSE:
      if (!DebuggerPauseDebuggee()) {
        ShowMessages("err, debugger tries to pause the debuggee but the "
                     "attempt was unsuccessful.\n");
      }
      break;
    default:
      ShowMessages("err, unknown packet action received from the debugger.\n");
      break;
    }

  } else {
    //
    // It's not a HyperDbg packet, it's probably a GDB packet
    //
    DebugBreak();
  }

  //
  // Wait for debug pause command again
  //
  goto StartAgain;

  return TRUE;
}

/**
 * @brief Check if the remote debugger needs to pause the system
 *
 * @param SerialHandle
 * @return BOOLEAN
 */
DWORD WINAPI ListeningSerialPauseThread(PVOID Param) {

  //
  // Create a listening thead
  //
  ListeningSerialPortInDebuggee((HANDLE)Param);

  return 0;
}
