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
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/*
 * Intel LBR_SELECT bits
 *
 * Hardware branch filter (not available on all CPUs)
 */
#define LBR_KERNEL_BIT     0 /* do not capture at ring0 */
#define LBR_USER_BIT       1 /* do not capture at ring > 0 */
#define LBR_JCC_BIT        2 /* do not capture conditional branches */
#define LBR_REL_CALL_BIT   3 /* do not capture relative calls */
#define LBR_IND_CALL_BIT   4 /* do not capture indirect calls */
#define LBR_RETURN_BIT     5 /* do not capture near returns */
#define LBR_IND_JMP_BIT    6 /* do not capture indirect jumps */
#define LBR_REL_JMP_BIT    7 /* do not capture relative jumps */
#define LBR_FAR_BIT        8 /* do not capture far branches */
#define LBR_CALL_STACK_BIT 9 /* enable call stack: not available on all CPUs */

/*
 * We mask it out before writing it to
 * the actual MSR. But it helps the constraint code to understand
 * that this is a separate configuration.
 */
#define LBR_KERNEL     (1 << LBR_KERNEL_BIT)
#define LBR_USER       (1 << LBR_USER_BIT)
#define LBR_JCC        (1 << LBR_JCC_BIT)
#define LBR_REL_CALL   (1 << LBR_REL_CALL_BIT)
#define LBR_IND_CALL   (1 << LBR_IND_CALL_BIT)
#define LBR_RETURN     (1 << LBR_RETURN_BIT)
#define LBR_IND_JMP    (1 << LBR_IND_JMP_BIT)
#define LBR_REL_JMP    (1 << LBR_REL_JMP_BIT)
#define LBR_FAR        (1 << LBR_FAR_BIT)
#define LBR_CALL_STACK (1 << LBR_CALL_STACK_BIT)

/**
 * @brief help of the !lbr command
 *
 * @return VOID
 */
VOID
CommandLbrHelp()
{
    ShowMessages("!lbr : enables and disables Last Branch Record (LBR).\n");

    ShowMessages("syntax : \t!lbr [Function (string)]\n");
    ShowMessages("syntax : \t!lbr [filter FilterOptions (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !lbr enable\n");
    ShowMessages("\t\te.g : !lbr disable\n");
    ShowMessages("\t\te.g : !lbr save\n");
    ShowMessages("\t\te.g : !lbr dump\n");
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

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send the request over serial kernel debugger
        //
        if (!KdSendHyperTraceLbrPacketsToDebuggee(LbrRequest, SIZEOF_HYPERTRACE_LBR_OPERATION_PACKETS))
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
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
    HYPERTRACE_LBR_OPERATION_PACKETS LbrRequest = {0};

    if (CommandTokens.size() == 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());

        CommandLbrHelp();
        return;
    }

    //
    // Parse the LBR operation type
    //
    if (CompareLowerCaseStrings(CommandTokens.at(1), "enable") && CommandTokens.size() == 2)
    {
        LbrRequest.LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "disable") && CommandTokens.size() == 2)
    {
        LbrRequest.LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "save") && CommandTokens.size() == 2)
    {
        LbrRequest.LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_SAVE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "dump") && CommandTokens.size() == 2)
    {
        LbrRequest.LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DUMP;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "flush") && CommandTokens.size() == 2)
    {
        LbrRequest.LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FLUSH;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "filter"))
    {
        LbrRequest.LbrOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FILTER;
        LbrRequest.LbrFilterOptions = 0; // filter without any option means capture everything

        //
        // Parse filter options
        //
        for (size_t i = 2; i < CommandTokens.size(); i++)
        {
            if (CompareLowerCaseStrings(CommandTokens.at(i), "kernel"))
            {
                LbrRequest.LbrFilterOptions |= LBR_KERNEL;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "user"))
            {
                LbrRequest.LbrFilterOptions |= LBR_USER;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "jcc"))
            {
                LbrRequest.LbrFilterOptions |= LBR_JCC;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "rel_call"))
            {
                LbrRequest.LbrFilterOptions |= LBR_REL_CALL;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "ind_call"))
            {
                LbrRequest.LbrFilterOptions |= LBR_IND_CALL;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "return"))
            {
                LbrRequest.LbrFilterOptions |= LBR_RETURN;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "ind_jmp"))
            {
                LbrRequest.LbrFilterOptions |= LBR_IND_JMP;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "rel_jmp"))
            {
                LbrRequest.LbrFilterOptions |= LBR_REL_JMP;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "far"))
            {
                LbrRequest.LbrFilterOptions |= LBR_FAR;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "call_stack"))
            {
                LbrRequest.LbrFilterOptions |= LBR_CALL_STACK;
            }
            else
            {
                ShowMessages("unknown filter option '%s'\n\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                CommandLbrHelp();
                return;
            }
        }
    }
    else
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
        if (LbrRequest.LbrOperationType == HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE)
        {
            ShowMessages("LBR enabled successfully\n");
        }
        else if (LbrRequest.LbrOperationType == HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE)
        {
            ShowMessages("LBR disabled successfully\n");
        }
        else if (LbrRequest.LbrOperationType == HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_SAVE)
        {
            ShowMessages("LBR branches are saved\n");
        }
        else if (LbrRequest.LbrOperationType == HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DUMP)
        {
            ShowMessages("LBR branches are shown\n");
        }
        else if (LbrRequest.LbrOperationType == HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FLUSH)
        {
            ShowMessages("LBR branches are flush\n");
        }
        else if (LbrRequest.LbrOperationType == HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_FILTER)
        {
            ShowMessages("LBR filter options are updated successfully\n");
        }
        else
        {
            ShowMessages("unknown LBR operation type\n");
        }
    }
    else
    {
        ShowErrorMessage(LbrRequest.KernelStatus);
        return;
    }
}
