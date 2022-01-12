/**
 * @file ud.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief control the user-mode debugging affairs
 * @details
 * @version 0.1
 * @date 2021-12-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

//
// Global Variables
//
extern ACTIVE_DEBUGGING_THREAD g_ActiveThreadDebuggingState;

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
UdSetActiveDebuggingThread(UINT64  DebuggingId,
                           UINT32  ProcessId,
                           UINT32  ThreadId,
                           BOOLEAN Is32Bit)
{
    g_ActiveThreadDebuggingState.ProcessId         = ProcessId;
    g_ActiveThreadDebuggingState.ThreadId          = ThreadId;
    g_ActiveThreadDebuggingState.Is32Bit           = Is32Bit;
    g_ActiveThreadDebuggingState.UniqueDebuggingId = DebuggingId;

    //
    // Activate the debugging
    //
    g_ActiveThreadDebuggingState.IsActive = TRUE;
}

/**
 * @brief Remove the current active debugging process (thread)
 * @param ProcessId
 * 
 * @return VOID
 */
VOID
UdRemoveActiveDebuggingThread(UINT32 ProcessId)
{
    //
    // Do sth
    //

    //
    // Activate the debugging
    //
    g_ActiveThreadDebuggingState.IsActive = FALSE;
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
 * @details this function will not check whether the process id and
 * thread id is valid or not
 *
 * @param TargetPid
 * @param TargetTid
 * @param TargetFileAddress
 * @param CommandLine
 * @return BOOLEAN
 */
BOOLEAN
UdAttachToProcess(UINT32        TargetPid,
                  UINT32        TargetTid,
                  const WCHAR * TargetFileAddress,
                  WCHAR *       CommandLine)
{
    BOOLEAN                                  Status;
    ULONG                                    ReturnedLength;
    DEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachRequest = {0};
    PROCESS_INFORMATION                      ProcInfo      = {0};

    //
    // Check if debugger is loaded or not
    //
    if (!g_DeviceHandle)
    {
        ShowMessages("handle of the driver not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return FALSE;
    }

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
        // Set the process id and thread id
        //
        AttachRequest.ProcessId = TargetPid;
        AttachRequest.ThreadId  = TargetTid;
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
        //
        // Set the current active debugging process (thread)
        //
        UdSetActiveDebuggingThread(AttachRequest.Token,
                                   AttachRequest.ProcessId,
                                   AttachRequest.ThreadId,
                                   AttachRequest.Is32Bit);

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
    if (!g_DeviceHandle)
    {
        ShowMessages("handle of the driver not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return FALSE;
    }

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
        UdRemoveActiveDebuggingThread(TargetPid);

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

    return FALSE;
}

/**
 * @brief Handle pause packets from user debugger
 *
 * @param PausePacket
 * 
 * @return VOID
 */
VOID
UdHandleUserDebuggerPausing(PDEBUGGEE_UD_PAUSED_PACKET PausePacket)
{
    //
    // Perform extra tasks for pausing reasons
    //
    switch (PausePacket->PausingReason)
    {
    case DEBUGGEE_PAUSING_REASON_DEBUGGEE_ENTRY_POINT_REACHED:

        ShowMessages("reached to the entrypoint of the main module\n");

        break;

    default:
        break;
    }

    //
    // Check if the instruction is received completely or not
    //
    if (PausePacket->ReadInstructionLen != MAXIMUM_INSTR_SIZE)
    {
        //
        // We check if the disassembled buffer has greater size
        // than what is retrieved
        //
        if (HyperDbgLengthDisassemblerEngine(PausePacket->InstructionBytesOnRip,
                                             MAXIMUM_INSTR_SIZE,
                                             PausePacket->Is32BitAddress ? FALSE : TRUE) > PausePacket->ReadInstructionLen)
        {
            ShowMessages("oOh, no! there might be a misinterpretation in disassembling the current instruction\n");
        }
    }

    if (!PausePacket->Is32BitAddress)
    {
        //
        // Show diassembles
        //
        HyperDbgDisassembler64(PausePacket->InstructionBytesOnRip,
                               PausePacket->Rip,
                               MAXIMUM_INSTR_SIZE,
                               1,
                               TRUE,
                               &PausePacket->Rflags);
    }
    else
    {
        //
        // Show diassembles
        //
        HyperDbgDisassembler32(PausePacket->InstructionBytesOnRip,
                               PausePacket->Rip,
                               MAXIMUM_INSTR_SIZE,
                               1,
                               TRUE,
                               &PausePacket->Rflags);
    }
}

/**
 * @brief Send the command to the user debugger
 * @param ThreadDetailToken
 * @param ActionType
 * @param OptionalParam1
 * @param OptionalParam2
 * @param OptionalParam3
 * @param OptionalParam4
 * 
 * @return VOID
 */
VOID
UdSendCommand(UINT64                          ThreadDetailToken,
              DEBUGGER_UD_COMMAND_ACTION_TYPE ActionType,
              UINT64                          OptionalParam1,
              UINT64                          OptionalParam2,
              UINT64                          OptionalParam3,
              UINT64                          OptionalParam4)
{
    BOOL                       Status;
    ULONG                      ReturnedLength;
    DEBUGGER_UD_COMMAND_PACKET CommandPacket;

    if (!g_DeviceHandle)
    {
        ShowMessages("handle of the driver not found, probably the driver is not loaded. Did you "
                     "use 'load' command?\n");
        return;
    }

    //
    // Zero the packet
    //
    RtlZeroMemory(&CommandPacket, sizeof(DEBUGGER_UD_COMMAND_PACKET));

    //
    // Set to the details
    //
    CommandPacket.ThreadDebuggingDetailToken = ThreadDetailToken;
    CommandPacket.UdAction.ActionType        = ActionType;
    CommandPacket.UdAction.OptionalParam1    = OptionalParam1;
    CommandPacket.UdAction.OptionalParam2    = OptionalParam2;
    CommandPacket.UdAction.OptionalParam3    = OptionalParam3;
    CommandPacket.UdAction.OptionalParam4    = OptionalParam4;

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
 * @param ThreadDetailToken
 * 
 * @return VOID
 */
VOID
UdContinueDebuggee(UINT64 ThreadDetailToken)
{
    //
    // Send the 'continue' command
    //
    UdSendCommand(ThreadDetailToken, DEBUGGER_UD_COMMAND_ACTION_TYPE_CONTINUE, NULL, NULL, NULL, NULL);
}
