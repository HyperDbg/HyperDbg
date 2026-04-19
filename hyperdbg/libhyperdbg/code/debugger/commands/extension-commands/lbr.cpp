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

    ShowMessages("\n");
    ShowMessages("\t\te.g : !lbr enable\n");
    ShowMessages("\t\te.g : !lbr disable\n");
    ShowMessages("\t\te.g : !lbr show\n");
}

/**
 * @brief Send LBR requests
 *
 * @param LbrRequest
 *
 * @return VOID
 */
BOOLEAN
CommandLbrSendRequest(HYPERTRACE_OPERATION_PACKETS * LbrRequest)
{
    BOOL  Status;
    ULONG ReturnedLength;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send the request over serial kernel debugger
        //
        if (!KdSendHyperTracePacketsToDebuggee(LbrRequest, SIZEOF_HYPERTRACE_OPERATION_PACKETS))
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
            g_DeviceHandle,                      // Handle to device
            IOCTL_PERFORM_HYPERTRACE_OPERATION,  // IO Control Code (IOCTL)
            LbrRequest,                          // Input Buffer to driver.
            SIZEOF_HYPERTRACE_OPERATION_PACKETS, // Input buffer length
            LbrRequest,                          // Output Buffer from driver.
            SIZEOF_HYPERTRACE_OPERATION_PACKETS, // Length of output buffer in bytes.
            &ReturnedLength,                     // Bytes placed in buffer.
            NULL                                 // synchronous call
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
HyperDbgPerformLbrOperation(HYPERTRACE_OPERATION_PACKETS * LbrRequest)
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
    HYPERTRACE_OPERATION_PACKETS LbrRequest = {0};

    if (CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());

        CommandLbrHelp();
        return;
    }

    if (CompareLowerCaseStrings(CommandTokens.at(1), "enable"))
    {
        LbrRequest.HyperTraceOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "disable"))
    {
        LbrRequest.HyperTraceOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "show"))
    {
        LbrRequest.HyperTraceOperationType = HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_SHOW;
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
        if (LbrRequest.HyperTraceOperationType == HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE)
        {
            ShowMessages("LBR enabled successfully\n");
        }
        else if (LbrRequest.HyperTraceOperationType == HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE)
        {
            ShowMessages("LBR disabled successfully\n");
        }
        else if (LbrRequest.HyperTraceOperationType == HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_SHOW)
        {
            ShowMessages("LBR branches are shown\n");
        }
    }
    else
    {
        ShowErrorMessage(LbrRequest.KernelStatus);
        return;
    }
}
