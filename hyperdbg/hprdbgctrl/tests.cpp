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
 * @brief
 *
 * @param BufferLength Setup test process name
 * @return BOOLEAN
 */
BOOLEAN
SetupTestName(_Inout_updates_bytes_all_(BufferLength) PCHAR TestLocation,
              ULONG                                         BufferLength)
{
    HANDLE  fileHandle;
    DWORD   driverLocLen = 0;
    HMODULE ProcHandle   = GetModuleHandle(NULL);
    char *  Pos;

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
    if (Pos != NULL)
    {
        //
        // this will put the null terminator here. you can also copy to
        // another string if you want, we can also use PathCchRemoveFileSpec
        //
        *Pos = '\0';
    }

    //
    // Setup path name to driver file.
    //
    if (FAILED(StringCbCat(TestLocation, BufferLength, "\\" TEST_PROCESS_NAME)))
    {
        return FALSE;
    }

    //
    // Insure driver file is in the specified directory.
    //
    if ((fileHandle = CreateFile(TestLocation, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) ==
        INVALID_HANDLE_VALUE)
    {
        ShowMessages("%s.exe is not loaded.\n", TEST_PROCESS_NAME);

        //
        // Indicate failure.
        //
        return FALSE;
    }

    //
    // Close open file handle.
    //
    if (fileHandle)
    {
        CloseHandle(fileHandle);
    }

    //
    // Indicate success.
    //
    return TRUE;
}

/**
 * @brief Create a Process And Open Pipe Connection object
 *
 * @param TestCase Test Case Number
 * @param ConnectionPipeHandle Pointer to receive Pipe Handle
 * @param ThreadHandle Pointer to receive Thread Handle
 * @param ProcessHandle Pointer to receive Process Handle
 * @return BOOLEAN
 */
BOOLEAN
CreateProcessAndOpenPipeConnection(UINT32  TestCase,
                                   PHANDLE ConnectionPipeHandle,
                                   PHANDLE ThreadHandle,
                                   PHANDLE ProcessHandle)
{
    HANDLE    PipeHandle;
    BOOLEAN   SentMessageResult;
    UINT32    ReadBytes;
    const int BufferSize               = 1024;
    char      BufferToRead[BufferSize] = {0};
    char      BufferToSend[BufferSize] =
        "Hello, Dear Test Process... Yes, I'm HyperDbg Debugger :)";
    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO         StartupInfo;
    char                CmdArgs[] = TEST_PROCESS_NAME " im-hyperdbg";

    PipeHandle = NamedPipeServerCreatePipe("\\\\.\\Pipe\\HyperDbgTests",
                                           BufferSize,
                                           BufferSize);
    if (!PipeHandle)
    {
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
    if (!SetupTestName(g_TestLocation, sizeof(g_TestLocation)))
    {
        //
        // Test process not found
        //
        return FALSE;
    }

    if (CreateProcess(g_TestLocation, CmdArgs, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &StartupInfo, &ProcessInfo))
    {
        //
        // Wait for message from the target processs
        //
        if (!NamedPipeServerWaitForClientConntection(PipeHandle))
        {
            //
            // Error in connection
            //
            return FALSE;
        }

        ReadBytes =
            NamedPipeServerReadClientMessage(PipeHandle, BufferToRead, BufferSize);

        if (!ReadBytes)
        {
            //
            // Nothing to read
            //
            return FALSE;
        }

        if (strcmp(BufferToRead, "Hey there, Are you HyperDbg?") == 0)
        {
            //
            // *** Connected to the remote process ***
            //

            //
            // Send the hand shake message
            //
            SentMessageResult = NamedPipeServerSendMessageToClient(
                PipeHandle,
                BufferToSend,
                strlen(BufferToSend) + 1);

            if (!SentMessageResult)
            {
                //
                // error in sending
                //
                return FALSE;
            }

            //
            // Receive the request for the test case number
            //
            RtlZeroMemory(BufferToRead, BufferSize);

            ReadBytes = NamedPipeServerReadClientMessage(PipeHandle, BufferToRead, BufferSize);

            if (!ReadBytes)
            {
                //
                // Nothing to read
                //
                return FALSE;
            }

            if (strcmp(BufferToRead, "Wow! I miss you... Would you plz send me the "
                                     "test case number?") == 0)
            {
                {
                    //
                    // Send the test case number
                    //
                    RtlZeroMemory(BufferToSend, BufferSize);
                    *(UINT32 *)BufferToSend = TestCase;
                    SentMessageResult       = NamedPipeServerSendMessageToClient(
                        PipeHandle,
                        BufferToSend,
                        strlen(BufferToSend) + 1);

                    if (!SentMessageResult)
                    {
                        //
                        // error in sending
                        //
                        return FALSE;
                    }
                }
            }

            //
            // Set the output handles
            //
            *ConnectionPipeHandle = PipeHandle;
            *ThreadHandle         = ProcessInfo.hThread;
            *ProcessHandle        = ProcessInfo.hProcess;

            return TRUE;
        }
        else
        {
            ShowMessages("The process could not be started...\n");
            return FALSE;
        }
    }
    return FALSE;
}

/**
 * @brief Close the pipe connection and the remote process
 *
 * @param ConnectionPipeHandle Handle of remote pipe connection
 * @param ThreadHandle Handle of test thread
 * @param ProcessHandle Handle of test process
 * @return VOID
 */
VOID
CloseProcessAndOpenPipeConnection(HANDLE ConnectionPipeHandle,
                                  HANDLE ThreadHandle,
                                  HANDLE ProcessHandle)
{
    //
    // Close the connection and handles
    //
    NamedPipeServerCloseHandle(ConnectionPipeHandle);
    CloseHandle(ThreadHandle);
    CloseHandle(ProcessHandle);
}

/**
 * @brief perform test on attaching, stepping and detaching threads
 *
 * @return BOOLEAN returns true if the results was true and false if the
 * results was not ok
 */
BOOLEAN
TestInfiniteLoop()
{
    HANDLE    PipeHandle;
    HANDLE    ThreadHandle;
    HANDLE    ProcessHandle;
    UINT32    ReadBytes;
    UINT32    TestProcessId;
    UINT32    TestThreadId;
    const int BufferSize         = 1024 / sizeof(UINT64);
    UINT64    Buffer[BufferSize] = {0};

    //
    // Create tests process to create a thread for us
    //
    if (!CreateProcessAndOpenPipeConnection(
            DEBUGGER_TEST_USER_MODE_INFINITE_LOOP_THREAD,
            &PipeHandle,
            &ThreadHandle,
            &ProcessHandle))
    {
        ShowMessages("err, enable to connect to the test process\n");
        return FALSE;
    }

    //
    // ***** Perform test specific routines *****
    //

    //
    // Wait for process id and thread id
    //
    ReadBytes =
        NamedPipeServerReadClientMessage(PipeHandle, (char *)Buffer, BufferSize);

    if (!ReadBytes)
    {
        //
        // Nothing to read
        //
        return FALSE;
    }

    //
    // Read the received buffer into process id and thread id
    //
    TestProcessId = Buffer[0];
    TestThreadId  = Buffer[1];

    ShowMessages("Received Process Id : 0x%x  |  Thread Id : 0x%x \n",
                 TestProcessId,
                 TestThreadId);

    //
    // Attach to the target process (thread)
    //
    Sleep(5000);
    AttachToProcess(TestProcessId, TestThreadId);
    Sleep(10000);
    DetachFromProcess();

    //
    // Close connection and remote process
    //
    CloseProcessAndOpenPipeConnection(PipeHandle, ThreadHandle, ProcessHandle);

    return FALSE;
}
