/**
 * @file test.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief test command
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of the test command
 *
 * @return VOID
 */
VOID
CommandTestHelp()
{
    ShowMessages(
        "test : tests essential features of HyperDbg in current machine.\n");

    ShowMessages("syntax : \ttest [Task (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : test\n");
    ShowMessages("\t\te.g : test query\n");
    ShowMessages("\t\te.g : test trap-status\n");
    ShowMessages("\t\te.g : test pool\n");
    ShowMessages("\t\te.g : test query\n");
    ShowMessages("\t\te.g : test breakpoint on\n");
    ShowMessages("\t\te.g : test breakpoint off\n");
    ShowMessages("\t\te.g : test trap on\n");
    ShowMessages("\t\te.g : test trap off\n");
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

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // By the way, we don't need to send an input buffer
    // to the kernel, but let's keep it like this, if we
    // want to pass some other arguments to the kernel in
    // the future
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                       // Handle to device
        IOCTL_PERFROM_KERNEL_SIDE_TESTS,      // IO Control Code (IOCTL)
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

    if (KernelTestRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
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
CommandTestPerformTest(PDEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION KernelSideInformation, UINT32 KernelSideInformationSize)
{
    BOOLEAN ResultOfTest = FALSE;
    HANDLE  PipeHandle;
    HANDLE  ThreadHandle;
    HANDLE  ProcessHandle;
    UINT32  ReadBytes;
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
    else if (Buffer[0] == 'c' &&
             Buffer[1] == 'm' &&
             Buffer[2] == 'd' &&
             Buffer[3] == ':')
    {
        //
        // It's a command as it starts with "cmd:"
        //
        ShowMessages("command is : %s\n", &Buffer[4]);
        HyperDbgInterpreter(&Buffer[4]);
        goto WaitForResponse;
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
 * @brief test command for VMI mode
 *
 * @return VOID
 */
VOID
CommandTestInVmiMode()
{
    BOOL  Status;
    ULONG ReturnedLength;
    PDEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION
    KernelSideTestInformationRequestArray;

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    //
    // *** Read kernel-side debugging information ***
    //
    KernelSideTestInformationRequestArray = (DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION *)malloc(TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    RtlZeroMemory(KernelSideTestInformationRequestArray, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    //
    // Send Ioctl to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                   // Handle to device
        IOCTL_SEND_GET_KERNEL_SIDE_TEST_INFORMATION,      // IO Control Code (IOCTL)
        KernelSideTestInformationRequestArray,            // Input Buffer to driver.
        SIZEOF_DEBUGGEE_KERNEL_AND_USER_TEST_INFORMATION, // Input buffer
                                                          // length
        KernelSideTestInformationRequestArray,            // Output Buffer from driver.
        TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE,         // Length
                                                          // of
                                                          // output
                                                          // buffer
                                                          // in
                                                          // bytes.
        &ReturnedLength,                                  // Bytes placed in buffer.
        NULL                                              // synchronous call
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

/**
 * @brief test command for query the state
 *
 * @return VOID
 */
VOID
CommandTestQueryState()
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
                     "in debugger mode\n");
        return;
    }

    //
    // Send the query to the debuggee
    //
    KdSendTestQueryPacketToDebuggee(TEST_QUERY_HALTING_CORE_STATUS);
}

/**
 * @brief test command for query the trap state
 *
 * @return VOID
 */
VOID
CommandTestQueryTrapState()
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
                     "in debugger mode\n");
        return;
    }

    //
    // Send the query to the debuggee
    //
    KdSendTestQueryPacketToDebuggee(TEST_QUERY_TRAP_STATE);
}

/**
 * @brief test command for query the state of pre-allocated pools
 *
 * @return VOID
 */
VOID
CommandTestQueryPreAllocPoolsState()
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
                     "in debugger mode\n");
        return;
    }

    //
    // Send the query to the debuggee
    //
    KdSendTestQueryPacketToDebuggee(TEST_QUERY_PREALLOCATED_POOL_STATE);
}

/**
 * @brief test command for setting target tasks to halted cores
 * @param Synchronous
 *
 * @return VOID
 */
VOID
CommandTestSetTargetTaskToHaltedCores(BOOLEAN Synchronous)
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
                     "in debugger mode\n");
        return;
    }

    //
    // Send the target tasks to the halted cores
    //
    KdSendTestQueryPacketToDebuggee(Synchronous ? TEST_SETTING_TARGET_TASKS_ON_HALTED_CORES_SYNCHRONOUS : TEST_SETTING_TARGET_TASKS_ON_HALTED_CORES_ASYNCHRONOUS);
}

