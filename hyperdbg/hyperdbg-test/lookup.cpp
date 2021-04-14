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
    char                                          SuccessMessage[]    = "success";
    char                                          KernelTestMessage[] = "perform-kernel-test";
    vector<DEBUGGEE_KERNEL_SIDE_TEST_INFORMATION> LookupTable;
    vector<string>                                TestCases;
    int                                           IndexOfTestCasesVector = 0;

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
    if (!TestCase(TestCases))
    {
        //
        // There was an error
        //
        return;
    }

    //
    // Replace tags with addresses
    //

    for (auto CurrentCase : TestCases)
    {
        //
        // Template instantiations for
        // extracting the matching pattern
        //
        smatch Match;
        regex  r("\\[(.*?)\\]");
        string Subject = CurrentCase;

        int i = 1;
        while (regex_search(Subject, Match, r))
        {
            for (auto item : LookupTable)
            {
                string Temp = ConvertToString(item.Tag);
                Temp        = "[" + Temp + "]";

                if (!Temp.compare(Match.str(0)))
                {
                    StringReplace(TestCases[IndexOfTestCasesVector], Temp, Uint64ToString(item.Value));
                }
            }

            //
            // suffix to find the rest of the string
            //
            Subject = Match.suffix().str();
            i++;
        }
        IndexOfTestCasesVector++;
    }

    for (auto NewCase : TestCases)
    {
        printf("new cases : %s\n", NewCase.c_str());
    }

    _getch();

    //
    // Send test kernel message to the HyperDbg
    //
    SentMessageResult = NamedPipeClientSendMessage(PipeHandle, (char *)KernelTestMessage, sizeof(KernelTestMessage));

    if (!SentMessageResult)
    {
        //
        // Sending error
        //
        return;
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
