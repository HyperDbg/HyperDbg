/**
 * @file ud.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief control the user-mode debugging affairs
 * @details
 * @version 0.1
 * @date 2021-12-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;
extern BOOLEAN                  g_IsUserDebuggerInitialized;
extern DEBUGGER_SYNCRONIZATION_EVENTS_STATE
    g_UserSyncronizationObjectsHandleTable[DEBUGGER_MAXIMUM_SYNCRONIZATION_USER_DEBUGGER_OBJECTS];

/**
 * @brief Initialize the user debugger in user mode
 * 
 * @return VOID
 */
VOID
UdInitializeUserDebugger()

{
    if (!g_IsUserDebuggerInitialized)
    {
        //
        // Initialize the handle table
        //
        for (size_t i = 0; i < DEBUGGER_MAXIMUM_SYNCRONIZATION_USER_DEBUGGER_OBJECTS; i++)
        {
            g_UserSyncronizationObjectsHandleTable[i].IsOnWaitingState = FALSE;
            g_UserSyncronizationObjectsHandleTable[i].EventHandle =
                CreateEvent(NULL, FALSE, FALSE, NULL);
        }

        //
        // Indicate that it's initialized
        //
        g_IsUserDebuggerInitialized = TRUE;
    }
}

/**
 * @brief Uninitialize the user debugger in user mode
 * 
 * @return VOID
 */
VOID
UdUninitializeUserDebugger()

{
    if (g_IsUserDebuggerInitialized)
    {
        //
        // Remove the active process
        //
        UdRemoveActiveDebuggingProcess(TRUE);

        //
        // Initialize the handle table
        //
        for (size_t i = 0; i < DEBUGGER_MAXIMUM_SYNCRONIZATION_USER_DEBUGGER_OBJECTS; i++)
        {
            if (g_UserSyncronizationObjectsHandleTable[i].EventHandle != NULL)
            {
                if (g_UserSyncronizationObjectsHandleTable[i].IsOnWaitingState)
                {
                    DbgReceivedUserResponse(i);
                }

                CloseHandle(g_UserSyncronizationObjectsHandleTable[i].EventHandle);
                g_UserSyncronizationObjectsHandleTable[i].EventHandle = NULL;
            }
        }

        //
        // Indicate that user debuggger is not initialized
        //
        g_IsUserDebuggerInitialized = FALSE;
    }
}

/**
 * @brief set the current active debugging process (thread)
 * @param DebuggingId
 * @param ProcessId
 * @param ThreadId
 * @param Is32Bit
 * 
 * @return VOID
 */
VOID
UdSetActiveDebuggingProcess(UINT64  DebuggingId,
                            UINT32  ProcessId,
                            UINT32  ThreadId,
                            BOOLEAN Is32Bit,
                            BOOLEAN IsPaused)
{
    g_ActiveProcessDebuggingState.ProcessId             = ProcessId;
    g_ActiveProcessDebuggingState.ThreadId              = ThreadId;
    g_ActiveProcessDebuggingState.Is32Bit               = Is32Bit;
    g_ActiveProcessDebuggingState.ProcessDebuggingToken = DebuggingId;

    //
    // Set pausing state
    //
    g_ActiveProcessDebuggingState.IsPaused = IsPaused;

    //
    // Activate the debugging
    //
    g_ActiveProcessDebuggingState.IsActive = TRUE;
}

/**
 * @brief Remove the current active debugging process (thread)
 * @param DontSwitchToNewProcess 
 * 
 * @return VOID
 */
VOID
UdRemoveActiveDebuggingProcess(BOOLEAN DontSwitchToNewProcess)
{
    //
    // Activate the debugging
    //
    g_ActiveProcessDebuggingState.IsActive = FALSE;
}

/**
 * @brief print error messages relating to the finding thread id
 * @details this function is used only in the scope of Thread32First
 * 
 * @return VOID
 */
