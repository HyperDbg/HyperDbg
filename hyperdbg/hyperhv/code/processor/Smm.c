/**
 * @file Smm.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines for operations related to System Management Mode (SMM)
 * @details
 *
 * @version 0.15
 * @date 2025-08-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/*
 * @brief Read the count of System Management Interrupts (SMIs)
 *
 * @return UINT64
 */
UINT64
SmmReadSmiCount()
{
    UINT64 SmiCount = 0;

    //
    // Read the SMI count from MSR
    //
    SmiCount = (UINT64)__readmsr(MSR_SMI_COUNT);

    return SmiCount;
}

/*
 * @brief Trigger a Power SMI
 *
 * @return BOOLEAN
 */
BOOLEAN
SmmTriggerPowerSmi()
{
    UINT8 SmmResponse = 0;

    //
    // check the initial value received from 0xB3 port
    //
    SmmResponse = __inbyte(0xb2);

    //
    // write to 0xB2 port to cause SMI
    //
    __outbyte(0xb2, SMI_TRIGGER_POWER_VALUE);

    //
    // Check the respose in port 0xB3
    //
    SmmResponse = __inbyte(0xb2);

    if (SmmResponse == SMI_TRIGGER_POWER_VALUE)
    {
        //
        // If the response is SMI_TRIGGER_POWER_VALUE, it means the SMI was triggered successfully
        //
        return TRUE;
    }
    else
    {
        //
        // If the response is not SMI_TRIGGER_POWER_VALUE, it means the SMI was not triggered successfully
        //
        return FALSE;
    }
}

/**
 * @brief Perform actions related to System Management Interrupts (SMIs)
 *
 * @param SmiOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
SmmPerformSmiOperation(SMI_OPERATION_PACKETS * SmiOperationRequest, BOOLEAN ApplyFromVmxRootMode)
{
    BOOLEAN Status = FALSE;

    UNREFERENCED_PARAMETER(ApplyFromVmxRootMode);

    //
    // Check the SMI operation type and perform the corresponding action
    //
    switch (SmiOperationRequest->SmiOperationType)
    {
    case SMI_OPERATION_REQUEST_TYPE_READ_COUNT:

        //
        // Read the SMI count from the MSR
        //
        SmiOperationRequest->SmiCount = SmmReadSmiCount();

        Status = TRUE;
        break;

    case SMI_OPERATION_REQUEST_TYPE_TRIGGER_POWER_SMI:

        if (SmmTriggerPowerSmi())
        {
            //
            // If the SMI was triggered successfully
            //
            Status = TRUE;
        }
        else
        {
            //
            // If the SMI was not triggered successfully, set error status
            //
            SmiOperationRequest->KernelStatus = DEBUGGER_ERROR_UNABLE_TO_TRIGGER_SMI;
        }

        break;

    default:

        Status                            = FALSE;
        SmiOperationRequest->KernelStatus = DEBUGGER_ERROR_INVALID_SMI_OPERATION_PARAMETERS;

        break;
    }

    //
    // Set the status of the SMI operation request
    //
    if (Status)
    {
        SmiOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return Status;
}
