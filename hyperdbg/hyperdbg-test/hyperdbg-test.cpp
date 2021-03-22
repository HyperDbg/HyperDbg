/**
 * @file hyperdbg-test.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief perform tests on a remote process (this is the remote process)
 * @details
 * @version 0.1
 * @date 2020-09-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

/**
 * @brief Main function of test process
 *
 * @param argc
 * @param argv
 * @return int
 */
int
main(int argc, char * argv[])
{
    HANDLE    PipeHandle;
    BOOLEAN   SentMessageResult;
    UINT32    ReadBytes;
    UINT32    TestCaseNumber;
    const int BufferSize         = 1024;
    char      Buffer[BufferSize] = "Hey there, Are you HyperDbg?";

    if (argc != 2)
    {
        printf("you should not test functionalities directly, instead use 'test' "
               "command from HyperDbg...\n");
        return 1;
    }

    if (!strcmp(argv[1], "im-hyperdbg"))
    {
        //
        // Perform our shaking with HyperDbg
        //

        //
        // It's not called directly, it's probably from HyperDbg
        //
        PipeHandle = NamedPipeClientCreatePipe("\\\\.\\Pipe\\HyperDbgTests");

        if (!PipeHandle)
        {
            //
            // Unable to create handle
            //
            return 1;
        }

        SentMessageResult =
            NamedPipeClientSendMessage(PipeHandle, Buffer, strlen(Buffer) + 1);

        if (!SentMessageResult)
        {
            //
            // Sending error
            //
            return 1;
        }

        ReadBytes = NamedPipeClientReadMessage(PipeHandle, Buffer, BufferSize);

        if (!ReadBytes)
        {
            //
            // Nothing to read
            //
            return 1;
        }

        if (strcmp(Buffer,
                   "Hello, Dear Test Process... Yes, I'm HyperDbg Debugger :)") ==
            0)
        {
            //
            // *** Connected to the HyperDbg debugger ***
            //

            //
            // Now we should request the test case number from the HyperDbg Debugger
            //
            RtlZeroMemory(Buffer, BufferSize);

            strcpy_s(
                Buffer,
                "Wow! I miss you... Would you plz send me the test case number?");

            SentMessageResult =
                NamedPipeClientSendMessage(PipeHandle, Buffer, strlen(Buffer) + 1);

            if (!SentMessageResult)
            {
                //
                // Sending error
                //
                return 1;
            }

            //
            // Read the test case number
            //
            RtlZeroMemory(Buffer, BufferSize);
            ReadBytes = NamedPipeClientReadMessage(PipeHandle, Buffer, BufferSize);

            if (!ReadBytes)
            {
                //
                // Nothing to read
                //
                return 1;
            }

            TestCaseNumber = *(UINT32 *)Buffer;

            //
            // Dispatch the test case number
            //
            TestCaseDispatcher(TestCaseNumber, PipeHandle);

            //
            // Close the pipe connection
            //
            NamedPipeClientClosePipe(PipeHandle);

            //
            // Make sure to exit the test program
            //
            exit(0);
        }
    }
    else
    {
        printf("you should not test functionalities directly, instead use 'test' "
               "command from HyperDbg...\n");
        return 1;
    }
    return 0;
}

/**
 * @brief Dispatcher to test case numbers on test process
 *
 * @param TestCase Test case number
 * @param PipeHandle Handle of client's pipe connection
 * @return VOID
 */
VOID
TestCaseDispatcher(UINT32 TestCase, HANDLE PipeHandle)
{
    //
    // Dispatch the Test Case Number
    //
    switch (TestCase)
    {
    case DEBUGGER_TEST_USER_MODE_INFINITE_LOOP_THREAD:
        AttachDetachTest(PipeHandle);
        break;
    default:
        printf("err, test-process called with a wrong test case number.\n");
        break;
    }
}