VOID
UdPrintError()
{
    DWORD   ErrNum;
    TCHAR   SysMsg[256];
    TCHAR * p;

    ErrNum = GetLastError();
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  ErrNum,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                  SysMsg,
                  256,
                  NULL);

    //
    // Trim the end of the line and terminate it with a null
    //
    p = SysMsg;
    while ((*p > 31) || (*p == 9))
        ++p;
    do
    {
        *p-- = 0;
    } while ((p >= SysMsg) && ((*p == '.') || (*p < 33)));

    //
    // Display the message
    //
    ShowMessages("\n  WARNING: Thread32First failed with error (%x:%s)", ErrNum, SysMsg);
}

/**
 * @brief List of threads by owner process id
 *
 * @param OwnerPID
 * @return BOOL if there was an error then returns false, otherwise return true
 */
BOOL
UdListProcessThreads(DWORD OwnerPID)
{
    HANDLE        ThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 Te32;

    //
    // Take a snapshot of all running threads
    //
    ThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (ThreadSnap == INVALID_HANDLE_VALUE)
        return FALSE;

    //
    // Fill in the size of the structure before using it
    //
    Te32.dwSize = sizeof(THREADENTRY32);

    //
    // Retrieve information about the first thread and exit if unsuccessful
    //
    if (!Thread32First(ThreadSnap, &Te32))
    {
        UdPrintError();          // Show cause of failure
        CloseHandle(ThreadSnap); // Must clean up the snapshot object!
        return FALSE;
    }
    ShowMessages("\nThread's of pid\t= 0x%08X\n", OwnerPID);

    //
    // Now walk the thread list of the system, and display information
    // about each thread associated with the specified process
    //
    do
    {
        if (Te32.th32OwnerProcessID == OwnerPID)
        {
            ShowMessages("\n     Thread Id\t= 0x%08X", Te32.th32ThreadID);
            // ShowMessages("\n     base priority  = %x", Te32.tpBasePri);
            // ShowMessages("\n     delta priority = %x", Te32.tpDeltaPri);
        }
    } while (Thread32Next(ThreadSnap, &Te32));

    ShowMessages("\n");

    //
    // Don't forget to clean up the snapshot object
    //
    CloseHandle(ThreadSnap);
    return TRUE;
}

/**
 * @brief Check if a thread belongs to special process
 *
 * @param Pid Process id
 * @param Tid Thread id
 * @return BOOLEAN if the thread belongs to process then return true; otherwise
 * returns false
 */
BOOLEAN
UdCheckThreadByProcessId(DWORD Pid, DWORD Tid)
{
    HANDLE        ThreadSnap = INVALID_HANDLE_VALUE;
    THREADENTRY32 Te32;
    BOOLEAN       Result = FALSE;

    //
    // Take a snapshot of all running threads
    //
    ThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (ThreadSnap == INVALID_HANDLE_VALUE)
        return FALSE;

    //
    // Fill in the size of the structure before using it
    //
    Te32.dwSize = sizeof(THREADENTRY32);

    //
    // Retrieve information about the first thread and exit if unsuccessful
    //
    if (!Thread32First(ThreadSnap, &Te32))
    {
        UdPrintError();          // Show cause of failure
        CloseHandle(ThreadSnap); // Must clean up the snapshot object!
        return FALSE;
    }

    //
    // Now walk the thread list of the system, and display information
    // about each thread associated with the specified process
    //
    do
    {
        if (Te32.th32OwnerProcessID == Pid)
        {
            if (Te32.th32ThreadID == Tid)
            {
                //
                // The thread found in target process
                //
                Result = TRUE;
                break;
            }
        }
    } while (Thread32Next(ThreadSnap, &Te32));

    //
    // Don't forget to clean up the snapshot object
    //
    CloseHandle(ThreadSnap);
    return Result;
}

/**
 * @brief Attach to a target process
 * @param FileName
 * @param CommandLine
 * @param ProcessInformation
 * 
 * @return BOOLEAN
 */
