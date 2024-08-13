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
 * @brief perform test on for all functionalities
 *
 * @return VOID
 */
VOID
CommandTestAllFunctionalities()
{
    HANDLE ThreadHandle;
    HANDLE ProcessHandle;

    //
    // Test command parser
    //
    if (!OpenHyperDbgTestProcess(&ThreadHandle, &ProcessHandle, (CHAR *)"test-command-parser"))
    {
        ShowMessages("err, start HyperDbg test process\n");
        return;
    }
}

/**
 * @brief perform test on the remote process
 *
 * @return BOOLEAN returns true if the results was true and false if the results
 * was not ok
 */
BOOLEAN
CommandTestPerformTest()
{
    BOOLEAN ResultOfTest = FALSE;
    HANDLE  PipeHandle;
    HANDLE  ThreadHandle;
    HANDLE  ProcessHandle;
    UINT32  ReadBytes;
    CHAR *  Buffer = NULL;

    //
    // Allocate memory
    //
    Buffer = (CHAR *)malloc(TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    if (!Buffer)
    {
        ShowMessages("err, enable allocate communication buffer\n");
        return FALSE;
    }

    RtlZeroMemory(Buffer, TEST_CASE_MAXIMUM_BUFFERS_TO_COMMUNICATE);

    //
    // Create tests process to create a thread for us
    //
    if (!CreateProcessAndOpenPipeConnection(&PipeHandle,
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

SendCommandAndWaitForResponse:

    CHAR TestCommand[] = "this is a test command";

    BOOLEAN SentMessageResult = NamedPipeServerSendMessageToClient(
        PipeHandle,
        TestCommand,
        (UINT32)strlen(TestCommand) + 1);

    if (!SentMessageResult)
    {
        //
        // error in sending
        //
        return FALSE;
    }

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

    goto SendCommandAndWaitForResponse;

    //
    // Close connection and remote process
    //
    CloseProcessAndClosePipeConnection(PipeHandle, ThreadHandle, ProcessHandle);

    free(Buffer);

    return ResultOfTest;
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
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandTest(vector<CommandToken> CommandTokens, string Command)
{
    UINT64 Context = NULL;

    UINT32 CommandSize = (UINT32)CommandTokens.size();

    if (CommandSize == 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandTestHelp();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "query"))
    {
        //
        // Query the state of debuggee in debugger mode
        //
        CommandTestQueryState();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "trap-status"))
    {
        //
        // Query the state of trap flag in debugger mode
        //
        CommandTestQueryTrapState();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "pool"))
    {
        //
        // Query the state of pre-allocated pools in debugger mode
        //
        CommandTestQueryPreAllocPoolsState();
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "sync-task"))
    {
        //
        // Send target task to the halted cores in debugger mode (synchronous)
        //
        CommandTestSetTargetTaskToHaltedCores(TRUE);
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "async-task"))
    {
        //
        // Send target task to the halted cores in debugger mode (asynchronous)
        //
        CommandTestSetTargetTaskToHaltedCores(FALSE);
    }
    else if (CommandSize == 3 && CompareLowerCaseStrings(CommandTokens.at(1), "target-core-task"))
    {
        if (!ConvertTokenToUInt64(CommandTokens.at(2), &Context))
        {
            ShowMessages("err, you should enter a valid hex number as the core id\n\n");
            return;
        }

        //
        // Send target task to the specific halted core in debugger mode
        //
        CommandTestSetTargetTaskToTargetCore((UINT32)Context);
    }
    else if (CommandSize == 3 && CompareLowerCaseStrings(CommandTokens.at(1), "breakpoint"))
    {
        //
        // Change breakpoint state
        //
        if (CompareLowerCaseStrings(CommandTokens.at(2), "on"))
        {
            CommandTestSetBreakpointState(TRUE);
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(2), "off"))
        {
            CommandTestSetBreakpointState(FALSE);
        }
        else
        {
            ShowMessages("err, couldn't resolve error at '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)).c_str());
            return;
        }
    }
    else if (CommandSize == 3 && CompareLowerCaseStrings(CommandTokens.at(1), "trap"))
    {
        //
        // Change debug break state
        //
        if (CompareLowerCaseStrings(CommandTokens.at(2), "on"))
        {
            CommandTestSetDebugBreakState(TRUE);
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(2), "off"))
        {
            CommandTestSetDebugBreakState(FALSE);
        }
        else
        {
            ShowMessages("err, couldn't resolve error at '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)).c_str());
            return;
        }
    }
    else if (CommandSize == 2 && CompareLowerCaseStrings(CommandTokens.at(1), "all"))
    {
        //
        // For testing functionalities
        //
        CommandTestAllFunctionalities();
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandTestHelp();
        return;
    }
}
