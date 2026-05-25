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
    ShowMessages("for using this command, you should configure it using the '!lbr' command\n");

    ShowMessages("syntax : \t!lbrdump [core CoreId (hex)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !lbrdump\n");
    ShowMessages("\t\te.g : !lbrdump core 1\n");
}

/**
 * @brief Get the branch type name based on the LBR branch type value (only applicable for architectural LBR)
 *
 * @details THIS FUNCTION IS ALSO IMPLEMENTED IN THE USER MODE
 *
 * @param BrType The raw branch type value from the LBR info MSR
 * @param BrTypeName A character buffer to receive the branch type name string
 *
 * @return VOID
 */
VOID
CommandLbrdumpGetArchBranchTypet(UINT32 BrType, CHAR * BrTypeName)
{
    if (BrType == LBR_BR_TYPE_COND)
    {
        strncpy(BrTypeName, "COND         ", LBR_BR_TYPE_NAME_MAX_LEN);
    }
    else if (BrType == LBR_BR_TYPE_JMP_INDIRECT)
    {
        strncpy(BrTypeName, "JMP Indirect ", LBR_BR_TYPE_NAME_MAX_LEN);
    }
    else if (BrType == LBR_BR_TYPE_JMP_DIRECT)
    {
        strncpy(BrTypeName, "JMP Direct   ", LBR_BR_TYPE_NAME_MAX_LEN);
    }
    else if (BrType == LBR_BR_TYPE_CALL_INDIRECT)
    {
        strncpy(BrTypeName, "CALL Indirect", LBR_BR_TYPE_NAME_MAX_LEN);
    }
    else if (BrType == LBR_BR_TYPE_CALL_DIRECT)
    {
        strncpy(BrTypeName, "CALL Direct  ", LBR_BR_TYPE_NAME_MAX_LEN);
    }
    else if (BrType == LBR_BR_TYPE_RET)
    {
        strncpy(BrTypeName, "RET          ", LBR_BR_TYPE_NAME_MAX_LEN);
    }
    else if (BrType >= LBR_BR_TYPE_RESERVED_MIN && BrType <= LBR_BR_TYPE_RESERVED_MAX)
    {
        strncpy(BrTypeName, "Reserved     ", LBR_BR_TYPE_NAME_MAX_LEN);
    }
    else if (BrType >= LBR_BR_TYPE_OTHER_MIN && BrType <= LBR_BR_TYPE_OTHER_MAX)
    {
        strncpy(BrTypeName, "Other Branch ", LBR_BR_TYPE_NAME_MAX_LEN);
    }
    else
    {
        strncpy(BrTypeName, "Unknown      ", LBR_BR_TYPE_NAME_MAX_LEN);
    }
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
    BOOLEAN IsCoreEmpty                          = TRUE;
    CHAR    BrTypeName[LBR_BR_TYPE_NAME_MAX_LEN] = {0};
    UINT32  BrType                               = 0;

    ShowMessages("LBR Chronological Trace on core: 0x%x\n\n", LbrdumpRequest->CoreId);

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

        if (LbrdumpRequest->ArchBasedLBR)
        {
            BrType = (UINT32)LbrdumpRequest->LbrStack.LastBranchInfo[CurrentIdx].BrType_OnlyArchLbr;

            //
            // Get the branch type name for better readability when printing
            //
            CommandLbrdumpGetArchBranchTypet(BrType, BrTypeName);

            //
            // Architectural LBR
            //
            ShowMessages("\t [%2u] Branch Mispredicted: %s, Branch type: %s, Cycle Count (Decimal): %04d (is valid? %s) - From: %016llx  To: %016llx\n",
                         CurrentIdx,
                         LbrdumpRequest->LbrStack.LastBranchInfo[CurrentIdx].Mispred ? "true " : "false",
                         BrTypeName,
                         LbrdumpRequest->LbrStack.LastBranchInfo[CurrentIdx].CycleCount,
                         LbrdumpRequest->LbrStack.LastBranchInfo[CurrentIdx].CycCntValid_OnlyArchLbr ? "true " : "false",
                         LbrdumpRequest->LbrStack.BranchEntry[CurrentIdx].From,
                         LbrdumpRequest->LbrStack.BranchEntry[CurrentIdx].To);
        }
        else
        {
            //
            // Legacy LBR
            //
            ShowMessages("\t [%2u] Branch Mispredicted: %s, Cycle Count (Decimal): %04d - From: %016llx  To: %016llx\n",
                         CurrentIdx,
                         LbrdumpRequest->LbrStack.LastBranchInfo[CurrentIdx].Mispred ? "true " : "false",
                         LbrdumpRequest->LbrStack.LastBranchInfo[CurrentIdx].CycleCount,
                         LbrdumpRequest->LbrStack.BranchEntry[CurrentIdx].From,
                         LbrdumpRequest->LbrStack.BranchEntry[CurrentIdx].To);
        }
    }

    if (IsCoreEmpty)
    {
        ShowMessages("\t no LBR entries found for this core\n"
                     "\t you can use the 'lbr_save();' function in the script engine\n"
                     "\t on the target core to save the LBR entries before dumping\n\n"
                     "\t ===========================================================\n\n");
    }
    else
    {
        ShowMessages("\n\t ===========================================================\n\n");
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
