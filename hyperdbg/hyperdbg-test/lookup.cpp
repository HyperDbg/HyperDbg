/**
 * @file lookup.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief perform tests and create lookup details
 * @details
 * @version 0.1
 * @date 2021-04-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Test Test epthook on test process
 *
 * @param PipeHandle
 * @return VOID
 */
VOID
TestCreateLookupTable(HANDLE PipeHandle, PVOID KernelInformation, UINT32 KernelInformationSize)
{
    BOOLEAN SentMessageResult;
    UINT64  Buffer[TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE] = {0};
    char    SuccessMessage[]                                 = "success";

    printf("start testing event commands...\n");

    PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION Test = (PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION)KernelInformation;

    printf("heeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeerrrrrrrrreeee\n");
    printf("Ex : %llx\n", Test->Address1);
    printf("Tag : %llx\n", Test->Tag);
    _getch();

    //
    // Send success message to the HyperDbg
    //
    SentMessageResult = NamedPipeClientSendMessage(PipeHandle, (char *)SuccessMessage, sizeof(SuccessMessage));

    if (!SentMessageResult)
    {
        //
        // Sending error
        //
        return;
    }
}
