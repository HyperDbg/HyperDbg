/**
 * @file debug.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .debug command
 * @details
 * @version 0.1
 * @date 2020-12-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern HANDLE g_SerialListeningThreadHandle;

/**
 * @brief help of .debug command
 *
 * @return VOID
 */
VOID CommandDebugHelp() {
  ShowMessages(".debug : debug target .\n\n");
  ShowMessages("syntax : \t.debug [action (remote | prepare)] [type (serial | "
               "namedpipe)] [baud rate (decimal value)] address \n");
  ShowMessages("\t\te.g : .debug prepare serial 115200 com2\n");
  ShowMessages("\t\te.g : .debug remote serial 115200 com3\n");
  ShowMessages("\t\te.g : .debug remote namedpipe \\\\.\\pipe\\HyperDbgPipe\n");
  ShowMessages(
      "\nvalid baud rates (decimal) : 110, 300, 600, 1200, 2400, 4800, 9600, "
      "14400, 19200, 38400, 56000, 57600, 115200, 128000, 256000\n");
  ShowMessages("valid COM ports : COM1, COM2, COM3, COM4 \n");
}

/**
 * @brief Check if baud rate is valid or not
 *
 * @param Baudrate
 * @return BOOLEAN
 */
BOOLEAN CommandDebugCheckBaudrate(DWORD Baudrate) {

  if (Baudrate == CBR_110 || Baudrate == CBR_300 || Baudrate == CBR_600 ||
      Baudrate == CBR_1200 || Baudrate == CBR_2400 || Baudrate == CBR_4800 ||
      Baudrate == CBR_9600 || Baudrate == CBR_14400 || Baudrate == CBR_19200 ||
      Baudrate == CBR_38400 || Baudrate == CBR_56000 || Baudrate == CBR_57600 ||
      Baudrate == CBR_115200 || Baudrate == CBR_128000 ||
      Baudrate == CBR_256000) {
    return TRUE;
  }
  return FALSE;
}

/**
 * @brief Check if COM port is valid or not
 *
 * @param ComPort
 * @return BOOLEAN
 */
BOOLEAN CommandDebugCheckComPort(string ComPort, UINT32 *Port) {

  if (!ComPort.compare("com1")) {

    *Port = COM1_PORT;
    return TRUE;
  } else if (!ComPort.compare("com2")) {

    *Port = COM2_PORT;
    return TRUE;
  } else if (!ComPort.compare("com3")) {

    *Port = COM3_PORT;
    return TRUE;
  } else if (!ComPort.compare("com4")) {

    *Port = COM4_PORT;
    return TRUE;
  }

  return FALSE;
}

/**
 * @brief Prepare to connect to a named pipe
 *
 * @param PipeName
 */
BOOLEAN CommandDebugConnectNamedPipe(const char *PipeName) { return TRUE; }

/**
 * @brief Prepare and initialize COM port
 *
 * @param PortName
 * @param Baudrate
 * @param Port
 * @param IsPreparing
 * @return BOOLEAN
 */
BOOLEAN CommandDebugPrepareAndConnectDebugPort(const char *PortName,
                                               DWORD Baudrate, UINT32 Port,
                                               BOOLEAN IsPreparing) {

  HANDLE Comm;                 /* Handle to the Serial port */
  BOOL Status;                 /* Status */
  DCB SerialParams = {0};      /* Initializing DCB structure */
  COMMTIMEOUTS Timeouts = {0}; /* Initializing timeouts structure */
  char SerialBuffer[64] = {0}; /* Buffer to send and receive data */
  DWORD BytesWritten = 0;      /* No of bytes written to the port */
  DWORD EventMask;             /* Event mask to trigger */
  char ReadData;               /* temperory Character */
  DWORD NoBytesRead;           /* Bytes read by ReadFile() */
  char PortNo[20] = {0};       /* contain friendly name */
  unsigned char Loop = 0;
  BOOLEAN StatusIoctl;
  ULONG ReturnedLength;
  DEBUGGER_PREPARE_DEBUGGEE DebuggeeRequest = {0};

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
  // retreives  the current settings
  //
  Status = GetCommState(Comm, &SerialParams);

  if (Status == FALSE) {
    ShowMessages("err, to Get the COM state\n");
    return FALSE;
  }

  SerialParams.BaudRate = Baudrate; // BaudRate = 9600 (Based on user selection)
  SerialParams.ByteSize = 8;        // ByteSize = 8
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
    ShowMessages("err, to Setting Time outs.\n");
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
      ShowMessages("The operation was successful.\n");
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
  }

  //
  // If we are here, then it's a debugger (not debuggee)
  //

  //
  // everything was done
  //
  return TRUE;
}