BOOLEAN
UdCreateSuspendedProcess(const WCHAR * FileName, WCHAR * CommandLine, PPROCESS_INFORMATION ProcessInformation)
{
    STARTUPINFOW StartupInfo;
    BOOL         CreateProcessResult;

    memset(&StartupInfo, 0, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(STARTUPINFOA);

    //
    // Create process suspended
    //
    CreateProcessResult = CreateProcessW(FileName,
                                         CommandLine,
                                         NULL,
                                         NULL,
                                         FALSE,
                                         CREATE_SUSPENDED,
                                         NULL,
                                         NULL,
                                         &StartupInfo,
                                         ProcessInformation);

    if (!CreateProcessResult)
    {
        ShowMessages("err, start process failed (%x)", GetLastError());
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Attach to target process
 * @details this function will not check whether the process id  is valid or not
 *
 * @param TargetPid
 * @param TargetFileAddress
 * @param CommandLine
 * @return BOOLEAN
 */
BOOLEAN
UdAttachToProcess(UINT32        TargetPid,
                  const WCHAR * TargetFileAddress,
                  WCHAR *       CommandLine)
{
    BOOLEAN                                  Status;
    ULONG                                    ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest = {0};
    PROCESS_INFORMATION                      ProcInfo      = {0};

    //
    // Check to initialize the user-debugger
    //
    UdInitializeUserDebugger();

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // Check whether it's starting a new process or not
    //
    if (TargetFileAddress == NULL)
    {
        AttachRequest.IsStartingNewProcess = FALSE;
    }
    else
    {
        AttachRequest.IsStartingNewProcess = TRUE;
    }

    //
    // We wanna attach to a remote process
    //
    AttachRequest.Action = DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_ATTACH;

    if (AttachRequest.IsStartingNewProcess)
    {
        //
        // Check if file exists or not
        //
        if (!IsFileExistW(TargetFileAddress))
        {
            ShowMessages("err, unable to start, file not found\n");
            return FALSE;
        }

        //
        // Start the process in suspended state
        //
        UdCreateSuspendedProcess(TargetFileAddress, CommandLine, &ProcInfo);

        //
        // Set the process id and thread id
        //
        AttachRequest.ProcessId = ProcInfo.dwProcessId;
        AttachRequest.ThreadId  = ProcInfo.dwThreadId;
    }
    else
    {
        //
        // Set the process id
        //
        AttachRequest.ProcessId = TargetPid;
    }

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                  // Handle to device
        IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,  // IO Control
                                                         // code
        &AttachRequest,                                  // Input Buffer to driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
        &AttachRequest,                                  // Output Buffer from driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                         // buffer in bytes.
        &ReturnedLength,                                 // Bytes placed in buffer.
        NULL                                             // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Check if attaching was successful then we can set the attached to true
    //
    if (AttachRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        if (!AttachRequest.IsStartingNewProcess)
        {
            //
            // it's a .attach command, no need for further action
            //
            ShowMessages("successfully attached to the target process!\n"
                         "please keep interacting with the process until all the "
                         "threads are intercepted and halted; whenever you execute "
                         "the first command, the thread interception will be stopped\n");
            return TRUE;
        }

        // ShowMessages("Base Address : %llx\n", AttachRequest.BaseAddressOfMainModule);
        // ShowMessages("Entrypoint Address : %llx\n", AttachRequest.EntrypoinOfMainModule);

        //
        // Resume the suspended process
        //
        ResumeThread(ProcInfo.hThread);

        //
        // *** Remove the hooks ***
        //

        while (TRUE)
        {
            //
            // Send the previous request with removing hook as the action
            //
            AttachRequest.Action = DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_REMOVE_HOOKS;

            //
            // Send the request to the kernel
            //
            Status = DeviceIoControl(
                g_DeviceHandle,                                  // Handle to device
                IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,  // IO Control
                                                                 // code
                &AttachRequest,                                  // Input Buffer to driver.
                SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
                &AttachRequest,                                  // Output Buffer from driver.
                SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                                 // buffer in bytes.
                &ReturnedLength,                                 // Bytes placed in buffer.
                NULL                                             // synchronous call
            );

            if (!Status)
            {
                ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
                return FALSE;
            }

            //
            // Check whether the result of removing hooks was successful or we should
            // re-send the request
            //
            if (AttachRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
            {
                //
                // The hook is remove successfuly
                //
                break;
            }
            else if (AttachRequest.Result == DEBUGGER_ERROR_UNABLE_TO_REMOVE_HOOKS_ENTRYPOINT_NOT_REACHED)
            {
                //
                // Wait for a while until the Windows call the entrypoint
                //
                // ShowMessages("entrypoint is not reached, continue sending the request...\n");

                Sleep(1000);
                continue;
            }
            else
            {
                //
                // An error happend, we should not continue
                //
                ShowErrorMessage(AttachRequest.Result);
                return FALSE;
            }
        }

        //
        // The operation of attaching was successful
        //
        return TRUE;
    }
    else
    {
        ShowErrorMessage(AttachRequest.Result);
        return FALSE;
    }

    return FALSE;
}

/**
 * @brief Kill the target process from kernel
 * @details this function will not check whether the process id and
 * thread id is valid or not
 *
 * @param TargetPid
 * 
 * @return BOOLEAN
 */
BOOLEAN
UdKillProcess(UINT32 TargetPid)
{
    BOOLEAN                                  Status;
    ULONG                                    ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS KillRequest = {0};

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // We wanna kill a process
    //
    KillRequest.Action = DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_KILL_PROCESS;

    //
    // Set the process id
    //
    KillRequest.ProcessId = TargetPid;

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                  // Handle to device
        IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,  // IO Control
                                                         // code
        &KillRequest,                                    // Input Buffer to driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
        &KillRequest,                                    // Output Buffer from driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                         // buffer in bytes.
        &ReturnedLength,                                 // Bytes placed in buffer.
        NULL                                             // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Check if killing was successful or not
    //
    if (KillRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        //
        // Remove the current active debugging process (thread)
        //
        UdRemoveActiveDebuggingProcess(FALSE);

        //
        // The operation of attaching was successful
        //
        return TRUE;
    }
    else
    {
        ShowErrorMessage(KillRequest.Result);
        return FALSE;
    }
}

/**
 * @brief Detach the target process
 * @details this function will not check whether the process id and
 * thread id is valid or not
 *
 * @param TargetPid
 * @param ProcessDetailToken
 * 
 * @return BOOLEAN
 */
BOOLEAN
UdDetachProcess(UINT32 TargetPid, UINT64 ProcessDetailToken)
{
    BOOLEAN                                  Status;
    ULONG                                    ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS DetachRequest = {0};

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // Send the continue command to the target process as we
    // want to continue the debuggee process before detaching
    //
    UdContinueDebuggee(ProcessDetailToken);

    //
    // We wanna detach a process
    //
    DetachRequest.Action = DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_DETACH;

    //
    // Set the process id
    //
    DetachRequest.ProcessId = TargetPid;

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                  // Handle to device
        IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,  // IO Control
                                                         // code
        &DetachRequest,                                  // Input Buffer to driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
        &DetachRequest,                                  // Output Buffer from driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                         // buffer in bytes.
        &ReturnedLength,                                 // Bytes placed in buffer.
        NULL                                             // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Check if detaching was successful or not
    //
    if (DetachRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        //
        // Remove the current active debugging process (thread)
        //
        UdRemoveActiveDebuggingProcess(FALSE);

        //
        // The operation of attaching was successful
        //
        return TRUE;
    }
    else
    {
        ShowErrorMessage(DetachRequest.Result);
        return FALSE;
    }
}

/**
 * @brief Pause the target process
 *
 * @param ProcessDebuggingToken
 * 
 * @return BOOLEAN
 */
BOOLEAN
UdPauseProcess(UINT64 ProcessDebuggingToken)
{
    BOOLEAN                                  Status;
    ULONG                                    ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS PauseRequest = {0};

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // We wanna pause a process
    //
    PauseRequest.Action = DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_PAUSE_PROCESS;

    //
    // Set the process debugging token
    //
    PauseRequest.Token = ProcessDebuggingToken;

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                  // Handle to device
        IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,  // IO Control
                                                         // code
        &PauseRequest,                                   // Input Buffer to driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
        &PauseRequest,                                   // Output Buffer from driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                         // buffer in bytes.
        &ReturnedLength,                                 // Bytes placed in buffer.
        NULL                                             // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Check if killing was successful or not
    //
    if (PauseRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        //
        // The operation of attaching was successful
        //
        return TRUE;
    }
    else
    {
        ShowErrorMessage(PauseRequest.Result);
        return FALSE;
    }
}

/**
 * @brief Send the command to the user debugger
 * @param ProcessDetailToken
 * @param ThreadId
 * @param ActionType
 * @param ApplyToAllPausedThreads
 * @param OptionalParam1
 * @param OptionalParam2
 * @param OptionalParam3
 * @param OptionalParam4
 * 
 * @return VOID
 */
VOID
UdSendCommand(UINT64                          ProcessDetailToken,
              UINT32                          ThreadId,
              DEBUGGER_UD_COMMAND_ACTION_TYPE ActionType,
              BOOLEAN                         ApplyToAllPausedThreads,
              UINT64                          OptionalParam1,
              UINT64                          OptionalParam2,
              UINT64                          OptionalParam3,
              UINT64                          OptionalParam4)
{
    BOOL                       Status;
    ULONG                      ReturnedLength;
    DEBUGGER_UD_COMMAND_PACKET CommandPacket;

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

    //
    // Zero the packet
    //
    RtlZeroMemory(&CommandPacket, sizeof(DEBUGGER_UD_COMMAND_PACKET));

    //
    // Set to the details
    //
    CommandPacket.ProcessDebuggingDetailToken = ProcessDetailToken;
    CommandPacket.ApplyToAllPausedThreads     = ApplyToAllPausedThreads;
    CommandPacket.TargetThreadId              = ThreadId;
    CommandPacket.UdAction.ActionType         = ActionType;
    CommandPacket.UdAction.OptionalParam1     = OptionalParam1;
    CommandPacket.UdAction.OptionalParam2     = OptionalParam2;
    CommandPacket.UdAction.OptionalParam3     = OptionalParam3;
    CommandPacket.UdAction.OptionalParam4     = OptionalParam4;

    //
    // Send IOCTL
    //
    Status =
        DeviceIoControl(g_DeviceHandle,                     // Handle to device
                        IOCTL_SEND_USER_DEBUGGER_COMMANDS,  // IO Control code
                        &CommandPacket,                     // Input Buffer to driver.
                        sizeof(DEBUGGER_UD_COMMAND_PACKET), // Input buffer length
                        &CommandPacket,                     // Output Buffer from driver.
                        sizeof(DEBUGGER_UD_COMMAND_PACKET), // Length of output buffer in bytes.
                        &ReturnedLength,                    // Bytes placed in buffer.
                        NULL                                // synchronous call
        );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return;
    }
}

/**
 * @brief Continue the target user debugger
 * @param ProcessDetailToken
 * 
 * @return VOID
 */
VOID
UdContinueDebuggee(UINT64 ProcessDetailToken)
{
    //
    // Send the 'continue' command
    //
    UdSendCommand(ProcessDetailToken,
                  NULL,
                  DEBUGGER_UD_COMMAND_ACTION_TYPE_CONTINUE,
                  TRUE,
                  NULL,
                  NULL,
                  NULL,
                  NULL);
}

/**
 * @brief Send stepping instructions packet to user debugger
 * @param ProcessDetailToken
 * @param TargetThreadId
 * @param StepType
 * 
 * @return VOID
 */
VOID
UdSendStepPacketToDebuggee(UINT64 ProcessDetailToken, UINT32 TargetThreadId, DEBUGGER_REMOTE_STEPPING_REQUEST StepType)
{
    //
    // Wait until the result of user-input received
    //
    g_UserSyncronizationObjectsHandleTable
        [DEBUGGER_SYNCRONIZATION_OBJECT_USER_DEBUGGER_IS_DEBUGGER_RUNNING]
            .IsOnWaitingState = TRUE;

    //
    // Send the 'continue' command
    //
    UdSendCommand(ProcessDetailToken,
                  TargetThreadId,
                  DEBUGGER_UD_COMMAND_ACTION_TYPE_REGULAR_STEP,
                  FALSE,
                  StepType,
                  NULL,
                  NULL,
                  NULL);

    WaitForSingleObject(
        g_UserSyncronizationObjectsHandleTable
            [DEBUGGER_SYNCRONIZATION_OBJECT_USER_DEBUGGER_IS_DEBUGGER_RUNNING]
                .EventHandle,
        INFINITE);
}

/**
 * @brief Set the active debugging thread by process id or thread id 
 *
 * @param TargetPidOrTid
 * @param IsTid
 * 
 * @return BOOLEAN
 */
BOOLEAN
UdSetActiveDebuggingThreadByPidOrTid(UINT32 TargetPidOrTid, BOOLEAN IsTid)
{
    BOOLEAN                                  Status;
    ULONG                                    ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS SwitchRequest = {0};

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // We wanna switch to a process or thread
    //
    SwitchRequest.Action = DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_SWITCH_BY_PROCESS_OR_THREAD;

    //
    // Set the process id or thread id
    //
    if (IsTid)
    {
        SwitchRequest.ThreadId = TargetPidOrTid;
    }
    else
    {
        SwitchRequest.ProcessId = TargetPidOrTid;
    }

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                  // Handle to device
        IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,  // IO Control
                                                         // code
        &SwitchRequest,                                  // Input Buffer to driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
        &SwitchRequest,                                  // Output Buffer from driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                         // buffer in bytes.
        &ReturnedLength,                                 // Bytes placed in buffer.
        NULL                                             // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Check if killing was successful or not
    //
    if (SwitchRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        //
        // Set the current active debugging process (thread)
        //
        UdSetActiveDebuggingProcess(SwitchRequest.Token,
                                    SwitchRequest.ProcessId,
                                    SwitchRequest.ThreadId,
                                    SwitchRequest.Is32Bit,
                                    SwitchRequest.IsPaused);

        //
        // The operation of attaching was successful
        //
        return TRUE;
    }
    else
    {
        ShowErrorMessage(SwitchRequest.Result);
        return FALSE;
    }
}

/**
 * @brief Show list of active debugging processes and threads 
 *
 * @return BOOLEAN
 */
BOOLEAN
UdShowListActiveDebuggingProcessesAndThreads()
{
    BOOLEAN                                              Status;
    BOOLEAN                                              CheckCurrentProcessOrThread;
    ULONG                                                ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS             QueryCountOfActiveThreadsRequest        = {0};
    USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS * AddressOfThreadsAndProcessDetails       = NULL;
    UINT32                                               SizeOfBufferForThreadsAndProcessDetails = NULL;

    //
    // Check if debugger is loaded or not
    //
    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // Check if user debugger is active or not
    //
    if (!g_IsUserDebuggerInitialized)
    {
        ShowMessages("user debugger is not initialized. Did you use the '.attach' or the '.start' "
                     "command before?\n");
        return FALSE;
    }

    //
    // We wanna query the count of active debugging threads
    //
    QueryCountOfActiveThreadsRequest.Action = DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS_ACTION_QUERY_COUNT_OF_ACTIVE_DEBUGGING_THREADS;

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                  // Handle to device
        IOCTL_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS,  // IO Control
                                                         // code
        &QueryCountOfActiveThreadsRequest,               // Input Buffer to driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Input buffer length
        &QueryCountOfActiveThreadsRequest,               // Output Buffer from driver.
        SIZEOF_DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS, // Length of output
                                                         // buffer in bytes.
        &ReturnedLength,                                 // Bytes placed in buffer.
        NULL                                             // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    //
    // Query was successful
    //
    if (QueryCountOfActiveThreadsRequest.Result == DEBUGGER_OPERATION_WAS_SUCCESSFULL)
    {
        if (QueryCountOfActiveThreadsRequest.CountOfActiveDebuggingThreadsAndProcesses == 0)
        {
            ShowMessages("no active debugging threads!\n");
        }
        else
        {
            //
            // *** We should send another IOCTL and get the list of threads ***
            //

            //
            // Allocate the storage for the pull details of threads and processes
            //
            SizeOfBufferForThreadsAndProcessDetails = QueryCountOfActiveThreadsRequest.CountOfActiveDebuggingThreadsAndProcesses * SIZEOF_USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS;

            AddressOfThreadsAndProcessDetails = (USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS *)
                malloc(SizeOfBufferForThreadsAndProcessDetails);

            RtlZeroMemory(AddressOfThreadsAndProcessDetails, SizeOfBufferForThreadsAndProcessDetails);

            //
            // Send the request to the kernel
            //
            Status = DeviceIoControl(
                g_DeviceHandle,                                   // Handle to device
                IOCTL_GET_DETAIL_OF_ACTIVE_THREADS_AND_PROCESSES, // IO Control
                                                                  // code
                NULL,                                             // Input Buffer to driver.
                0,                                                // Input buffer length.
                AddressOfThreadsAndProcessDetails,                // Output Buffer from driver.
                SizeOfBufferForThreadsAndProcessDetails,          // Length of output buffer in bytes.
                &ReturnedLength,                                  // Bytes placed in buffer.
                NULL                                              // synchronous call
            );

            if (!Status)
            {
                ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
                return FALSE;
            }

            //
            // Show list of active processes and threads
            //
            for (size_t i = 0; i < QueryCountOfActiveThreadsRequest.CountOfActiveDebuggingThreadsAndProcesses; i++)
            {
                if (AddressOfThreadsAndProcessDetails[i].IsProcess)
                {
                    CheckCurrentProcessOrThread = FALSE;

                    if (g_ActiveProcessDebuggingState.IsActive &&
                        AddressOfThreadsAndProcessDetails[i].ProcessId == g_ActiveProcessDebuggingState.ProcessId)
                    {
                        CheckCurrentProcessOrThread = TRUE;
                    }

                    ShowMessages("%s%04x (process)\n",
                                 CheckCurrentProcessOrThread ? "*" : "",
                                 AddressOfThreadsAndProcessDetails[i].ProcessId);
                }
                else
                {
                    CheckCurrentProcessOrThread = FALSE;

                    if (g_ActiveProcessDebuggingState.IsActive &&
                        AddressOfThreadsAndProcessDetails[i].ThreadId == g_ActiveProcessDebuggingState.ThreadId)
                    {
                        CheckCurrentProcessOrThread = TRUE;
                    }
                    ShowMessages("\t%s %04x (thread)\n",
                                 CheckCurrentProcessOrThread ? "->" : "  ",
                                 AddressOfThreadsAndProcessDetails[i].ThreadId);
                }
            }
        }

        //
        // The operation of attaching was successful
        //
        return TRUE;
    }
    else
    {
        ShowErrorMessage(QueryCountOfActiveThreadsRequest.Result);
        return FALSE;
    }
}
