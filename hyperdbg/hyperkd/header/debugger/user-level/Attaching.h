/**
 * @file Attaching.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for attaching and detaching for debugging user-mode processes
 * @details
 * @version 0.1
 * @date 2021-12-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Constants					//
//////////////////////////////////////////////////

/**
 * @brief Maximum actions in paused threads storage
 *
 */
#define MAX_USER_ACTIONS_FOR_THREADS 3

/**
 * @brief Maximum threads that a process thread holder might have
 *
 */
#define MAX_THREADS_IN_A_PROCESS_HOLDER 100

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
    UINT64     UsermodeReservedBuffer;
    UINT64     EntrypointOfMainModule;
    UINT64     BaseAddressOfMainModule;
    PEPROCESS  Eprocess;
    UINT32     ProcessId;
    BOOLEAN    Is32Bit;
    BOOLEAN    IsOnTheStartingPhase;
    BOOLEAN    IsOnThreadInterceptingPhase;
    BOOLEAN    CheckCallBackForInterceptingFirstInstruction; // checks for the callbacks for interceptions of the very first instruction (used by RE Machine)
    LIST_ENTRY ThreadsListHead;

} USERMODE_DEBUGGING_PROCESS_DETAILS, *PUSERMODE_DEBUGGING_PROCESS_DETAILS;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
AttachingInitialize();

BOOLEAN
AttachingCheckThreadInterceptionWithUserDebugger(UINT32 CoreId);

BOOLEAN
AttachingConfigureInterceptingThreads(UINT64 ProcessDebuggingToken, BOOLEAN Enable);

VOID
AttachingTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS Request);

VOID
AttachingHandleEntrypointInterception(PROCESSOR_DEBUGGING_STATE * DbgState);

VOID
AttachingRemoveAndFreeAllProcessDebuggingDetails();

PUSERMODE_DEBUGGING_PROCESS_DETAILS
AttachingFindProcessDebuggingDetailsByToken(UINT64 Token);

PUSERMODE_DEBUGGING_PROCESS_DETAILS
AttachingFindProcessDebuggingDetailsByProcessId(UINT32 ProcessId);

BOOLEAN
AttachingQueryDetailsOfActiveDebuggingThreadsAndProcesses(PVOID BufferToStoreDetails, UINT32 BufferSize);

BOOLEAN
AttachingCheckUnhandledEptViolation(UINT32 CoreId,
                                    UINT64 ViolationQualification,
                                    UINT64 GuestPhysicalAddr);

BOOLEAN
AttachingReachedToValidLoadedModule(PROCESSOR_DEBUGGING_STATE *         DbgState,
                                    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail);
