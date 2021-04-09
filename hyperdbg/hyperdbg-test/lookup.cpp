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
    BOOLEAN                                SentMessageResult;
    PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION KernelInfoArray;
    char                                   SuccessMessage[] = "success";

    printf("start testing event commands...\n");

    KernelInfoArray = (PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION)KernelInformation;

    for (size_t i = 0; i < KernelInformationSize / sizeof(DEBUGGEE_KERNEL_SIDE_TEST_INFORMATION); i++)
    {
        printf("Address : %llx\n", KernelInfoArray[i].Address);
        printf("Tag : %s\n", KernelInfoArray[i].Tag);
    }

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
