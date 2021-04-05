/**
 * @file test-epthook.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief perform tests for !epthook command
 * @details
 * @version 0.1
 * @date 2021-04-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global variables in this scope
//

/**
 * @brief Test Test epthook on test process
 *
 * @param PipeHandle
 * @return VOID
 */
VOID
TestEpthook(HANDLE PipeHandle)
{
    BOOLEAN   SentMessageResult;
    const int BufferSize         = 1024 / sizeof(UINT64);
    UINT64    Buffer[BufferSize] = {0};
    UINT32    ProcessId;

    printf("start testing !epthook command...\n");

    ProcessId = GetCurrentProcessId();

    //
    // Send the command to HyperDbg
    //
    strcpy_s((char *)Buffer, 22, "this is test command!");

    SentMessageResult = NamedPipeClientSendMessage(PipeHandle, (char *)Buffer, sizeof(UINT64) * 2);

    if (!SentMessageResult)
    {
        //
        // Sending error
        //
        return;
    }
}
