/**
 * @file ValidateEvents.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of debugger functions for validating events
 * @details
 *
 * @version 0.7
 * @date 2023-10-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Validating monitor memory hook events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN
 */
BOOLEAN
ValidateEventMonitor(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                     PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                     BOOLEAN                           InputFromVmxRoot)
{
    UINT32 TempPid;

    //
    // First check if the address are valid
    //
    TempPid = EventDetails->ProcessId;
    if (TempPid == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES)
    {
        TempPid = HANDLE_TO_UINT32(PsGetCurrentProcessId());
    }

    //
    // Check if input is coming from VMX-root or not
    // If it's coming from VMX-root, then as switching
    // to another process is not possible, we'll return
    // an error
    //
    if (InputFromVmxRoot && TempPid != HANDLE_TO_UINT32(PsGetCurrentProcessId()))
    {
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_PROCESS_ID_CANNOT_BE_SPECIFIED_WHILE_APPLYING_EVENT_FROM_VMX_ROOT_MODE;
        return FALSE;
    }

    //
    // Check whether address is valid or not based on whether the event needs
    // to be applied directly from VMX-root mode or not
    //
    if (InputFromVmxRoot)
    {
        if (VirtualAddressToPhysicalAddressOnTargetProcess((PVOID)EventDetails->Options.OptionalParam1) == (UINT64)NULL ||
            VirtualAddressToPhysicalAddressOnTargetProcess((PVOID)EventDetails->Options.OptionalParam2) == (UINT64)NULL)
        {
            //
            // Address is invalid (Set the error)
            //

            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }
    }
    else
    {
        if (VirtualAddressToPhysicalAddressByProcessId((PVOID)EventDetails->Options.OptionalParam1, TempPid) == (UINT64)NULL ||
            VirtualAddressToPhysicalAddressByProcessId((PVOID)EventDetails->Options.OptionalParam2, TempPid) == (UINT64)NULL)
        {
            //
            // Address is invalid (Set the error)
            //

            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }
    }

    //
    // Check if the 'to' is greater that 'from'
    //
    if (EventDetails->Options.OptionalParam1 >= EventDetails->Options.OptionalParam2)
    {
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_INVALID_ADDRESS;
        return FALSE;
    }

    //
    // The event parameters are valid at this stage
    //
    return TRUE;
}

/**
 * @brief Validating exception events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN
 */
BOOLEAN
ValidateEventException(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                       PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                       BOOLEAN                           InputFromVmxRoot)
{
    UNREFERENCED_PARAMETER(InputFromVmxRoot);

    //
    // Check if the exception entry doesn't exceed the first 32 entry (start from zero)
    //
    if (EventDetails->Options.OptionalParam1 != DEBUGGER_EVENT_EXCEPTIONS_ALL_FIRST_32_ENTRIES && EventDetails->Options.OptionalParam1 >= 31)
    {
        //
        // We don't support entries other than first 32 IDT indexes,
        // it is because we use exception bitmaps and in order to support
        // more than 32 indexes we should use pin-based external interrupt
        // exiting which is completely different
        //
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_EXCEPTION_INDEX_EXCEED_FIRST_32_ENTRIES;
        return FALSE;
    }

    //
    // The event parameters are valid at this stage
    //
    return TRUE;
}

/**
 * @brief Validating interrupt events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN
 */
BOOLEAN
ValidateEventInterrupt(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                       PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                       BOOLEAN                           InputFromVmxRoot)
{
    UNREFERENCED_PARAMETER(InputFromVmxRoot);

    //
    // Check if the exception entry is between 32 to 255
    //
    if (!(EventDetails->Options.OptionalParam1 >= 32 && EventDetails->Options.OptionalParam1 <= 0xff))
    {
        //
        // The IDT Entry is either invalid or is not in the range
        // of the pin-based external interrupt exiting controls
        //
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_INTERRUPT_INDEX_IS_NOT_VALID;
        return FALSE;
    }

    //
    // The event parameters are valid at this stage
    //
    return TRUE;
}

/**
 * @brief Validating trap exec events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN
 */
BOOLEAN
ValidateEventTrapExec(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                      PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                      BOOLEAN                           InputFromVmxRoot)
{
    UNREFERENCED_PARAMETER(InputFromVmxRoot);

    //
    // Check if the execution mode is valid or not
    //
    if (EventDetails->Options.OptionalParam1 != DEBUGGER_EVENT_MODE_TYPE_USER_MODE &&
        EventDetails->Options.OptionalParam1 != DEBUGGER_EVENT_MODE_TYPE_KERNEL_MODE &&
        EventDetails->Options.OptionalParam1 != DEBUGGER_EVENT_MODE_TYPE_USER_MODE_AND_KERNEL_MODE)
    {
        //
        // The execution mode is not correctly applied
        //
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_MODE_EXECUTION_IS_INVALID;
        return FALSE;
    }

    //
    // The event parameters are valid at this stage
    //
    return TRUE;
}

/**
 * @brief Validating EPT hook exec (hidden breakpoint and inline hook) events
 *
 * @param Event The created event object
 * @param ResultsToReturn Result buffer that should be returned to
 * the user-mode
 * @param InputFromVmxRoot Whether the input comes from VMX root-mode or IOCTL
 *
 * @return BOOLEAN
 */
BOOLEAN
ValidateEventEptHookHiddenBreakpointAndInlineHooks(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                                                   PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                                                   BOOLEAN                           InputFromVmxRoot)
{
    UINT32 TempPid;

    //
    // First check if the address are valid
    //
    TempPid = EventDetails->ProcessId;

    if (TempPid == DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES)
    {
        TempPid = HANDLE_TO_UINT32(PsGetCurrentProcessId());
    }

    //
    // Check if input is coming from VMX-root or not
    // If it's coming from VMX-root, then as switching
    // to another process is not possible, we'll return
    // an error
    //
    if (InputFromVmxRoot && TempPid != HANDLE_TO_UINT32(PsGetCurrentProcessId()))
    {
        ResultsToReturn->IsSuccessful = FALSE;
        ResultsToReturn->Error        = DEBUGGER_ERROR_PROCESS_ID_CANNOT_BE_SPECIFIED_WHILE_APPLYING_EVENT_FROM_VMX_ROOT_MODE;
        return FALSE;
    }

    //
    // Check whether address is valid or not based on whether the event needs
    // to be applied directly from VMX-root mode or not
    //
    if (InputFromVmxRoot)
    {
        if (VirtualAddressToPhysicalAddressOnTargetProcess((PVOID)EventDetails->Options.OptionalParam1) == (UINT64)NULL)
        {
            //
            // Address is invalid (Set the error)
            //

            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }
    }
    else
    {
        if (VirtualAddressToPhysicalAddressByProcessId((PVOID)EventDetails->Options.OptionalParam1, TempPid) == (UINT64)NULL)
        {
            //
            // Address is invalid (Set the error)
            //

            ResultsToReturn->IsSuccessful = FALSE;
            ResultsToReturn->Error        = DEBUGGER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }
    }

    //
    // The event parameters are valid at this stage
    //
    return TRUE;
}
