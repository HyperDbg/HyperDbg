/**
 * @file kd.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief routines to kernel debugging
 * @details
 * @version 0.1
 * @date 2020-12-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern HANDLE g_SerialListeningThreadHandle;
extern HANDLE g_DebuggerRunningEventHandle;
extern HANDLE g_SerialRemoteComPortHandle;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;
extern BOOLEAN g_IsDebuggerConntectedToNamedPipe;
extern BOOLEAN g_IsDebuggeeRunning;

/**
 * @brief compares the buffer with a string
 *
 * @param CurrentLoopIndex Number of previously read bytes
 * @param Buffer
 * @return BOOLEAN
 */
BOOLEAN KdCheckForTheEndOfTheBuffer(PUINT32 CurrentLoopIndex, BYTE *Buffer) {

  UINT32 ActualBufferLength;

  ActualBufferLength = *CurrentLoopIndex;

  //
  // End of buffer is 4 character long
  //
  if (*CurrentLoopIndex <= 3) {
    return FALSE;
  }

  if (Buffer[ActualBufferLength] == 0xff &&
      Buffer[ActualBufferLength - 1] == 0xee &&
      Buffer[ActualBufferLength - 2] == 0x80 &&
      Buffer[ActualBufferLength - 3] == 0x00) {

    //
    // Clear the end character
    //
    Buffer[ActualBufferLength - 3] = NULL;
    Buffer[ActualBufferLength - 2] = NULL;
    Buffer[ActualBufferLength - 1] = NULL;
    Buffer[ActualBufferLength] = NULL;

    //
    // Set the new length
    //
    *CurrentLoopIndex = ActualBufferLength - 3;

    return TRUE;
  }
  return FALSE;
}

/**
 * @brief compares the buffer with a string
 *
 * @param Buffer
 * @param CompareBuffer
 * @return BOOLEAN
 */
BOOLEAN KdCompareBufferWithString(CHAR *Buffer, const CHAR *CompareBuffer) {

  int Result;

  Result = strcmp(Buffer, CompareBuffer);

  if (Result == 0)
    return TRUE;
  else
    return FALSE;
}

/**
 * @brief Sends a continue or 'g' command packet to the debuggee
 *
 * @return BOOLEAN
 */
BOOLEAN KdSendContinuePacketToDebuggee() {

  CHAR BufferToReceive[64] = {0};
  UINT32 LengthReceived = 0;

  //
  // Send 'G' as pause packet
  //
  if (!KdSendPacketToDebuggee("G", 1)) {
    return FALSE;
  }

  return TRUE;
}

/**
 * @brief Sends a PAUSE packet to the debuggee
 *
 * @return BOOLEAN
 */
BOOLEAN KdSendPausePacketToDebuggee() {

  CHAR BufferToReceive[64] = {0};
  UINT32 LengthReceived = 0;

  //
  // Send 'P' as pause packet
  //
  if (!KdSendPacketToDebuggee("P", 1)) {
    return FALSE;
  }

  //
  // Wait for 1 sec so it send the packets
  //
  Sleep(800);

  //
  // Wait for handshake to complete or in other words
  // get the receive packet
  //
  if (!KdReceivePacketFromDebuggee(BufferToReceive, &LengthReceived)) {
    return FALSE;
  }

  //
  // Check if the handshake is correct or not
  //
  if (KdCompareBufferWithString(BufferToReceive, "Paused")) {

    return TRUE;
  }

  return FALSE;
}

/**
 * @brief Sends a PAUSE packet to the debuggee
 *
 * @param BufferToSave
 * @param LengthReceived
 *
 * @return BOOLEAN
 */
