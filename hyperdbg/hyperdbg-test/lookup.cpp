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
 * @brief Create lookup table for test
 *
 * @param PipeHandle
 * @param KernelInformation
 * @param KernelInformationSize
 * 
 * @return VOID
 */
VOID
TestCreateLookupTable(HANDLE PipeHandle, PVOID KernelInformation, UINT32 KernelInformationSize)
{
    BOOLEAN                                       SentMessageResult;
    PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION        KernelInfoArray;
    char                                          SuccessMessage[] = "success";
    vector<DEBUGGEE_KERNEL_SIDE_TEST_INFORMATION> LookupTable;
    vector<string>                                TestCases;

    printf("start testing event commands...\n");

    KernelInfoArray = (PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION)KernelInformation;

    //
    // Add kernel-mode details to lookup table
    //
    for (size_t i = 0; i < KernelInformationSize / sizeof(DEBUGGEE_KERNEL_SIDE_TEST_INFORMATION); i++)
    {
        LookupTable.push_back(KernelInfoArray[i]);
    }

    //
    // Add user-mode details to lookup table
    //

    // TODO

    //
    // Read test-cases
    //
    TestCase(TestCases);

    //
    // Replace tags with addresses
    //
    for (auto CurrentCase : TestCases)
    {
        //
        // Template instantiations for
        // extracting the matching pattern
        //
        smatch match;
        regex  r("\\[(.*?)\\]");
        string subject = CurrentCase;
        int    i       = 1;

        while (regex_search(subject, match, r))
        {
            printf("Matched string : %s\n", match.str(0).c_str());

            i++;

            //
            // suffix to find the rest of the string
            //
            subject = match.suffix().str();
        }
        printf("\n\n");
    }

    /*
    for (auto item : LookupTable)
    {
        printf("Address : %llx\n", item.Value);
        printf("Tag : %s\n", item.Tag);
    }
    */

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
