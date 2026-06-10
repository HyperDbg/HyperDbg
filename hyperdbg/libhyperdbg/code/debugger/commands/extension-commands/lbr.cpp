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

//
// Global Variables
//
extern HANDLE  g_DeviceHandle;
extern BOOLEAN g_IsHyperTraceModuleLoaded;

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
    ShowMessages("\t\te.g : !lbr filter call_stack user\n");
    ShowMessages("\t\te.g : !lbr filter call_stack kernel\n");

    ShowMessages("\nlist of filter options: \n");
    ShowMessages("\t kernel:         do not capture at ring0\n");
    ShowMessages("\t user:           do not capture at ring > 0\n");
    ShowMessages("\t jcc:            do not capture conditional branches\n");
    ShowMessages("\t rel_call:       do not capture relative calls\n");
    ShowMessages("\t ind_call:       do not capture indirect calls\n");
    ShowMessages("\t return:         do not capture near returns\n");
    ShowMessages("\t ind_jmp:        do not capture indirect jumps\n");
    ShowMessages("\t rel_jmp:        do not capture relative jumps\n");
    ShowMessages("\t far:            do not capture far branches (only in legacy LBR. check docs for details)\n");
    ShowMessages("\t other_branches: do not capture jmp/call ptr*, jmp/call m*, ret (0c8h), sys*, interrupts,\n");
    ShowMessages("\t                 exceptions (other than debug exceptions), iret, int3, intn, into, tsx abort\n");
    ShowMessages("\t                 eenter, eresume, eexit, aex, init, sipi, rsm (only in ARCH LBR. check docs for details)\n");
    ShowMessages("\t call_stack:     enable LBR stack to use LIFO filtering to capture call stack profile\n");
    ShowMessages("\t                 not available on CPUs older than Haswell. for this item you can only specify the 'user'\n");
    ShowMessages("\t                 or the 'kernel'. it prevents all types of branches except calls and rets\n");
    ShowMessages("\t (no option):    capture everything (default option)\n");

    ShowMessages("\nnote 1: LBR is usually not supported (or is emulated) in nested virtualization (VM) environments\n");
    ShowMessages("note 2: LBR will be disabled if there is a debug-break (#DB) condition, such as the trap flags or\n");
    ShowMessages("        hardware debug registers (to learn how to mitigate this, check the documentation)\n");
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

    AssertShowMessageReturnStmt(g_IsHyperTraceModuleLoaded, g_DeviceHandle, ASSERT_MESSAGE_HYPERTRACE_NOT_LOADED, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

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
 * @brief Check validity of the filter options in case of call_stack mode
 *
 * @param LbrRequest
 * @return BOOLEAN TRUE if the filter options are valid in case of call_stack mode
 */
BOOLEAN
CommandLbrValidateCallStackFilterOptions(HYPERTRACE_LBR_OPERATION_PACKETS * LbrRequest)
{
    if (LbrRequest->LbrFilterOptions & LBR_CALL_STACK)
    {
        //
        // Call-stack mode should be used with branch type enabling configured to capture only CALLs (NEAR_REL_CALL and
        // NEAR_IND_CALL) and RETs (NEAR_RET). if the user specifed LBR_REL_CALL | LBR_IND_CALL | LBR_RETURN then
        // it is invalid
        //
        if ((LbrRequest->LbrFilterOptions & (LBR_REL_CALL | LBR_IND_CALL | LBR_RETURN)))
        {
            ShowMessages("err, invalid filter options for 'call_stack' mode. when the 'call_stack' is enabled,"
                         " 'rel_call', 'ind_call', and 'return' could not be specified as filter options\n\n");
            return FALSE;
        }
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

    for (SIZE_T i = 2; i < CommandTokens.size(); i++)
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
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "far") ||
                 CompareLowerCaseStrings(CommandTokens.at(i), "other_branch") ||
                 CompareLowerCaseStrings(CommandTokens.at(i), "other_branches"))
        {
            LbrRequest->LbrFilterOptions |= LBR_FAR_OTHER_BRANCHES;
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

    //
    // Call-stack mode should be used with branch type enabling configured to capture only CALLs (NEAR_REL_CALL and
    // NEAR_IND_CALL) and RETs (NEAR_RET). When configured in this manner, the LBR array emulates a call stack,
    // where CALLs are "pushed" and RETs "pop" them off the stack. If other branch types (JCC, NEAR_*_JMP, or
    // OTHER_BRANCH) are enabled for recording with call-stack mode, LBR behavior may be undefined, so we will
    // mask out any branch type filters that are not CALLs or RETs when call-stack mode is requested to ensure
    // we are correctly emulating a call stack and avoiding undefined behavior
    //

    //
    // If it is call_stack then we only keep the user and kernel bit and filter all branch types
    // except calls and rets to ensure we are only capturing call stack profile
    //
    if (LbrRequest->LbrFilterOptions & LBR_CALL_STACK)
    {
        //
        // Validate the filter options in case of call_stack mode
        //
        if (CommandLbrValidateCallStackFilterOptions(LbrRequest) == FALSE)
        {
            return FALSE;
        }

        //
        // Preserve only the user/kernel privilege bits from the original options,
        // then apply the mandatory call stack base flags
        //
        LbrRequest->LbrFilterOptions = LBR_CALL_STACK_BASE_FLAGS | (LbrRequest->LbrFilterOptions & (LBR_KERNEL | LBR_USER));
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
    // Check if the HyperTrace module is loaded, as it is required for LBR operations
    //
    AssertShowMessageReturnStmt(g_IsHyperTraceModuleLoaded, g_DeviceHandle, ASSERT_MESSAGE_HYPERTRACE_NOT_LOADED, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

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