BOOLEAN KdReceivePacketFromDebuggee(CHAR *BufferToSave,
                                    UINT32 *LengthReceived) {

  BOOL Status;           /* Status */
  char ReadData = NULL;  /* temperory Character */
  DWORD NoBytesRead = 0; /* Bytes read by ReadFile() */
  UINT32 Loop = 0;

  //
  // Read data and store in a buffer
  //
  do {
    Status = ReadFile(g_SerialRemoteComPortHandle, &ReadData, sizeof(ReadData),
                      &NoBytesRead, NULL);
    BufferToSave[Loop] = ReadData;

    if (KdCheckForTheEndOfTheBuffer(&Loop, (BYTE *)BufferToSave)) {
      break;
    }

    ++Loop;

  } while (NoBytesRead > 0);

  //
  // Set the length
  //
  *LengthReceived = Loop;

  return TRUE;
}

/**
 * @brief Sends a special packet to the debuggee
 *
 * @param Buffer
 * @param Length
 * @return BOOLEAN
 */
BOOLEAN KdSendPacketToDebuggee(const CHAR *Buffer, UINT32 Length) {

  BOOL Status;
  DWORD BytesWritten = 0;

  //
  // Check if the remote code's handle found or not
  //
  if (g_SerialRemoteComPortHandle == NULL) {
    ShowMessages("err, handle to remote debuggee's com port is not found.\n");
    return FALSE;
  }

  Status = WriteFile(g_SerialRemoteComPortHandle, // Handle to the Serialport
                     Buffer,        // Data to be written to the port
                     Length,        // No of bytes to write into the port
                     &BytesWritten, // No of bytes written to the port
                     NULL);

  if (Status == FALSE) {
    ShowMessages("err, fail to write to com port.\n");
    return FALSE;
  }

  //
  // Check if message delivered successfully
  //
  if (BytesWritten == Length) {
    return TRUE;
  }

  //
  // Not all the bytes are sent
  //
  return FALSE;
}

/**
 * @brief check if the debuggee needs to be paused
 *
 * @return VOID
 */
VOID KdBreakControlCheckAndPauseDebugger() {

  //
  // Check if debuggee is running, otherwise the user
  // pressed ctrl+c multiple times
  //
  if (g_IsDebuggeeRunning) {

    //
    // Send the pause request to the remote computer
    //
    if (KdSendPausePacketToDebuggee()) {
      //
      // Set the debuggee to show that it's not running
      //
      g_IsDebuggeeRunning = FALSE;

    } else {
      ShowMessages("err, unable to pause the debuggee\n");
    }

    //
    // Signal the event
    //
    SetEvent(g_DebuggerRunningEventHandle);
  }
}

/**
 * @brief check if the debuggee needs to be continued
 *
 * @return VOID
 */
VOID KdBreakControlCheckAndContinueDebugger() {

  //
  // Check if debuggee is paused or not
  //
  if (!g_IsDebuggeeRunning) {

    //
    // Send the continue request to the remote computer
    //
    if (KdSendContinuePacketToDebuggee()) {

      //
      // Set the debuggee to show that it's  running
      //
      g_IsDebuggeeRunning = TRUE;

      //
      // Halt the UI
      //
      KdTheRemoteSystemIsRunning();

    } else {
      ShowMessages("err, unable to continue the debuggee\n");
    }
  }
}

/**
 * @brief wait for a event to be triggered and if the debuggee
 * is running it just halts the system
 *
 * @return VOID
 */
VOID KdTheRemoteSystemIsRunning() {

  //
  // Indicate that the debuggee is running
  //
  ShowMessages("Debuggee is running...\n");

  //
  // Wait until the users press CTRL+C
  //
  WaitForSingleObject(g_DebuggerRunningEventHandle, INFINITE);
}

/**
 * @brief Prepare serial to connect to the remote server
 * @details wait to connect to debuggee (this is debugger)
 *
 * @param SerialHandle
 * @return BOOLEAN
 */
