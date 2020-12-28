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
BOOLEAN ListeningSerialPort(HANDLE SerialHandle) {

StartAgain:

  BOOL Status;                 /* Status */
  char SerialBuffer[64] = {0}; /* Buffer to send and receive data */
  DWORD EventMask = 0;         /* Event mask to trigger */
  char ReadData = NULL;        /* temperory Character */
  DWORD NoBytesRead = 0;       /* Bytes read by ReadFile() */
  UINT32 Loop = 0;
  BOOLEAN StatusIoctl = 0;
  ULONG ReturnedLength = 0;
  DEBUGGER_PAUSE_PACKET_RECEIVED PauseRequest = {0};

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

  //
  // print receive data on console
  //

  int index = 0;
  for (index = 0; index < Loop; ++index) {

    /* ShowMessages("%c", SerialBuffer[index]); */

    if (SerialBuffer[index] == 'P') {

      /*
      ShowMessages("\nPause packet received.\n");
      */

      //
      // Send a pause IOCTL
      //
      StatusIoctl = DeviceIoControl(
          g_DeviceHandle,                        // Handle to device
          IOCTL_PAUSE_PACKET_RECEIVED,           // IO Control code
          &PauseRequest,                         // Input Buffer to driver.
          SIZEOF_DEBUGGER_PAUSE_PACKET_RECEIVED, // Input buffer
                                                 // length
          &PauseRequest,                         // Output Buffer from driver.
          SIZEOF_DEBUGGER_PAUSE_PACKET_RECEIVED, // Length of output
                                                 // buffer in bytes.
          &ReturnedLength,                       // Bytes placed in buffer.
          NULL                                   // synchronous call
      );

      if (!StatusIoctl) {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
      }

      if (PauseRequest.Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL) {

        //
        // Nothing to show, the request was successfully processed
        //
      } else {
        ShowErrorMessage(PauseRequest.Result);
        return FALSE;
      }
    }
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
  ListeningSerialPort((HANDLE)Param);

  return 0;
}
