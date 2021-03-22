/**
 * @file bp.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @brief help of bp command
 *
 * @return VOID
 */
VOID
CommandBpHelp()
{
    ShowMessages("bp : Puts a breakpoint (0xcc).\n");
    ShowMessages(
        "Note : 'bp' is not an event, if you want to use an event version "
        "of breakpoints use !epthook or !epthook2 instead. See "
        "documentation for more inforamtion.\n\n");
    ShowMessages("syntax : \tbp [address (hex value)] [pid | tid | core (hex "
                 "value - optional)]\n");
    ShowMessages("\t\te.g : bp fffff8077356f010\n");
    ShowMessages("\t\te.g : bp fffff8077356f010 pid 0x4\n");
    ShowMessages("\t\te.g : bp fffff8077356f010 tid 0x1000\n");
    ShowMessages("\t\te.g : bp fffff8077356f010 pid 0x4 core 2\n");
}

/**
 * @brief bp command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandBp(vector<string> SplittedCommand, string Command)
{
    BOOL IsNextCoreId = FALSE;
    BOOL IsNextPid    = FALSE;
    BOOL IsNextTid    = FALSE;

    BOOLEAN SetCoreId  = FALSE;
    BOOLEAN SetPid     = FALSE;
    BOOLEAN SetTid     = FALSE;
    BOOLEAN SetAddress = FALSE;

    UINT32 Tid       = DEBUGGEE_BP_APPLY_TO_ALL_THREADS;
    UINT32 Pid       = DEBUGGEE_BP_APPLY_TO_ALL_PROCESSES;
    UINT32 CoreNumer = DEBUGGEE_BP_APPLY_TO_ALL_CORES;
    UINT64 Address   = NULL;

    DEBUGGEE_BP_PACKET BpPacket = {0};

    if (SplittedCommand.size() >= 9)
    {
        ShowMessages("incorrect use of 'bp'\n\n");
        CommandBpHelp();
        return;
    }

    for (auto Section : SplittedCommand)
    {
        //
        // Ignore the first argument as it's the command string itself (bp)
        //
        if (!Section.compare(SplittedCommand.at(0)))
        {
            continue;
        }

        if (IsNextCoreId)
        {
            if (!ConvertStringToUInt32(Section, &CoreNumer))
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
            if (!ConvertStringToUInt32(Section, &Pid))
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
            if (!ConvertStringToUInt32(Section, &Tid))
            {
                ShowMessages("please specify a correct hex value for thread id\n\n");
                CommandBpHelp();
                return;
            }
            IsNextTid = FALSE;
            continue;
        }

        if (!Section.compare("pid"))
        {
            IsNextPid = TRUE;
            continue;
        }
        if (!Section.compare("tid"))
        {
            IsNextTid = TRUE;
            continue;
        }
        if (!Section.compare("core"))
        {
            IsNextCoreId = TRUE;
            continue;
        }

        if (!SetAddress)
        {
            if (!ConvertStringToUInt64(Section, &Address))
            {
                ShowMessages("please specify a correct hex value as address\n\n");
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
