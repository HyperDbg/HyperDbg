/**
 * @file smi.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief !smi command
 * @details
 * @version 0.15
 * @date 2025-08-02
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
 * @brief help of the !smi command
 *
 * @return VOID
 */
VOID
CommandSmiHelp()
{
    ShowMessages("!smi : shows details and triggers functionalities related to System Management Interrupt (SMI).\n\n");

    ShowMessages("syntax : \t!smi [Function (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !smi count\n");
    ShowMessages("\t\te.g : !smi trigger\n");
}

/**
 * @brief Send SMI requests
 *
 * @param SmiRequest
 *
 * @return VOID
 */
BOOLEAN
CommandSmiSendRequest(SMI_OPERATION_PACKETS * SmiOperationRequest)
{
    BOOL  Status;
    ULONG ReturnedLength;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send the request over serial kernel debugger
        //
        if (!KdSendSmiPacketsToDebuggee(SmiOperationRequest, SIZEOF_SMI_OPERATION_PACKETS))
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
            g_DeviceHandle,               // Handle to device
            IOCTL_PERFORM_SMI_OPERATION,  // IO Control Code (IOCTL)
            SmiOperationRequest,          // Input Buffer to driver.
            SIZEOF_SMI_OPERATION_PACKETS, // Input buffer length
            SmiOperationRequest,          // Output Buffer from driver.
            SIZEOF_SMI_OPERATION_PACKETS, // Length of output buffer in bytes.
            &ReturnedLength,              // Bytes placed in buffer.
            NULL                          // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());

            return FALSE;
        }

        if (SmiOperationRequest->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
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
 * @brief Request to perform an SMI operation
 *
 * @param SmiOperation
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgPerformSmiOperation(SMI_OPERATION_PACKETS * SmiOperation)
{
    return CommandSmiSendRequest(SmiOperation);
}

/**
 * @brief !smi command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandSmi(vector<CommandToken> CommandTokens, string Command)
{
    SMI_OPERATION_PACKETS SmiOperationRequest = {0};

    if (CommandTokens.size() != 2)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());

        CommandSmiHelp();
        return;
    }

    if (CompareLowerCaseStrings(CommandTokens.at(1), "count"))
    {
        SmiOperationRequest.SmiOperationType = SMI_OPERATION_REQUEST_TYPE_READ_COUNT;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "trigger"))
    {
        SmiOperationRequest.SmiOperationType = SMI_OPERATION_REQUEST_TYPE_TRIGGER_POWER_SMI;
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandSmiHelp();
        return;
    }

    //
    // Send the SMI operation request
    //
    if (CommandSmiSendRequest(&SmiOperationRequest))
    {
        if (SmiOperationRequest.SmiOperationType == SMI_OPERATION_REQUEST_TYPE_READ_COUNT)
        {
            if (SmiOperationRequest.SmiCount == 0)
            {
                ShowMessages("SMI count: 0 (for security reasons, nested-virtualization environment (VMs) are unable to communicate with UEFI firmware)\n");
            }
            else
            {
                ShowMessages("SMI count: 0x%x\n", SmiOperationRequest.SmiCount);
            }
        }
        else if (SmiOperationRequest.SmiOperationType == SMI_OPERATION_REQUEST_TYPE_TRIGGER_POWER_SMI)
        {
            ShowMessages("power SMI triggered successfully (you can use '!smi count' to view the number of executed SMIs)\n");
        }
    }
    else
    {
        ShowErrorMessage(SmiOperationRequest.KernelStatus);
        return;
    }
}
