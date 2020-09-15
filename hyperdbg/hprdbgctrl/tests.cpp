/**
 * @file tests.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief perform tests
 * @details
 * @version 0.1
 * @date 2020-09-14
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern TCHAR g_TestLocation[MAX_PATH];

/**
 * @brief Setup test process name
 *
 * @param ULONG
 * @return BOOLEAN
 */
BOOLEAN
SetupTestName(_Inout_updates_bytes_all_(BufferLength) PCHAR TestLocation,
              ULONG BufferLength) {
  HANDLE fileHandle;
  DWORD driverLocLen = 0;
  HMODULE ProcHandle = GetModuleHandle(NULL);
  char *Pos;

  //
  // Get the current directory.
  //

  /*
  //
  // We use the location of running exe instead of
  // finding driver based on current directory
  //
  driverLocLen = GetCurrentDirectory(BufferLength, DriverLocation);

  if (driverLocLen == 0) {

    ShowMessages("GetCurrentDirectory failed!  Error = %d \n", GetLastError());

    return FALSE;
  }
  */

  GetModuleFileName(ProcHandle, TestLocation, BufferLength);

  Pos = strrchr(TestLocation, '\\');
  if (Pos != NULL) {
    //
    // this will put the null terminator here. you can also copy to
    // another string if you want, we can also use PathCchRemoveFileSpec
    //
    *Pos = '\0';
  }

  //
  // Setup path name to driver file.
  //
  if (FAILED(StringCbCat(TestLocation, BufferLength, "\\" TEST_PROCESS_NAME))) {
    return FALSE;
  }

  //
  // Insure driver file is in the specified directory.
  //
  if ((fileHandle = CreateFile(TestLocation, GENERIC_READ, 0, NULL,
                               OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) ==
      INVALID_HANDLE_VALUE) {

    ShowMessages("%s.exe is not loaded.\n", TEST_PROCESS_NAME);

    //
    // Indicate failure.
    //
    return FALSE;
  }

  //
  // Close open file handle.
  //
  if (fileHandle) {

    CloseHandle(fileHandle);
  }

  //
  // Indicate success.
  //
  return TRUE;
}

BOOLEAN CreateProcessAndOpenPipeConnection() {

  HANDLE PipeHandle;
  BOOLEAN SentMessageResult;
  UINT32 ReadBytes;
  const int BufferSize = 1024;
  char BufferToRead[BufferSize] = {0};
  char BufferToSend[BufferSize] =
      "hello, dear test process, I'm HyperDbg debugger :) !!!";
  PROCESS_INFORMATION ProcessInfo;
  STARTUPINFO StartupInfo;
  char CmdArgs[] = TEST_PROCESS_NAME " im-hyperdbg";

  PipeHandle = NamedPipeServerCreatePipe("\\\\.\\Pipe\\HyperDbgTests",
                                         BufferSize, BufferSize);
  if (!PipeHandle) {

    //
    // Error in creating handle
    //
    return FALSE;
  }

  //
  // Create the Test Process
  //

  ZeroMemory(&StartupInfo, sizeof(StartupInfo));

  //
  // the only compulsory field
  //
  StartupInfo.cb = sizeof StartupInfo;

  //
  // Set-up path
  //
  if (!SetupTestName(g_TestLocation, sizeof(g_TestLocation))) {

    //
    // Test process not found
    //
    return FALSE;
  }

  if (CreateProcess(g_TestLocation, CmdArgs, NULL, NULL, FALSE, 0, NULL, NULL,
                    &StartupInfo, &ProcessInfo)) {

    //WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

    ////////////////////////////////////////////////////////

    //
    // Wait for message from the target processs
    //
    if (!NamedPipeServerWaitForClientConntection(PipeHandle)) {
      //
      // Error in connection
      //
      return FALSE;
    }

    ReadBytes =
        NamedPipeServerReadClientMessage(PipeHandle, BufferToRead, BufferSize);

    if (!ReadBytes) {
      //
      // Nothing to read
      //
      return FALSE;
    }

    printf("Message from client : %s\n", BufferToRead);

    SentMessageResult = NamedPipeServerSendMessageToClient(
        PipeHandle, BufferToSend, strlen(BufferToSend) + 1);

    if (!SentMessageResult) {
      //
      // error in sending
      //
      return FALSE;
    }

    NamedPipeServerCloseHandle(PipeHandle);

    ////////////////////////////////////////////////////////

    CloseHandle(ProcessInfo.hThread);
    CloseHandle(ProcessInfo.hProcess);

  } else {
    ShowMessages("The process could not be started...\n");
  }
}

/**
 * @brief perform test on attaching, stepping and detaching threads
 *
 * @return BOOLEAN returns true if the results was true and false if the results
 * was not ok
 */
BOOLEAN TestInfiniteLoop() {

  //
  // Create tests process to create a thread for us
  //
  if (!CreateProcessAndOpenPipeConnection()) {
    ShowMessages("err, enable to connect to the test process\n");
    return FALSE;
  }

  return FALSE;
}
