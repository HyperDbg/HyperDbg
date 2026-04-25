/**
 * @file pt.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !pt command
 * @details
 * @version 0.19
 * @date 2026-04-25
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
 * @brief help of the !pt command
 *
 * @return VOID
 */
VOID
CommandPtHelp()
{
    ShowMessages("!pt : enables and disables Processor Trace (PT).\n");

    ShowMessages("syntax : \t!pt [Function (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !pt enable\n");
    ShowMessages("\t\te.g : !pt disable\n");
}

/**
 * @brief Send PT requests
 *
 * @param PtRequest
 *
 * @return VOID
 */
BOOLEAN
CommandPtSendRequest(HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    BOOL  Status;
    ULONG ReturnedLength;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send the request over serial kernel debugger
        //
        if (!KdSendHyperTracePtPacketsToDebuggee(PtRequest, SIZEOF_HYPERTRACE_PT_OPERATION_PACKETS))
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
            g_DeviceHandle,                         // Handle to device
            IOCTL_PERFORM_HYPERTRACE_PT_OPERATION,  // IO Control Code (IOCTL)
            PtRequest,                              // Input Buffer to driver.
            SIZEOF_HYPERTRACE_PT_OPERATION_PACKETS, // Input buffer length
            PtRequest,                              // Output Buffer from driver.
            SIZEOF_HYPERTRACE_PT_OPERATION_PACKETS, // Length of output buffer in bytes.
            &ReturnedLength,                        // Bytes placed in buffer.
            NULL                                    // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());

            return FALSE;
        }

        if (PtRequest->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
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
 * @brief Request to perform an PT operation
 *
 * @param PtRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgPerformPtOperation(HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    return CommandPtSendRequest(PtRequest);
}

/**
 * @brief !pt command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandPt(vector<CommandToken> CommandTokens, string Command)
{
    HYPERTRACE_PT_OPERATION_PACKETS PtRequest = {0};

    if (CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());

        CommandPtHelp();
        return;
    }

    if (CompareLowerCaseStrings(CommandTokens.at(1), "enable"))
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_ENABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "disable"))
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "save"))
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_SAVE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "dump"))
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DUMP;
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandPtHelp();
        return;
    }

    //
    // Send the PT operation request
    //
    if (CommandPtSendRequest(&PtRequest))
    {
        if (PtRequest.PtOperationType == HYPERTRACE_PT_OPERATION_REQUEST_TYPE_ENABLE)
        {
            ShowMessages("PT enabled successfully\n");
        }
        else if (PtRequest.PtOperationType == HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE)
        {
            ShowMessages("PT disabled successfully\n");
        }
        else if (PtRequest.PtOperationType == HYPERTRACE_PT_OPERATION_REQUEST_TYPE_SAVE)
        {
            ShowMessages("PT branches are saved\n");
        }
        else if (PtRequest.PtOperationType == HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DUMP)
        {
            ShowMessages("PT branches are shown\n");
        }
    }
    else
    {
        ShowErrorMessage(PtRequest.KernelStatus);
        return;
    }
}