/**
 * @brief test command for setting target task to the specified core
 * @param CoreNumber
 *
 * @return VOID
 */
VOID
CommandTestSetTargetTaskToTargetCore(UINT32 CoreNumber)
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
                     "in debugger mode\n");
        return;
    }

    //
    // Send the target task to the target halted core
    //
    KdSendTestQueryPacketWithContextToDebuggee(TEST_SETTING_TARGET_TASKS_ON_TARGET_HALTED_CORES, (UINT64)CoreNumber);
}

/**
 * @brief test command for turning on/off the breakpoints (#DB)
 * @param State
 * @return VOID
 */
VOID
CommandTestSetBreakpointState(BOOLEAN State)
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
                     "in debugger mode\n");
        return;
    }

    //
    // Send the breakpoint settings to the debuggee
    //
    if (State)
    {
        KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_ON_BPS);
    }
    else
    {
        KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_OFF_BPS);
    }
}

/**
 * @brief test command for turning on/off the debug breaks (#DB)
 * @param State
 * @return VOID
 */
VOID
CommandTestSetDebugBreakState(BOOLEAN State)
{
    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, query state of the debuggee is only possible when you connected "
                     "in debugger mode\n");
        return;
    }

    //
    // Send the debug break settings to the debuggee
    //
    if (State)
    {
        KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_ON_DBS);
    }
    else
    {
        KdSendTestQueryPacketToDebuggee(TEST_BREAKPOINT_TURN_OFF_DBS);
    }
}

/**
 * @brief test command handler
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandTest(vector<string> SplitCommand, string Command)
{
    UINT64 Context = NULL;

    if (SplitCommand.size() == 1)
    {
        //
        // For testing in vmi mode
        //
        CommandTestInVmiMode();
    }
    else if (SplitCommand.size() == 2 && !SplitCommand.at(1).compare("query"))
    {
        //
        // Query the state of debuggee in debugger mode
        //
        CommandTestQueryState();
    }
    else if (SplitCommand.size() == 2 && !SplitCommand.at(1).compare("trap-status"))
    {
        //
        // Query the state of trap flag in debugger mode
        //
        CommandTestQueryTrapState();
    }
    else if (SplitCommand.size() == 2 && !SplitCommand.at(1).compare("pool"))
    {
        //
        // Query the state of pre-allocated pools in debugger mode
        //
        CommandTestQueryPreAllocPoolsState();
    }
    else if (SplitCommand.size() == 2 && !SplitCommand.at(1).compare("sync-task"))
    {
        //
        // Send target task to the halted cores in debugger mode (synchronous)
        //
        CommandTestSetTargetTaskToHaltedCores(TRUE);
    }
    else if (SplitCommand.size() == 2 && !SplitCommand.at(1).compare("async-task"))
    {
        //
        // Send target task to the halted cores in debugger mode (asynchronous)
        //
        CommandTestSetTargetTaskToHaltedCores(FALSE);
    }
    else if (SplitCommand.size() == 3 && !SplitCommand.at(1).compare("target-core-task"))
    {
        if (!ConvertStringToUInt64(SplitCommand.at(2), &Context))
        {
            ShowMessages("err, you should enter a valid hex number as the core id\n\n");
            return;
        }

        //
        // Send target task to the specific halted core in debugger mode
        //
        CommandTestSetTargetTaskToTargetCore((UINT32)Context);
    }
    else if (SplitCommand.size() == 3 && !SplitCommand.at(1).compare("breakpoint"))
    {
        //
        // Change breakpoint state
        //
        if (!SplitCommand.at(2).compare("on"))
        {
            CommandTestSetBreakpointState(TRUE);
        }
        else if (!SplitCommand.at(2).compare("off"))
        {
            CommandTestSetBreakpointState(FALSE);
        }
        else
        {
            ShowMessages("err, couldn't resolve error at '%s'\n\n", SplitCommand.at(2).c_str());
            return;
        }
    }
    else if (SplitCommand.size() == 3 && !SplitCommand.at(1).compare("trap"))
    {
        //
        // Change debug break state
        //
        if (!SplitCommand.at(2).compare("on"))
        {
            CommandTestSetDebugBreakState(TRUE);
        }
        else if (!SplitCommand.at(2).compare("off"))
        {
            CommandTestSetDebugBreakState(FALSE);
        }
        else
        {
            ShowMessages("err, couldn't resolve error at '%s'\n\n", SplitCommand.at(2).c_str());
            return;
        }
    }
    else
    {
        ShowMessages("incorrect use of the 'test'\n\n");
        CommandTestHelp();
        return;
    }
}
