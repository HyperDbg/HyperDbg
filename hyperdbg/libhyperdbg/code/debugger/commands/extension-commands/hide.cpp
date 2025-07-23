/**
 * @file hide.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !hide command
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern UINT64 g_CpuidAverage;
extern UINT64 g_CpuidStandardDeviation;
extern UINT64 g_CpuidMedian;

extern UINT64 g_RdtscAverage;
extern UINT64 g_RdtscStandardDeviation;
extern UINT64 g_RdtscMedian;

extern BOOLEAN                  g_TransparentResultsMeasured;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of the !hide command
 *
 * @return VOID
 */
VOID
CommandHideHelp()
{
    ShowMessages("!hide : tries to make HyperDbg transparent from anti-debugging "
                 "and anti-hypervisor methods.\n\n");

    ShowMessages("syntax : \t!hide\n");
    ShowMessages("syntax : \t!hide [pid ProcessId (hex)]\n");
    ShowMessages("syntax : \t!hide [name ProcessName (string)]\n");

    ShowMessages("note : \tprocess names are case sensitive and you can use "
                 "this command multiple times.\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !hide\n");
    ShowMessages("\t\te.g : !hide pid b60 \n");
    ShowMessages("\t\te.g : !hide name procexp.exe\n");
}

/**
 * @brief This function is called when the user wants to hide the fill system calls
 * in the transparent mode based on current system call numbers
 *
 * @param SyscallNumberDetails
 * @return BOOLEAN
 */
