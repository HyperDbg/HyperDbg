/**
 * @file test.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief test command
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of test command
 *
 * @return VOID
 */
VOID
CommandTestHelp()
{
    ShowMessages(
        "test : Test essential features of HyperDbg in current machine.\n");
    ShowMessages("syntax : \ttest [test case (hex value)\n");

    ShowMessages("\t\te.g : test\n");
    ShowMessages("\t\te.g : test 0x3\n");
}

/**
 * @brief perform test on the remote process
 *
 * @param TestCaseNum Number of test case
 * @param InvalidTestCase if true then command is invalid, if false then test
 * case is valid
 *
 * @return BOOLEAN returns true if the results was true and false if the results
 * was not ok
 */
BOOLEAN
CommandTestPerformTest(UINT32 TestCaseNum, PBOOLEAN InvalidTestCase)
{
    BOOLEAN ResultOfTest = FALSE;
    *InvalidTestCase     = FALSE;

    switch (TestCaseNum)
    {
    case DEBUGGER_TEST_USER_MODE_INFINITE_LOOP_THREAD:
    {
        ShowMessages("[*] Test attaching to threads, stepping, detaching...\n");
        ResultOfTest = TestInfiniteLoop();
        break;
    }
    default:
        *InvalidTestCase = TRUE;
        ResultOfTest     = FALSE;
        break;
    }

    return ResultOfTest;
}

/**
 * @brief test command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandTest(vector<string> SplittedCommand, string Command)
{
    BOOLEAN GetTestCase     = FALSE;
    BOOLEAN TestEverything  = FALSE;
    BOOLEAN WrongTestCase   = FALSE;
    BOOLEAN Result          = FALSE;
    UINT64  SpecialTestCase = 0;

    if (SplittedCommand.size() > 2)
    {
        ShowMessages("incorrect use of 'test'\n\n");
        CommandTestHelp();
        return;
    }

    //
    // Interpret command specific details (if any)
    //
    for (auto Section : SplittedCommand)
    {
        if (!Section.compare("test"))
        {
            continue;
        }
        else if (!GetTestCase)
        {
            //
            // It's probably a test case number
            //
            if (!ConvertStringToUInt64(Section, &SpecialTestCase))
            {
                //
                // Unkonwn parameter
                //
                ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
                CommandTestHelp();
                return;
            }
            else
            {
                GetTestCase = TRUE;
            }
        }
        else
        {
            //
            // Unkonwn parameter
            //
            ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
            CommandTestHelp();
            return;
        }
    }

    //
    // Perform the test case
    //
    if (SpecialTestCase == DEBUGGER_TEST_ALL_COMMANDS)
    {
        //
        // Means to check everything
        //
        TestEverything = TRUE;
    }

    ShowMessages("---------------------------------------------------------\n");

    //
    // Means to check just one command
    //
    for (size_t i = 1; i < MAXLONG; i++)
    {
        if (TestEverything)
            SpecialTestCase = i;

        Result = CommandTestPerformTest(SpecialTestCase, &WrongTestCase);

        if (WrongTestCase)
        {
            //
            // Wrong test case
            //
            if (TestEverything)
            {
                break;
            }
            else
            {
                ShowMessages("Wrong test-case.\n");
            }
        }
        else
        {
            //
            // Check if this is wrong command or not
            //
            if (Result)
            {
                ShowMessages("\t[+] Successful\n");
            }
            else
            {
                ShowMessages("\t[-] Unsuccessful\n");
            }
        }
        if (!TestEverything)
        {
            break;
        }
    }

    ShowMessages("---------------------------------------------------------\n");
}