/**
 * @brief .debug command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID CommandDebug(vector<string> SplittedCommand, string Command) {

  UINT32 Baudrate;
  UINT32 Port;

  if (SplittedCommand.size() <= 3) {
    ShowMessages("incorrect use of '.debug'\n\n");
    CommandDebugHelp();
    return;
  }

  //
  // Check the main command
  //
  if (!SplittedCommand.at(1).compare("remote")) {

    //
    // in the case of 'remote'
    //

    if (!SplittedCommand.at(2).compare("serial")) {

      //
      // Connect to a remote serial device
      //
      if (SplittedCommand.size() != 5) {
        ShowMessages("incorrect use of '.debug'\n\n");
        CommandDebugHelp();
        return;
      }

      //
      // Set baudrate
      //
      if (!IsNumber(SplittedCommand.at(3))) {

        //
        // Unkonwn parameter
        //
        ShowMessages("Unknown parameter '%s'\n\n",
                     SplittedCommand.at(3).c_str());
        CommandDebugHelp();
        return;
      }

      Baudrate = stoi(SplittedCommand.at(3));

      //
      // Check if baudrate is valid or not
      //
      if (!CommandDebugCheckBaudrate(Baudrate)) {

        //
        // Baud-rate is invalid
        //
        ShowMessages("err, baud rate is invalid\n\n");
        CommandDebugHelp();
        return;
      }

      //
      // check if com port address is valid or not
      //
      if (!CommandDebugCheckComPort(SplittedCommand.at(4), &Port)) {
        //
        // com port is invalid
        //
        ShowMessages("err, COM port is invalid\n\n");
        CommandDebugHelp();
        return;
      }

      //
      // Everything is okay, connect to remote machine to send
      //
      CommandDebugPrepareAndConnectDebugPort(SplittedCommand.at(4).c_str(),
                                             Baudrate, Port, FALSE);

    } else if (!SplittedCommand.at(2).compare("namedpipe")) {

      //
      // Connect to a remote namedpipe
      //
      string Delimiter = "namedpipe";
      string Token = Command.substr(
          Command.find(Delimiter) + Delimiter.size() + 1, Command.size());

      //
      // Connect to a namedpipe (it's probably a Virtual Machine debugging)
      //
      CommandDebugConnectNamedPipe(Token.c_str());

    } else {

      //
      // Unkonwn parameter
      //
      ShowMessages("Unknown parameter '%s'\n\n", SplittedCommand.at(2).c_str());
      CommandDebugHelp();
      return;
    }

  } else if (!SplittedCommand.at(1).compare("prepare")) {

    if (SplittedCommand.size() != 5) {
      ShowMessages("incorrect use of '.debug'\n\n");
      CommandDebugHelp();
      return;
    }

    //
    // in the case of 'prepare'
    // currently we only support serial
    //
    if (!SplittedCommand.at(2).compare("serial")) {

      //
      // Set baudrate
      //
      if (!IsNumber(SplittedCommand.at(3))) {

        //
        // Unkonwn parameter
        //
        ShowMessages("Unknown parameter '%s'\n\n",
                     SplittedCommand.at(3).c_str());
        CommandDebugHelp();
        return;
      }

      Baudrate = stoi(SplittedCommand.at(3));

      //
      // Check if baudrate is valid or not
      //
      if (!CommandDebugCheckBaudrate(Baudrate)) {

        //
        // Baud-rate is invalid
        //
        ShowMessages("err, baud rate is invalid\n\n");
        CommandDebugHelp();
        return;
      }

      //
      // check if com port address is valid or not
      //
      if (!CommandDebugCheckComPort(SplittedCommand.at(4), &Port)) {
        //
        // com port is invalid
        //
        ShowMessages("err, COM port is invalid\n\n");
        CommandDebugHelp();
        return;
      }

      //
      // Everything is okay, prepare to send
      //
      CommandDebugPrepareAndConnectDebugPort(SplittedCommand.at(4).c_str(),
                                             Baudrate, Port, TRUE);

    } else {
      ShowMessages("invalid parameter '%s'\n\n", SplittedCommand.at(2));
      CommandDebugHelp();
      return;
    }
  } else {
    ShowMessages("invalid parameter '%s'\n\n", SplittedCommand.at(1));
    CommandDebugHelp();
    return;
  }
}
