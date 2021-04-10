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
    ShowMessages("syntax : \ttest\n");

    ShowMessages("\t\te.g : test\n");
}

/**
 * @brief perform test on the remote process
 *
 * @param KernelSideInformation Information from kernel
 * @param KernelSideInformationSize Information from kernel ()
 *
 * @return BOOLEAN returns true if the results was true and false if the results
 * was not ok
 */
BOOLEAN
CommandTestPerformTest(PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION KernelSideInformation, UINT32 KernelSideInformationSize)
{
    BOOLEAN ResultOfTest = FALSE;
    HANDLE  PipeHandle;
    HANDLE  ThreadHandle;
    HANDLE  ProcessHandle;
    UINT32  ReadBytes;
    BOOLEAN SentMessageResult;
    CHAR *  Buffer = {0};

    //
    // Allocate memory
    //
    Buffer = (CHAR *)malloc(TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);
    RtlZeroMemory(Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    //
    // Create tests process to create a thread for us
    //
    if (!CreateProcessAndOpenPipeConnection(
            KernelSideInformation,
            KernelSideInformationSize,
            &PipeHandle,
            &ThreadHandle,
            &ProcessHandle))
    {
        ShowMessages("err, enable to connect to the test process\n");

        free(Buffer);

        return FALSE;
    }

    //
    // ***** Perform test specific routines *****
    //

    //
    // Wait for the result of test to be received
    //
    ReadBytes =
        NamedPipeServerReadClientMessage(PipeHandle, (char *)Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    if (!ReadBytes)
    {
        //
        // Nothing to read
        //
        free(Buffer);

        return FALSE;
    }

    if (strcmp(Buffer, "success") == 0)
    {
        ResultOfTest = TRUE;
    }
    else
    {
        ResultOfTest = FALSE;
    }

    //
    // Close connection and remote process
    //
    CloseProcessAndClosePipeConnection(PipeHandle, ThreadHandle, ProcessHandle);

    free(Buffer);

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
    BOOL  Status;
    ULONG ReturnedLength;
    PDEBUGGEE_KERNEL_SIDE_TEST_INFORMATION
    KernelSideTestInformationRequestArray;

    if (SplittedCommand.size() != 1)
    {
        ShowMessages("incorrect use of 'test'\n\n");
        CommandTestHelp();
        return;
    }

    else if (!g_DeviceHandle)
    {
        ShowMessages("Handle not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

    //
    // *** Read kernel-side debugging information ***
    //
    KernelSideTestInformationRequestArray = (DEBUGGEE_KERNEL_SIDE_TEST_INFORMATION *)malloc(TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    RtlZeroMemory(KernelSideTestInformationRequestArray, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    //
    // Send Ioctl to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                               // Handle to device
        IOCTL_SEND_GET_KERNEL_SIDE_TEST_INFORMATION,  // IO Control code
        KernelSideTestInformationRequestArray,        // Input Buffer to driver.
        SIZEOF_DEBUGGEE_KERNEL_SIDE_TEST_INFORMATION, // Input buffer
                                                      // length
        KernelSideTestInformationRequestArray,        // Output Buffer from driver.
        TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE,     // Length
                                                      // of
                                                      // output
                                                      // buffer
                                                      // in
                                                      // bytes.
        &ReturnedLength,                              // Bytes placed in buffer.
        NULL                                          // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return;
    }

    //
    // Means to check just one command
    //
    if (CommandTestPerformTest(KernelSideTestInformationRequestArray, ReturnedLength))
    {
        ShowMessages("All the test were successful :)\n");
    }
    else
    {
        ShowMessages("At least one tests failed :(\n");
    }

    //
    // Free the kernel-side buffer
    //
    free(KernelSideTestInformationRequestArray);
}
