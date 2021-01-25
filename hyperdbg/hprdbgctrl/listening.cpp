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

//
// Global Variables
//
extern HANDLE
    g_SyncronizationObjectsHandleTable[DEBUGGER_MAXIMUM_SYNCRONIZATION_OBJECTS];
extern OVERLAPPED g_OverlappedIoStructureForReadDebugger;
extern OVERLAPPED g_OverlappedIoStructureForWriteDebugger;
extern HANDLE g_SerialRemoteComPortHandle;

/**
 * @brief Check if the remote debuggee needs to pause the system
 * and also process the debuggee's messages
 *
 * @return BOOLEAN
 */
BOOLEAN ListeningSerialPortInDebugger() {

StartAgain:

  CHAR BufferToReceive[0x1000] = {0};
  UINT32 LengthReceived = 0;
  PDEBUGGER_REMOTE_PACKET TheActualPacket;

  //
  // Wait for handshake to complete or in other words
  // get the receive packet
  //
  if (!KdReceivePacketFromDebuggee(BufferToReceive, &LengthReceived)) {
    return FALSE;
  }

  TheActualPacket = (PDEBUGGER_REMOTE_PACKET)BufferToReceive;

  if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKER) {

    //
    // Check if the packet type is correct
    //
    if (TheActualPacket->TypeOfThePacket !=
        DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER) {
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
    case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_STARTED:

      ShowMessages("Connected to debuggee %s !\n",
                   ((CHAR *)TheActualPacket) + sizeof(DEBUGGER_REMOTE_PACKET));
      ShowMessages("Press CTRL+C to pause the debuggee\n");

      //
      // Signal the event that the debugger started
      //
      SetEvent(g_SyncronizationObjectsHandleTable
                   [DEBUGGER_SYNCRONIZATION_OBJECT_STARTED_PACKET_RECEIVED]);

      break;
    case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_PAUSED_AND_CURRENT_INSTRUCTION:

      HyperDbgDisassembler64(((UCHAR *)TheActualPacket) +
                                 sizeof(DEBUGGER_REMOTE_PACKET) +
                                 sizeof(UINT64),
                             *((UINT64 *)((UCHAR *)TheActualPacket) +
                               sizeof(DEBUGGER_REMOTE_PACKET)),
                             MAXIMUM_INSTR_SIZE, 1);

      //
      // Signal the event
      //
      SetEvent(g_SyncronizationObjectsHandleTable
                   [DEBUGGER_SYNCRONIZATION_OBJECT_PAUSED_DEBUGGEE_DETAILS]);

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
BOOLEAN ListeningSerialPortInDebuggee() {

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
  Status = SetCommMask(g_SerialRemoteComPortHandle, EV_RXCHAR);
  if (Status == FALSE) {
    ShowMessages("err, in setting CommMask\n");
    return FALSE;
  }

  //
  // Setting WaitComm() Event
  //
  Status = WaitCommEvent(g_SerialRemoteComPortHandle, &EventMask,
                         NULL); /* Wait for the character to be received */

  if (Status == FALSE) {
    ShowMessages("err,in setting WaitCommEvent()\n");
    return FALSE;
  }

  //
  // Read data and store in a buffer
  //
  do {

    Status = ReadFile(g_SerialRemoteComPortHandle, &ReadData, sizeof(ReadData),
                      &NoBytesRead, NULL);

    SerialBuffer[Loop] = ReadData;

    if (KdCheckForTheEndOfTheBuffer(&Loop, (BYTE *)SerialBuffer)) {
      break;
    }

    ++Loop;
  } while (NoBytesRead > 0);

  //
  // Because we used overlapped I/O on the other side, sometimes
  // the debuggee might cancel the read so it returns, if it returns
  // then we should restart reading again
  //
  if (Loop == 1 && SerialBuffer[0] == NULL) {

    //
    // Chunk data to cancel non async read
    //
    goto StartAgain;
  }

  //
  // Get actual length of received data
  //
  // ShowMessages("\nNumber of bytes received = %d\n", Loop);
  // for (size_t i = 0; i < Loop; i++) {
  //   ShowMessages("%x ", SerialBuffer[i]);
  // }
  // ShowMessages("\n");

  if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKER) {

    //
    // Check if the packet type is correct
    //
    if (TheActualPacket->TypeOfThePacket !=
        DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_USER_MODE) {
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
    case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_USER_MODE_PAUSE:
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
 * @brief Check if the remote debuggee needs to pause the system
 *
 * @param Param
 * @return BOOLEAN
 */
DWORD WINAPI ListeningSerialPauseDebuggerThread(PVOID Param) {

  //
  // Create a listening thead in debugger
  //
  ListeningSerialPortInDebugger();

  return 0;
}

/**
 * @brief Check if the remote debugger needs to pause the system
 *
 * @param SerialHandle
 * @return BOOLEAN
 */
DWORD WINAPI ListeningSerialPauseDebuggeeThread(PVOID Param) {

  //
  // Create a listening thead in debuggee
  //
  ListeningSerialPortInDebuggee();

  return 0;
}
