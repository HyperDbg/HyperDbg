/**
 * @file attachdetach.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief perform tests for attach and detach
 * @details
 * @version 0.1
 * @date 2020-09-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global variables in this scope
//
UINT64 ThreadIncrement     = 0;
UINT64 TempThreadIncrement = 0;
UINT64 ValueToShow         = 0;

/**
 * @brief Test thread function (infinite loop)
 *
 * @param lpParameter Optional parameter
 * @return DWORD
 */
DWORD WINAPI
TestThread(LPVOID lpParameter)
{
    Sleep(100);

    printf("Started\n");

    while (TRUE)
    {
        XorSledFunction();
        ThreadIncrement++;
    }
    printf("Finished\n");

    return 0;
}

/**
 * @brief Test Attach and Detach routines on test process
 *
 * @param PipeHandle
 * @return VOID
 */
VOID
AttachDetachTest(HANDLE PipeHandle)
{
    DWORD     ThreadID;
    HANDLE    ThreadHandle;
    BOOLEAN   SentMessageResult;
    const int BufferSize         = 1024 / sizeof(UINT64);
    UINT64    Buffer[BufferSize] = {0};
    UINT32    ProcessId;

    printf("Testing thread attach and detach process...\n");

    ThreadHandle = CreateThread(0, 0, TestThread, NULL, 0, &ThreadID);

    ProcessId = GetCurrentProcessId();

    printf("Sent Process id : 0x%x  |  Thread id : 0x%x\n\n", ProcessId, ThreadID);

    //
    // Send the process id and thread id to the HyperDbg
    //
    Buffer[0] = ProcessId;
    Buffer[1] = ThreadID;

    SentMessageResult = NamedPipeClientSendMessage(PipeHandle, (char *)Buffer, sizeof(UINT64) * 2);

    if (!SentMessageResult)
    {
        //
        // Sending error
        //
        return;
    }

    while (TRUE)
    {
        if (TempThreadIncrement != ThreadIncrement)
        {
            ValueToShow++;
            TempThreadIncrement = ThreadIncrement;
        }
        Sleep(1000);
        printf("Current Index : %d\n", ValueToShow);
    }

    CloseHandle(ThreadHandle);
}