BOOLEAN KdPrepareSerialConnectionToRemoteSystem(HANDLE SerialHandle,
                                                BOOLEAN IsNamedPipe) {

StartAgain:

  BOOL Status;                 /* Status */
  char SerialBuffer[64] = {0}; /* Buffer to send and receive data */
  DWORD EventMask = 0;         /* Event mask to trigger */
  char ReadData = NULL;        /* temperory Character */
  DWORD NoBytesRead = 0;       /* Bytes read by ReadFile() */
  UINT32 Loop = 0;

  //
  // Show an indication to connect the debugger
  //
  ShowMessages("Waiting for debuggee to connect ...\n");

  if (!IsNamedPipe) {

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
  }

  //
  // Read data and store in a buffer
  //
  do {
    Status =
        ReadFile(SerialHandle, &ReadData, sizeof(ReadData), &NoBytesRead, NULL);
    SerialBuffer[Loop] = ReadData;

    //
    // Check for end character
    //
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
  // Wait for the 'Start' packet
  //
  if (Loop == 5 && SerialBuffer[0] == 'S' && SerialBuffer[1] == 't' &&
      SerialBuffer[2] == 'a' && SerialBuffer[3] == 'r' &&
      SerialBuffer[4] == 't') {

    //
    // Create an event for waiting if the debugger is running
    // (Manually. no signal)
    //
    g_DebuggerRunningEventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);

    //
    // Connected to the debuggee
    //
    g_IsSerialConnectedToRemoteDebuggee = TRUE;

    //
    // And debuggee is running
    //
    g_IsDebuggeeRunning = TRUE;

    //
    // Save the handler
    //
    g_SerialRemoteComPortHandle = SerialHandle;

    //
    // Is serial handle for a named pipe
    //
    g_IsDebuggerConntectedToNamedPipe = IsNamedPipe;

    //
    // Register the CTRL+C and CTRL+BREAK Signals handler
    //
    if (!SetConsoleCtrlHandler(BreakController, TRUE)) {
      ShowMessages(
          "Error in registering CTRL+C and CTRL+BREAK Signals handler\n");
      return FALSE;
    }

    ShowMessages("Connected to debuggee !\n");
    ShowMessages("Press CTRL+C to pause the debuggee\n");

    //
    // Wait for event on this thread
    //
    KdTheRemoteSystemIsRunning();

  } else {
    goto StartAgain;
  }

  return TRUE;
}

/**
 * @brief Prepare and initialize COM port
 *
 * @param PortName
 * @param Baudrate
 * @param Port
 * @param IsPreparing
 * @return BOOLEAN
 */
