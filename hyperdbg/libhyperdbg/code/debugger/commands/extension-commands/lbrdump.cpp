/**
 * @file lbrdump.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !lbrdump command
 * @details
 * @version 0.19
 * @date 2026-05-03
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
 * @brief Send LBR dump requests
 *
 * @param LbrdumpRequest
 *
 * @return VOID
 */
BOOLEAN
HyperDbgLbrdumpSendRequest(HYPERTRACE_LBR_DUMP_PACKETS * LbrdumpRequest)
{
    BOOL  Status;
    ULONG ReturnedLength;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send the request over serial kernel debugger
        //
        if (!KdSendHyperTraceLbrdumpPacketsToDebuggee(LbrdumpRequest, SIZEOF_HYPERTRACE_LBR_DUMP_PACKETS))
        {
            return FALSE;
        }
    }
    else
    {
        AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

        //
        // Send IOCTL
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                     // Handle to device
            IOCTL_PERFORM_HYPERTRACE_LBR_DUMP,  // IO Control Code (IOCTL)
            LbrdumpRequest,                     // Input Buffer to driver.
            SIZEOF_HYPERTRACE_LBR_DUMP_PACKETS, // Input buffer length
            LbrdumpRequest,                     // Output Buffer from driver.
            SIZEOF_HYPERTRACE_LBR_DUMP_PACKETS, // Length of output buffer in bytes.
            &ReturnedLength,                    // Bytes placed in buffer.
            NULL                                // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());

            return FALSE;
        }
    }

    //
    // Sending the request was successful
    //
    return TRUE;
}

/**
 * @brief help of the !lbrdump command
 *
 * @return VOID
 */
VOID
CommandLbrdumpHelp()
{
    ShowMessages("!lbrdump : dumps Last Branch Record (LBR).\n");

    ShowMessages("syntax : \t!lbrdump [core CoreId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !lbrdump\n");
    ShowMessages("\t\te.g : !lbrdump core 1\n");
}

/**
 * @brief Print collected LBR branches
 *
 * @param LbrdumpRequest
 *
 * @return VOID
 */
VOID
CommandLbrdumpPrint(HYPERTRACE_LBR_DUMP_PACKETS * LbrdumpRequest)
{
    ULONG   CurrentIdx;
    BOOLEAN IsCoreEmpty = TRUE;

    ShowMessages("LBR Chronological Trace on core (decimal): %d\n\n", LbrdumpRequest->CoreId);

    for (ULONG i = 1; i <= LbrdumpRequest->CurrentLbrCapacity; i++)
    {
        if (LbrdumpRequest->ArchBasedLBR)
        {
            //
            // In ARCH LBR, there is not TOS index and everything is in order
            //
            CurrentIdx = i - 1;
        }
        else
        {
            CurrentIdx = (ULONG)(LbrdumpRequest->LbrStack.Tos + i) % (ULONG)LbrdumpRequest->CurrentLbrCapacity;
        }

        if (LbrdumpRequest->LbrStack.BranchEntry[CurrentIdx].From == 0)
        {
            continue;
        }

        IsCoreEmpty = FALSE;

        ShowMessages("\t [%2u] Branch Mispredicted: %s, Cycle Count (Decimal): %03d - From: %016llx  To: %016llx\n",
                     CurrentIdx,
                     LbrdumpRequest->LbrStack.LastBranchInfo[CurrentIdx].Mispred ? "true " : "false",
                     LbrdumpRequest->LbrStack.LastBranchInfo[CurrentIdx].CycleCount,
                     LbrdumpRequest->LbrStack.BranchEntry[CurrentIdx].From,
                     LbrdumpRequest->LbrStack.BranchEntry[CurrentIdx].To);
    }

    if (IsCoreEmpty)
    {
        ShowMessages("\t no LBR entries found for this core\n"
                     "\t you can use the 'lbr_save();' function in the script engine\n"
                     "\t on the target core to save the LBR entries before dumping\n\n"
                     "\t ===========================================================\n\n");
    }
}

/**
 * @brief !lbrdump command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandLbrdump(vector<CommandToken> CommandTokens, string Command)
{
    HYPERTRACE_LBR_DUMP_PACKETS LbrdumpRequest          = {0};
    UINT32                      CoreId                  = 0;
    BOOLEAN                     ContinueDumpingAllCores = TRUE;

    if (CommandTokens.size() != 1 && CommandTokens.size() != 3)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandLbrdumpHelp();
        return;
    }

    if (CommandTokens.size() == 3)
    {
        if (CompareLowerCaseStrings(CommandTokens.at(1), "core"))
        {
            if (!ConvertTokenToUInt32(CommandTokens.at(2), &CoreId))
            {
                //
                // Unknown parameter
                //
                ShowMessages("unknown parameter '%s'\n\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
                CommandLbrdumpHelp();

                return;
            }
        }
        else
        {
            ShowMessages("incorrect use of the '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
            CommandLbrdumpHelp();
            return;
        }
    }
    else
    {
        //
        // If no core is specified, we can set the CoreId to a special value (e.g., 0xFFFFFFFF) to indicate that the dump should be performed for all cores
        //
        CoreId = HYPERTRACE_LBR_DUMP_ALL_CORES;
    }

    //
    // If the CoreId is set to the special value for dumping all cores,
    // we can set it to 0 to start dumping from the first core and rely on the NextCoreIsValid flag
    // in the dump request structure to indicate whether there are more cores to be dumped or not in
    // the driver response
    //
    if (CoreId == HYPERTRACE_LBR_DUMP_ALL_CORES)
    {
        LbrdumpRequest.CoreId = 0;
    }
    else
    {
        LbrdumpRequest.CoreId = CoreId;
    }

    while (ContinueDumpingAllCores)
    {
        //
        // Send the LBR dump request
        //
        if (HyperDbgLbrdumpSendRequest(&LbrdumpRequest))
        {
            if (LbrdumpRequest.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
            {
                //
                // Show entries of the LBR stack in the request structure which are filled by the driver in response to the dump request
                //
                CommandLbrdumpPrint(&LbrdumpRequest);

                if (CoreId == HYPERTRACE_LBR_DUMP_ALL_CORES && LbrdumpRequest.NextCoreIsValid)
                {
                    //
                    // If the NextCoreIsValid flag is set, it means there are more cores to be
                    // dumped in the case of dumping all cores, so we can update the CoreId
                    // in the dump request structure to the next core number for the next
                    // iteration of dumping
                    //
                    LbrdumpRequest.CoreId++;
                }
                else
                {
                    //
                    // If the NextCoreIsValid flag is not set, it means there are no more
                    // cores to be dumped in the case of dumping all cores, so we can break the loop
                    //
                    ContinueDumpingAllCores = FALSE;
                }
            }
            else
            {
                ShowErrorMessage(LbrdumpRequest.KernelStatus);
                break;
            }
        }
        else
        {
            ShowMessages("failed to send LBR dump request\n");
            break;
        }
    }
}
