/**
 * @file bp.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief bp command
 * @details
 * @version 0.1
 * @date 2021-10-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of the bp command
 *
 * @return VOID
 */
VOID
CommandBpHelp()
{
    ShowMessages("bp : puts a breakpoint (0xcc).\n");

    ShowMessages(
        "Note : 'bp' is not an event, if you want to use an event version "
        "of breakpoints use !epthook or !epthook2 instead. See "
        "documentation for more information.\n\n");

    ShowMessages("syntax : \tbp [Address (hex)] [pid ProcessId (hex)] [tid ThreadId (hex)] [core CoreId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : bp nt!ExAllocatePoolWithTag\n");
    ShowMessages("\t\te.g : bp nt!ExAllocatePoolWithTag+5\n");
    ShowMessages("\t\te.g : bp nt!ExAllocatePoolWithTag+@rcx+rbx\n");
    ShowMessages("\t\te.g : bp fffff8077356f010\n");
    ShowMessages("\t\te.g : bp fffff8077356f010 pid 0x4\n");
    ShowMessages("\t\te.g : bp fffff8077356f010 tid 0x1000\n");
    ShowMessages("\t\te.g : bp fffff8077356f010 pid 0x4 core 2\n");
}

/**
 * @brief Apply breakpoint on the user debugger
 * @param BpPacket
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandBpPerformApplyingBreakpointOnUserDebugger(DEBUGGEE_BP_PACKET * BpPacket)
{
    BOOL  Status;
    ULONG ReturnedLength;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Check if the debugger is connected to a remote debuggee, this request
        // is not for the kernel debugger
        //
        return FALSE;
    }
    else
    {
        AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

        //
        // Send IOCTL
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                     // Handle to device
            IOCTL_SET_BREAKPOINT_USER_DEBUGGER, // IO Control Code (IOCTL)
            BpPacket,                           // Input Buffer to driver.
            SIZEOF_DEBUGGEE_BP_PACKET,          // Input buffer length (not used in this case)
            BpPacket,                           // Output Buffer from driver.
            SIZEOF_DEBUGGEE_BP_PACKET,          // Length of output buffer in bytes.
            &ReturnedLength,                    // Bytes placed in buffer.
            NULL                                // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());

            return FALSE;
        }

        if (BpPacket->Result == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            return TRUE;
        }
        else
        {
            //
            // An err occurred, no results
            //
            ShowErrorMessage(BpPacket->Result);
            return FALSE;
        }
    }
}

/**
 * @brief request breakpoint
 *
 * @param Address Address
 * @param Pid Process Id
 * @param Tid Thread Id
 * @param CoreNumer Core Number
 *
 * @return BOOLEAN
 */
BOOLEAN
CommandBpRequest(UINT64 Address, UINT32 Pid, UINT32 Tid, UINT32 CoreNumer)
{
    DEBUGGEE_BP_PACKET BpPacket = {0};

    //
    // Check if the debugger is connected to a remote debuggee or a user-debugger is active
    //
    if (!g_IsSerialConnectedToRemoteDebuggee && !g_ActiveProcessDebuggingState.IsActive)
    {
        return FALSE;
    }

    //
    // Set the details for the remote packet
    //
    BpPacket.Address = Address;
    BpPacket.Core    = CoreNumer;
    BpPacket.Pid     = Pid;
    BpPacket.Tid     = Tid;

    //
    // Send the bp packet either to the user debugger or the kernel debugger
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        return CommandBpPerformApplyingBreakpointOnUserDebugger(&BpPacket);
    }
    else if (g_IsSerialConnectedToRemoteDebuggee)
    {
        return KdSendBpPacketToDebuggee(&BpPacket);
    }
    else
    {
        //
        // couldn't set breakpoint, no active process or remote debuggee
        //
        return FALSE;
    }
}