BOOLEAN KdPrepareAndConnectDebugPort(const char *PortName, DWORD Baudrate,
                                     UINT32 Port, BOOLEAN IsPreparing,
                                     BOOLEAN IsNamedPipe) {

  HANDLE Comm;                 /* Handle to the Serial port */
  BOOL Status;                 /* Status */
  DCB SerialParams = {0};      /* Initializing DCB structure */
  COMMTIMEOUTS Timeouts = {0}; /* Initializing timeouts structure */
  char PortNo[20] = {0};       /* contain friendly name */
  BOOLEAN StatusIoctl;
  ULONG ReturnedLength;
  DEBUGGER_PREPARE_DEBUGGEE DebuggeeRequest = {0};

  if (IsPreparing && IsNamedPipe) {
    ShowMessages("err, cannot used named pipe for debuggee");
    return FALSE;
  }

  //
  // Check if driver is loaded or not, in the case
  // of connecting to a remote machine as debuggee
  //
  if (IsPreparing) {
    if (!g_DeviceHandle) {
      ShowMessages(
          "Handle not found, probably the driver is not loaded. Did you "
          "use 'load' command?\n");
      return FALSE;
    }
  }

  if (!IsNamedPipe) {

    //
    // It's a serial
    //

    //
    // Append name to make a Windows understandable format
    //
    sprintf_s(PortNo, 20, "\\\\.\\%s", PortName);

    //
    // Open the serial com port
    //
    Comm = CreateFile(PortNo,                       // Friendly name
                      GENERIC_READ | GENERIC_WRITE, // Read/Write Access
                      0,             // No Sharing, ports cant be shared
                      NULL,          // No Security
                      OPEN_EXISTING, // Open existing port only
                      0,             // Non Overlapped I/O
                      NULL);         // Null for Comm Devices

    if (Comm == INVALID_HANDLE_VALUE) {
      ShowMessages("err, port can't be opened.\n");
      return FALSE;
    }

    //
    // Setting the Parameters for the SerialPort
    //
    SerialParams.DCBlength = sizeof(SerialParams);

    //
    // retreives the current settings
    //
    Status = GetCommState(Comm, &SerialParams);

    if (Status == FALSE) {
      ShowMessages("err, to Get the COM state\n");
      return FALSE;
    }

    SerialParams.BaudRate =
        Baudrate;              // BaudRate = 9600 (Based on user selection)
    SerialParams.ByteSize = 8; // ByteSize = 8
    SerialParams.StopBits = ONESTOPBIT; // StopBits = 1
    SerialParams.Parity = NOPARITY;     // Parity = None
    Status = SetCommState(Comm, &SerialParams);

    if (Status == FALSE) {
      ShowMessages("err, to Setting DCB Structure\n");
      return FALSE;
    }

    //
    // Setting Timeouts
    //
    Timeouts.ReadIntervalTimeout = 50;
    Timeouts.ReadTotalTimeoutConstant = 50;
    Timeouts.ReadTotalTimeoutMultiplier = 10;
    Timeouts.WriteTotalTimeoutConstant = 50;
    Timeouts.WriteTotalTimeoutMultiplier = 10;

    if (SetCommTimeouts(Comm, &Timeouts) == FALSE) {
      ShowMessages("err, to Setting Time outs %d.\n", GetLastError());
      return FALSE;
    }

  } else {
    //
    // It's a namedpipe
    //
    Comm = NamedPipeClientCreatePipe(PortName);

    if (!Comm) {

      //
      // Unable to create handle
      //
      ShowMessages("err, Is virtual machine running?\n");
      return FALSE;
    }
  }

  if (IsPreparing) {

    //
    // It's a debuggee request
    // Prepare the details structure
    //
    DebuggeeRequest.PortAddress = Port;
    DebuggeeRequest.Baudrate = Baudrate;

    //
    // Send the request to the kernel
    //
    StatusIoctl =
        DeviceIoControl(g_DeviceHandle,         // Handle to device
                        IOCTL_PREPARE_DEBUGGEE, // IO Control code
                        &DebuggeeRequest,       // Input Buffer to driver.
                        SIZEOF_DEBUGGER_PREPARE_DEBUGGEE, // Input buffer
                                                          // length
                        &DebuggeeRequest, // Output Buffer from driver.
                        SIZEOF_DEBUGGER_PREPARE_DEBUGGEE, // Length of output
                                                          // buffer in bytes.
                        &ReturnedLength, // Bytes placed in buffer.
                        NULL             // synchronous call
        );

    if (!StatusIoctl) {
      ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
      return FALSE;
    }

    if (DebuggeeRequest.Result == DEBUGEER_OPERATION_WAS_SUCCESSFULL) {
      ShowMessages("The operation was successful\n");
    } else {
      ShowErrorMessage(DebuggeeRequest.Result);
      return FALSE;
    }

    //
    // Create a thread to listen for pauses from the remote debugger
    //
    g_SerialListeningThreadHandle =
        CreateThread(NULL, 0, ListeningSerialPauseThread, Comm, 0, NULL);

    //
    // Finish it here
    //
    return TRUE;

  } else {

    //
    // If we are here, then it's a debugger (not debuggee)
    // let's prepare the debuggee
    //
    KdPrepareSerialConnectionToRemoteSystem(Comm, IsNamedPipe);
  }

  //
  // everything was done
  //
  return TRUE;
}
