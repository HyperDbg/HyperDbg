/**
 * @file Ud.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for routines related to user mode debugging
 * @details
 * @version 0.1
 * @date 2022-01-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Constants					//
//////////////////////////////////////////////////

/**
 * @brief How long the IOCTL thread waits for a paused thread to run a command
 * (in 100-nanosecond units: 10 seconds)
 *
 */
#define UD_COMMAND_COMPLETION_TIMEOUT (10LL * 1000 * 1000 * 10)

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief Description of each active thread in user-mode attaching
 * mechanism
 *
 */
typedef struct _USERMODE_DEBUGGING_PROCESS_DETAILS
{
    UINT64     Token;
    BOOLEAN    Enabled;
    PVOID      PebAddressToMonitor;
    UINT32     ActiveThreadId; // active thread
    GUEST_REGS Registers;      // active thread
    UINT64     Context;        // $context
    LIST_ENTRY AttachedProcessList;
    UINT64     EntrypointOfMainModule;
    UINT64     BaseAddressOfMainModule;
    PEPROCESS  Eprocess;
    UINT32     ProcessId;
    BOOLEAN    Is32Bit;
    BOOLEAN    IsOnTheStartingPhase;
    BOOLEAN    IsOnThreadInterceptingPhase;
    BOOLEAN    CheckCallBackForInterceptingFirstInstruction; // checks for the callbacks for interceptions of the very first instruction (used by RE Machine)
    UINT64     EntrypointBreakpointId;                       // the breakpoint on the entrypoint until it is hit (zero if none)
    LIST_ENTRY ThreadsListHead;

} USERMODE_DEBUGGING_PROCESS_DETAILS, *PUSERMODE_DEBUGGING_PROCESS_DETAILS;

//////////////////////////////////////////////////
//				      Functions     			//
//////////////////////////////////////////////////

BOOLEAN
UdInitializeUserDebugger();

VOID
UdUninitializeUserDebugger();

BOOLEAN
UdHandleInstantBreak(PROCESSOR_DEBUGGING_STATE *         DbgState,
                     DEBUGGEE_PAUSING_REASON             Reason,
                     PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail);

VOID
UdApplyHardwareDebugRegister(PVOID TargetAddress);

VOID
UdDisarmStepOverBreakpoint();

BOOLEAN
UdCheckAndHandleBreakpointsAndDebugBreaks(PROCESSOR_DEBUGGING_STATE *       DbgState,
                                          DEBUGGEE_PAUSING_REASON           Reason,
                                          PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails);

BOOLEAN
UdDispatchUsermodeCommands(PDEBUGGER_UD_COMMAND_PACKET ActionRequest,
                           UINT32                      ActionRequestInputLength,
                           UINT32                      ActionRequestOutputLength);

BOOLEAN
UdCheckForCommand(PROCESSOR_DEBUGGING_STATE *         DbgState,
                  PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail);

BOOLEAN
UdHandleDebugEventsWhenUserDebuggerIsAttached(PROCESSOR_DEBUGGING_STATE * DbgState,
                                              BOOLEAN                     TrapSetByDebugger);

VOID
UdSendFormatsFunctionResult(UINT64 Value);
