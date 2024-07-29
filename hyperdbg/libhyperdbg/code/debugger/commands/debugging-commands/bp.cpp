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
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

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
 * @brief request breakpoint
 *
 * @param Address Address
 * @param Pid Process Id
 * @param Tid Thread Id
 * @param CoreNumer Core Number
 *
 * @return VOID
 */
VOID
CommandBpRequest(UINT64 Address, UINT32 Pid, UINT32 Tid, UINT32 CoreNumer)
{
    DEBUGGEE_BP_PACKET BpPacket = {0};

    //
    // Set the details for the remote packet
    //
    BpPacket.Address = Address;
    BpPacket.Core    = CoreNumer;
    BpPacket.Pid     = Pid;
    BpPacket.Tid     = Tid;

    //
    // Send the bp packet
    //
    KdSendBpPacketToDebuggee(&BpPacket);
}

/**
 * @brief bp command handler
 *
 * @param CommandTokens
 *
 * @return VOID
 */
VOID
CommandBp(vector<CommandToken> CommandTokens)
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
        ShowMessages("incorrect use of the 'bp'\n\n");
        CommandBpHelp();
        return;
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
        ShowMessages(
            "please specify a correct hex value as the breakpoint address\n\n");
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

    if (!g_IsSerialConnectedToRemoteDebuggee)
    {
        ShowMessages("err, setting breakpoints is not possible when you're not "
                     "connected to a debuggee\n");
        return;
    }

    //
    // Request breakpoint the bp packet
    //
    CommandBpRequest(Address, Pid, Tid, CoreNumer);
}
