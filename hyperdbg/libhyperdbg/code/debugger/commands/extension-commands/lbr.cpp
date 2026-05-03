/**
 * @file lbr.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !lbr command
 * @details
 * @version 0.19
 * @date 2026-04-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the !lbr command
 *
 * @return VOID
 */
VOID
CommandLbrHelp()
{
    ShowMessages("!lbr : performs operation for Last Branch Record (LBR).\n");
    ShowMessages("for dumping LBR entries, you can use the '!lbrdump' command\n");

    ShowMessages("syntax : \t!lbr [Function (string)]\n");
    ShowMessages("syntax : \t!lbr [filter FilterOptions (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !lbr enable\n");
    ShowMessages("\t\te.g : !lbr disable\n");
    ShowMessages("\t\te.g : !lbr flush\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !lbr filter\n");
    ShowMessages("\t\te.g : !lbr filter kernel jcc ind_jmp rel_jmp far\n");
    ShowMessages("\t\te.g : !lbr filter kernel jcc return ind_jmp rel_jmp far\n");
    ShowMessages("\t\te.g : !lbr filter kernel jcc ind_jmp rel_jmp\n");
    ShowMessages("\t\te.g : !lbr filter user rel_call ind_call return far\n");

    ShowMessages("\nlist of filter options: \n");
    ShowMessages("\t kernel:      do not capture at ring0\n");
    ShowMessages("\t user:        do not capture at ring > 0\n");
    ShowMessages("\t jcc:         do not capture conditional branches\n");
    ShowMessages("\t rel_call:    do not capture relative calls\n");
    ShowMessages("\t ind_call:    do not capture indirect calls\n");
    ShowMessages("\t return:      do not capture near returns\n");
    ShowMessages("\t ind_jmp:     do not capture indirect jumps\n");
    ShowMessages("\t rel_jmp:     do not capture relative jumps\n");
    ShowMessages("\t far:         do not capture far branches\n");
    ShowMessages("\t (no option): capture everything (default option)\n");
}

/**
 * @brief Send LBR requests
 *
 * @param LbrRequest
 *
 * @return VOID
 */