/**
 * @brief bp command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandBp(vector<CommandToken> CommandTokens, string Command)
{
    BOOL IsNextCoreId = FALSE;
    BOOL IsNextPid    = FALSE;
    BOOL IsNextTid    = FALSE;

    BOOLEAN SetCoreId  = FALSE;
    BOOLEAN SetPid     = FALSE;
    BOOLEAN SetTid     = FALSE;
    BOOLEAN SetAddress = FALSE;

    UINT32  Tid            = DEBUGGEE_BP_APPLY_TO_ALL_THREADS;
    UINT32  Pid            = DEBUGGEE_BP_APPLY_TO_ALL_PROCESSES;
    UINT32  CoreNumer      = DEBUGGEE_BP_APPLY_TO_ALL_CORES;
    UINT64  Address        = NULL;
    BOOLEAN IsFirstCommand = TRUE;

    if (CommandTokens.size() >= 9)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandBpHelp();
        return;
    }

    //
    // If the user is debugging a process, use its pid
    //
    if (g_ActiveProcessDebuggingState.IsActive)
    {
        Pid = g_ActiveProcessDebuggingState.ProcessId;
    }

    for (auto Section : CommandTokens)
    {
        //
        // Ignore the first argument as it's the command string itself (bp)
        //
        if (IsFirstCommand == TRUE)
        {
            IsFirstCommand = FALSE;
            continue;
        }

        if (IsNextCoreId)
        {
            if (!ConvertTokenToUInt32(Section, &CoreNumer))
            {
                ShowMessages("please specify a correct hex value for core id\n\n");
                CommandBpHelp();
                return;
            }
            IsNextCoreId = FALSE;
            continue;
        }
        if (IsNextPid)
        {
            if (!ConvertTokenToUInt32(Section, &Pid))
            {
                ShowMessages("please specify a correct hex value for process id\n\n");
                CommandBpHelp();
                return;
            }
            IsNextPid = FALSE;
            continue;
        }

        if (IsNextTid)
        {
            if (!ConvertTokenToUInt32(Section, &Tid))
            {
                ShowMessages("please specify a correct hex value for thread id\n\n");
                CommandBpHelp();
                return;
            }
            IsNextTid = FALSE;
            continue;
        }

        if (CompareLowerCaseStrings(Section, "pid"))
        {
            IsNextPid = TRUE;
            continue;
        }
        if (CompareLowerCaseStrings(Section, "tid"))
        {
            IsNextTid = TRUE;
            continue;
        }
        if (CompareLowerCaseStrings(Section, "core"))
        {
            IsNextCoreId = TRUE;
            continue;
        }

        if (!SetAddress)
        {
            if (!SymbolConvertNameOrExprToAddress(GetCaseSensitiveStringFromCommandToken(Section), &Address))
            {
                //
                // Couldn't resolve or unknown parameter
                //
                ShowMessages("err, couldn't resolve error at '%s'\n\n",
                             GetCaseSensitiveStringFromCommandToken(Section).c_str());
                CommandBpHelp();
                return;
            }
            else
            {
                //
                // Means that address is received
                //
                SetAddress = TRUE;
                continue;
            }
        }
    }

    //
    // Check if address is set or not
    //
    if (!SetAddress)
    {
        ShowMessages("please specify a correct hex value as the breakpoint address\n\n");
        CommandBpHelp();
        return;
    }
    if (IsNextPid)
    {
        ShowMessages("please specify a correct hex value for process id\n\n");
        CommandBpHelp();
        return;
    }
    if (IsNextCoreId)
    {
        ShowMessages("please specify a correct hex value for core\n\n");
        CommandBpHelp();
        return;
    }
    if (IsNextTid)
    {
        ShowMessages("please specify a correct hex value for thread id\n\n");
        CommandBpHelp();
        return;
    }

    if (!g_IsSerialConnectedToRemoteDebuggee && !g_ActiveProcessDebuggingState.IsActive)
    {
        ShowMessages("setting breakpoints is not possible when you're not connected "
                     "to a target debuggee (kernel debugger or user debugger)\n");
        return;
    }

    //
    // Request breakpoint the bp packet
    //
    if (!CommandBpRequest(Address, Pid, Tid, CoreNumer))
    {
        ShowMessages("err, couldn't set breakpoint\n");
    }
}
