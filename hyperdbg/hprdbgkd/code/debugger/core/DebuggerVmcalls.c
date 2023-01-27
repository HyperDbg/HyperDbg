/**
 * @file DebuggerVmcalls.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of debugger VMCALLs
 * @details
 *
 * @version 0.2
 * @date 2023-01-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Termination function for external-interrupts
 *
 * @param CoreId
 * @param VmcallNumber
 * @param OptionalParam1
 * @param OptionalParam2
 * @param OptionalParam3
 *
 * @return BOOLEAN
 */
BOOLEAN
DebuggerVmcallHandler(UINT32 CoreId,
                      UINT64 VmcallNumber,
                      UINT64 OptionalParam1,
                      UINT64 OptionalParam2,
                      UINT64 OptionalParam3)
{
    BOOLEAN Result = FALSE;

    switch (VmcallNumber)
    {
    case DEBUGGER_VMCALL_VM_EXIT_HALT_SYSTEM:
    {
        KdHandleBreakpointAndDebugBreakpointsCallback(CoreId,
                                                      DEBUGGEE_PAUSING_REASON_REQUEST_FROM_DEBUGGER,
                                                      NULL);
        Result = TRUE;
        break;
    }
    case DEBUGGER_VMCALL_VM_EXIT_HALT_SYSTEM_AS_A_RESULT_OF_TRIGGERING_EVENT:
    {
        DEBUGGER_TRIGGERED_EVENT_DETAILS TriggeredEventDetail = {0};
        PGUEST_REGS                      TempReg              = NULL;
        PROCESSOR_DEBUGGING_STATE *      DbgState             = &g_DbgState[CoreId];

        TriggeredEventDetail.Context = OptionalParam1;
        TriggeredEventDetail.Tag     = OptionalParam2;

        TempReg = DbgState->Regs;

        //
        // We won't send current vmcall registers
        // instead we send the registers provided
        // from the third parameter
        //
        DbgState->Regs = OptionalParam3;

        KdHandleBreakpointAndDebugBreakpointsCallback(CoreId,
                                                      DEBUGGEE_PAUSING_REASON_DEBUGGEE_EVENT_TRIGGERED,
                                                      &TriggeredEventDetail);

        //
        // Restore the register
        //
        DbgState->Regs = TempReg;

        Result = TRUE;
        break;
    }
    case DEBUGGER_VMCALL_SIGNAL_DEBUGGER_EXECUTION_FINISHED:
    {
        KdSendCommandFinishedSignal(CoreId);

        Result = TRUE;
        break;
    }
    case DEBUGGER_VMCALL_SEND_MESSAGES_TO_DEBUGGER:
    {
        //
        // Kernel debugger is active, we should send the bytes over serial
        //
        KdLoggingResponsePacketToDebugger(
            OptionalParam1,
            OptionalParam2,
            OPERATION_LOG_INFO_MESSAGE);

        Result = TRUE;
        break;
    }
    case DEBUGGER_VMCALL_SEND_GENERAL_BUFFER_TO_DEBUGGER:
    {
        //
        // Cast the buffer received to perform sending buffer and possibly
        // halt the the debuggee
        //
        PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER DebuggeeBufferRequest =
            OptionalParam1;

        KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                   DebuggeeBufferRequest->RequestedAction,
                                   (UINT64)DebuggeeBufferRequest + (SIZEOF_DEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER),
                                   DebuggeeBufferRequest->LengthOfBuffer);

        //
        // Check if we expect a buffer and command from the debugger or the
        // request is just finished
        //
        if (DebuggeeBufferRequest->PauseDebuggeeWhenSent)
        {
            KdHandleBreakpointAndDebugBreakpointsCallback(CoreId,
                                                          DEBUGGEE_PAUSING_REASON_PAUSE_WITHOUT_DISASM,
                                                          NULL);
        }

        Result = TRUE;
        break;
    }
    default:
        Result = FALSE;
        LogError("Err, invalid VMCALL in top-level debugger");

        break;
    }

    return Result;
}
