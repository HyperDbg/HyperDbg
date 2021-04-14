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
 * @brief Send an IOCTL to the kernel to run the 
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandTestPerformKernelTestsIoctl()
{
    BOOL                          Status;
    ULONG                         ReturnedLength;
    DEBUGGER_PERFORM_KERNEL_TESTS KernelTestRequest = {0};

    if (!g_DeviceHandle)
    {
        ShowMessages(
            "handle not found, probably the driver is not loaded. Did you "
            "use 'load' command?\n");
        return FALSE;
    }

    //
    // By the way, we don't need to send an input buffer
    // to the kernel, but let's keep it like this, if we
    // want to pass some other aguments to the kernel in
    // the future
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                       // Handle to device
        IOCTL_PERFROM_KERNEL_SIDE_TESTS,      // IO Control code
        &KernelTestRequest,                   // Input Buffer to driver.
        SIZEOF_DEBUGGER_PERFORM_KERNEL_TESTS, // Input buffer length
        &KernelTestRequest,                   // Output Buffer from driver.
        SIZEOF_DEBUGGER_PERFORM_KERNEL_TESTS, // Length of output buffer in
                                              // bytes.
        &ReturnedLength,                      // Bytes placed in buffer.
        NULL                                  // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    if (KernelTestRequest.KernelStatus == DEBUGEER_OPERATION_WAS_SUCCESSFULL)
    {
        //
        // Nothing to show
        //
        return TRUE;
    }
    else
    {
        //
        // Show err message
        //
        ShowErrorMessage(KernelTestRequest.KernelStatus);
        return FALSE;
    }
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

WaitForResponse:

    RtlZeroMemory(Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);
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

    if (strcmp(Buffer, "perform-kernel-test") == 0)
    {
        //
        // Send IOCTL to perform kernel tasks
        //
        CommandTestPerformKernelTestsIoctl();

        goto WaitForResponse;
    }
    else if (strcmp(Buffer, "success") == 0)
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
        ShowMessages("handle of driver , probably the driver is not loaded. Did you "
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
        ShowMessages("all the tests were successful :)\n");
    }
    else
    {
        ShowMessages("all or at least one of the tests failed :(\n");
    }

    //
    // Free the kernel-side buffer
    //
    free(KernelSideTestInformationRequestArray);
}