BOOLEAN
CommandLbrSendRequest(HYPERTRACE_LBR_OPERATION_PACKETS * LbrRequest)
{
    BOOL  Status;
    ULONG ReturnedLength;

    AssertShowMessageReturnStmt(g_DeviceHandle, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    //
    // Send IOCTL
    //
    Status = DeviceIoControl(
        g_DeviceHandle,                          // Handle to device
        IOCTL_PERFORM_HYPERTRACE_LBR_OPERATION,  // IO Control Code (IOCTL)
        LbrRequest,                              // Input Buffer to driver.
        SIZEOF_HYPERTRACE_LBR_OPERATION_PACKETS, // Input buffer length
        LbrRequest,                              // Output Buffer from driver.
        SIZEOF_HYPERTRACE_LBR_OPERATION_PACKETS, // Length of output buffer in bytes.
        &ReturnedLength,                         // Bytes placed in buffer.
        NULL                                     // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());

        return FALSE;
    }

    if (LbrRequest->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Request to perform an LBR operation
 *
 * @param LbrRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgPerformLbrOperation(HYPERTRACE_LBR_OPERATION_PACKETS * LbrRequest)
{
    return CommandLbrSendRequest(LbrRequest);
}
/**
 * @brief Parses simple (no-argument) LBR operations: enable, disable, flush
 *
 * @param CommandTokens
 * @param LbrRequest
 *
 * @return BOOLEAN TRUE if parsed successfully
 */
BOOLEAN
CommandLbrParseSimpleOperation(vector<CommandToken> CommandTokens, HYPERTRACE_LBR_OPERATION_PACKETS * LbrRequest)
{
    if (CommandTokens.size() != 2)
    {
        return FALSE;
    }

    if (CompareLowerCaseStrings(CommandTokens.at(1), "enable"))
    {
        LbrRequest->LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "disable"))
    {
        LbrRequest->LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "flush"))
    {
        LbrRequest->LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FLUSH;
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Parses the LBR filter operation and accumulates filter option flags
 *
 * @param CommandTokens
 * @param LbrRequest
 *
 * @return BOOLEAN TRUE if parsed successfully
 */
BOOLEAN
CommandLbrParseFilterOperation(vector<CommandToken> CommandTokens, HYPERTRACE_LBR_OPERATION_PACKETS * LbrRequest)
{
    LbrRequest->LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FILTER;
    LbrRequest->LbrFilterOptions = 0; // no options = capture everything

    for (size_t i = 2; i < CommandTokens.size(); i++)
    {
        if (CompareLowerCaseStrings(CommandTokens.at(i), "kernel"))
        {
            LbrRequest->LbrFilterOptions |= LBR_KERNEL;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "user"))
        {
            LbrRequest->LbrFilterOptions |= LBR_USER;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "jcc"))
        {
            LbrRequest->LbrFilterOptions |= LBR_JCC;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "rel_call"))
        {
            LbrRequest->LbrFilterOptions |= LBR_REL_CALL;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "ind_call"))
        {
            LbrRequest->LbrFilterOptions |= LBR_IND_CALL;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "return"))
        {
            LbrRequest->LbrFilterOptions |= LBR_RETURN;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "ind_jmp"))
        {
            LbrRequest->LbrFilterOptions |= LBR_IND_JMP;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "rel_jmp"))
        {
            LbrRequest->LbrFilterOptions |= LBR_REL_JMP;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "far"))
        {
            LbrRequest->LbrFilterOptions |= LBR_FAR;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "call_stack"))
        {
            LbrRequest->LbrFilterOptions |= LBR_CALL_STACK;
        }
        else
        {
            ShowMessages("unknown filter option '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Displays the success message appropriate for the completed LBR operation
 *
 * @param LbrRequest
 *
 * @return VOID
 */
VOID
CommandLbrShowSuccessMessage(const HYPERTRACE_LBR_OPERATION_PACKETS * LbrRequest)
{
    switch (LbrRequest->LbrOperationType)
    {
    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE:
        ShowMessages("LBR enabled successfully\n");
        break;
    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE:
        ShowMessages("LBR disabled successfully\n");
        break;
    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FLUSH:
        ShowMessages("LBR branches are flushed\n");
        break;
    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FILTER:
        ShowMessages("LBR filter options are updated successfully\n");
        break;
    default:
        ShowMessages("unknown LBR operation type\n");
        break;
    }
}

/**
 * @brief !lbr command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandLbr(vector<CommandToken> CommandTokens, string Command)
{
    HYPERTRACE_LBR_OPERATION_PACKETS LbrRequest  = {0};
    BOOLEAN                          ParseResult = FALSE;

    if (CommandTokens.size() == 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandLbrHelp();
        return;
    }

    //
    // Dispatch to the appropriate parser based on the subcommand
    //
    if (CompareLowerCaseStrings(CommandTokens.at(1), "enable") ||
        CompareLowerCaseStrings(CommandTokens.at(1), "disable") ||
        CompareLowerCaseStrings(CommandTokens.at(1), "flush"))
    {
        ParseResult = CommandLbrParseSimpleOperation(CommandTokens, &LbrRequest);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "filter"))
    {
        ParseResult = CommandLbrParseFilterOperation(CommandTokens, &LbrRequest);
    }
    else
    {
        ParseResult = FALSE;
    }

    if (!ParseResult)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandLbrHelp();
        return;
    }

    //
    // Send the LBR operation request
    //
    if (CommandLbrSendRequest(&LbrRequest))
    {
        CommandLbrShowSuccessMessage(&LbrRequest);
    }
    else
    {
        ShowErrorMessage(LbrRequest.KernelStatus);
    }
}