BOOLEAN
CommandHideFillSystemCalls(SYSTEM_CALL_NUMBERS_INFORMATION * SyscallNumberDetails)
{
    BOOLEAN Result = TRUE;

    //
    // Get the syscall number of NtQuerySystemInformation
    //
    SyscallNumberDetails->SysNtQuerySystemInformation = PeGetSyscallNumber("NtQuerySystemInformation");

    if (SyscallNumberDetails->SysNtQuerySystemInformation == 0)
    {
        ShowMessages("warning, failed to get NtQuerySystemInformation syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtQuerySystemInformationEx
    //
    SyscallNumberDetails->SysNtQuerySystemInformationEx = PeGetSyscallNumber("NtQuerySystemInformationEx");

    if (SyscallNumberDetails->SysNtQuerySystemInformationEx == 0)
    {
        ShowMessages("warning, failed to get NtQuerySystemInformationEx syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtSystemDebugControl
    //
    SyscallNumberDetails->SysNtSystemDebugControl = PeGetSyscallNumber("NtSystemDebugControl");

    if (SyscallNumberDetails->SysNtSystemDebugControl == 0)
    {
        ShowMessages("warning, failed to get NtSystemDebugControl syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtQueryAttributesFile
    //
    SyscallNumberDetails->SysNtQueryAttributesFile = PeGetSyscallNumber("NtQueryAttributesFile");

    if (SyscallNumberDetails->SysNtQueryAttributesFile == 0)
    {
        ShowMessages("warning, failed to get NtQueryAttributesFile syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtOpenDirectoryObject
    //
    SyscallNumberDetails->SysNtOpenDirectoryObject = PeGetSyscallNumber("NtOpenDirectoryObject");

    if (SyscallNumberDetails->SysNtOpenDirectoryObject == 0)
    {
        ShowMessages("warning, failed to get NtOpenDirectoryObject syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtQueryDirectoryObject
    //
    SyscallNumberDetails->SysNtQueryDirectoryObject = PeGetSyscallNumber("NtQueryDirectoryObject");

    if (SyscallNumberDetails->SysNtQueryDirectoryObject == 0)
    {
        ShowMessages("warning, failed to get NtQueryDirectoryObject syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtQueryInformationProcess
    //
    SyscallNumberDetails->SysNtQueryInformationProcess = PeGetSyscallNumber("NtQueryInformationProcess");

    if (SyscallNumberDetails->SysNtQueryInformationProcess == 0)
    {
        ShowMessages("warning, failed to get NtQueryInformationProcess syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtSetInformationProcess
    //
    SyscallNumberDetails->SysNtSetInformationProcess = PeGetSyscallNumber("NtSetInformationProcess");

    if (SyscallNumberDetails->SysNtSetInformationProcess == 0)
    {
        ShowMessages("warning, failed to get NtSetInformationProcess syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtQueryInformationThread
    //
    SyscallNumberDetails->SysNtQueryInformationThread = PeGetSyscallNumber("NtQueryInformationThread");

    if (SyscallNumberDetails->SysNtQueryInformationThread == 0)
    {
        ShowMessages("warning, failed to get NtQueryInformationThread syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtSetInformationThread
    //
    SyscallNumberDetails->SysNtSetInformationThread = PeGetSyscallNumber("NtSetInformationThread");

    if (SyscallNumberDetails->SysNtSetInformationThread == 0)
    {
        ShowMessages("warning, failed to get NtSetInformationThread syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtOpenFile
    //
    SyscallNumberDetails->SysNtOpenFile = PeGetSyscallNumber("NtOpenFile");

    if (SyscallNumberDetails->SysNtOpenFile == 0)
    {
        ShowMessages("warning, failed to get NtOpenFile syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtOpenKey
    //
    SyscallNumberDetails->SysNtOpenKey = PeGetSyscallNumber("NtOpenKey");

    if (SyscallNumberDetails->SysNtOpenKey == 0)
    {
        ShowMessages("warning, failed to get NtOpenKey syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtOpenKeyEx
    //
    SyscallNumberDetails->SysNtOpenKeyEx = PeGetSyscallNumber("NtOpenKeyEx");

    if (SyscallNumberDetails->SysNtOpenKeyEx == 0)
    {
        ShowMessages("warning, failed to get NtOpenKeyEx syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtQueryValueKey
    //
    SyscallNumberDetails->SysNtQueryValueKey = PeGetSyscallNumber("NtQueryValueKey");

    if (SyscallNumberDetails->SysNtQueryValueKey == 0)
    {
        ShowMessages("warning, failed to get NtQueryValueKey syscall number for transparent-mode\n");
        Result = FALSE;
    }

    //
    // Get the syscall number of NtEnumerateKey
    //
    SyscallNumberDetails->SysNtEnumerateKey = PeGetSyscallNumber("NtEnumerateKey");

    if (SyscallNumberDetails->SysNtEnumerateKey == 0)
    {
        ShowMessages("warning, failed to get NtEnumerateKey syscall number for transparent-mode\n");
        Result = FALSE;
    }

    return TRUE;
}

/**
 * @brief Enable transparent mode
 * @param ProcessId
 * @param ProcessName
 * @param IsProcessId
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgEnableTransparentMode(UINT32 ProcessId, CHAR * ProcessName, BOOLEAN IsProcessId)
{
    BOOLEAN                                      Status;
    ULONG                                        ReturnedLength;
    DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE  HideRequest        = {0};
    PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE FinalRequestBuffer = 0;
    size_t                                       RequestBufferSize  = 0;

    //
    // Check if debugger is loaded or not
    //
    // AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // We wanna hide the debugger and make transparent vm-exits
    //
    HideRequest.IsHide = TRUE;

    HideRequest.TrueIfProcessIdAndFalseIfProcessName = IsProcessId;

    if (IsProcessId)
    {
        //
        // It's a process id
        //
        HideRequest.ProcId = (UINT32)ProcessId;

        RequestBufferSize = sizeof(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE);
    }
    else
    {
        //
        // It's a process name
        //
        HideRequest.LengthOfProcessName = (UINT32)strlen(ProcessName) + 1;
        RequestBufferSize               = sizeof(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE) + HideRequest.LengthOfProcessName;
    }

    //
    // Allocate the requested buffer
    //
    FinalRequestBuffer = (PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE)malloc(RequestBufferSize);

    if (FinalRequestBuffer == NULL)
    {
        ShowMessages("insufficient space\n");
        return FALSE;
    }

    //
    // Zero the memory
    //
    RtlZeroMemory(FinalRequestBuffer, RequestBufferSize);

    //
    // Copy the buffer on the top of the final buffer
    // to send the kernel
    //
    memcpy(FinalRequestBuffer, &HideRequest, sizeof(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE));

    //
    // If it's a name then we should add it to the end of the buffer
    //
    if (!IsProcessId)
    {
        CHAR * ProcName = ProcessName;

        memcpy(((UINT64 *)((UINT64)FinalRequestBuffer + sizeof(DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE))),
               ProcName,
               HideRequest.LengthOfProcessName);
    }

    //
    // Send the request to the kernel
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                                             // Handle to device
        IOCTL_DEBUGGER_HIDE_AND_UNHIDE_TO_TRANSPARENT_THE_DEBUGGER, // IO Control
        // code
        FinalRequestBuffer,                                 // Input Buffer to driver.
        (DWORD)RequestBufferSize,                           // Input buffer length
        FinalRequestBuffer,                                 // Output Buffer from driver.
        SIZEOF_DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE, // Length of output
        // buffer in bytes.
        &ReturnedLength, // Bytes placed in buffer.
        NULL             // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        free(FinalRequestBuffer);
        return FALSE;
    }

    if (FinalRequestBuffer->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        ShowMessages("transparent debugging successfully enabled :)\n");
    }
    else if (FinalRequestBuffer->KernelStatus == DEBUGGER_ERROR_UNABLE_TO_HIDE_OR_UNHIDE_DEBUGGER)
    {
        ShowMessages("unable to hide the debugger (transparent-debugging) :(\n");
        free(FinalRequestBuffer);
        return FALSE;
    }
    else
    {
        ShowMessages("unknown error occurred :(\n");
        free(FinalRequestBuffer);
        return FALSE;
    }

    //
    // free the buffer
    //
    free(FinalRequestBuffer);

    //
    // It means the transparent mode enabled successfully
    //
    return TRUE;
}

/**
 * @brief !hide command handler
 *
 * @param CommandTokens
 * @param Command
 * @return VOID
 */
VOID
CommandHide(vector<CommandToken> CommandTokens, string Command)
{
    UINT32  TargetPid;
    BOOLEAN TrueIfProcessIdAndFalseIfProcessName;

#if ActivateHyperEvadeProject == TRUE

    ShowMessages("warning, the !hide command (hyperevade project) is in the Beta phase and is not yet well-tested, "
                 "so it is disabled in this version. If you want to test, you can enable it "
                 "from the configuration file (set ActivateHyperEvadeProject to TRUE) and recompile HyperDbg\n\n");
    return;

#endif

    if (CommandTokens.size() <= 2 && CommandTokens.size() != 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandHideHelp();
        return;
    }

    //
    // Find out whether the user enters pid or name
    //
    if (CommandTokens.size() == 1)
    {
        if (g_ActiveProcessDebuggingState.IsActive)
        {
            TrueIfProcessIdAndFalseIfProcessName = TRUE;
            TargetPid                            = g_ActiveProcessDebuggingState.ProcessId;
        }
        else
        {
            //
            // There is no user-debugging process
            //
            ShowMessages("you're not attached to any user-mode process, "
                         "please explicitly specify the process id or process name\n\n");
            CommandHideHelp();
            return;
        }
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "pid"))
    {
        TrueIfProcessIdAndFalseIfProcessName = TRUE;

        //
        // Check for the user to not add extra arguments
        //
        if (CommandTokens.size() != 3)
        {
            ShowMessages("incorrect use of the '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
            CommandHideHelp();
            return;
        }

        //
        // It's just a pid for the process
        //
        if (!ConvertTokenToUInt32(CommandTokens.at(2), &TargetPid))
        {
            ShowMessages("incorrect process id\n\n");
            return;
        }
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "name"))
    {
        TrueIfProcessIdAndFalseIfProcessName = FALSE;
    }
    else
    {
        //
        // Invalid argument for the second parameter to the command
        //
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandHideHelp();
        return;
    }

    //
    // Enable the transparent mode
    //
    if (TrueIfProcessIdAndFalseIfProcessName)
    {
        HyperDbgEnableTransparentMode(TargetPid,
                                      NULL,
                                      TRUE);
    }
    else
    {
        HyperDbgEnableTransparentMode(NULL,
                                      (CHAR *)GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)).c_str(),
                                      FALSE);
    }
}
