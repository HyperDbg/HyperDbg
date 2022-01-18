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
 * @brief Maximum threads that a process might have
 * 
 */
#define MAX_THREADS_IN_A_PROCESS 100

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief Details of each thread in process
 * 
 */
typedef struct _USERMODE_DEBUGGING_THREAD_DETAILS
{
    UINT32                     ThreadId;
    UINT64                     ThreadRip; // if IsPaused is TRUE
    BOOLEAN                    IsPaused;
    BOOLEAN                    IsRflagsTrapFlagsSet;
    DEBUGGER_UD_COMMAND_ACTION UdAction[MAX_USER_ACTIONS_FOR_THREADS];

} USERMODE_DEBUGGING_THREAD_DETAILS, *PUSERMODE_DEBUGGING_THREAD_DETAILS;

/**
 * @brief Description of each active thread in user-mode attaching 
 * mechanism
 * 
 */
typedef struct _USERMODE_DEBUGGING_PROCESS_DETAILS
{
    UINT64                            Token;
    BOOLEAN                           Enabled;
    PVOID                             PebAddressToMonitor;
    UINT32                            ActiveThreadId; // active thread
    GUEST_REGS                        Registers;      // active thread
    UINT64                            Context;        // $context
    LIST_ENTRY                        AttachedProcessList;
    UINT64                            UsermodeReservedBuffer;
    UINT64                            EntrypointOfMainModule;
    UINT64                            BaseAddressOfMainModule;
    PEPROCESS                         Eprocess;
    UINT32                            ProcessId;
    BOOLEAN                           Is32Bit;
    BOOLEAN                           IsOnTheStartingPhase;
    BOOLEAN                           IsOnThreadInterceptingPhase;
    USERMODE_DEBUGGING_THREAD_DETAILS Threads[MAX_THREADS_IN_A_PROCESS];

} USERMODE_DEBUGGING_PROCESS_DETAILS, *PUSERMODE_DEBUGGING_PROCESS_DETAILS;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
AttachingInitialize();

BOOLEAN
AttachingCheckPageFaultsWithUserDebugger(UINT32                CurrentProcessorIndex,
                                         PGUEST_REGS           GuestRegs,
                                         VMEXIT_INTERRUPT_INFO InterruptExit,
                                         UINT64                Address,
                                         ULONG                 ErrorCode);

BOOLEAN
AttachingConfigureInterceptingThreads(UINT64 ProcessDebuggingToken, BOOLEAN Enable);

VOID
AttachingTargetProcess(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS Request);

VOID
AttachingHandleEntrypointDebugBreak(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs);

VOID
AttachingRemoveAndFreeAllProcessDebuggingDetails();

PUSERMODE_DEBUGGING_PROCESS_DETAILS
AttachingFindProcessDebuggingDetailsByToken(UINT64 Token);

PUSERMODE_DEBUGGING_PROCESS_DETAILS
AttachingFindProcessDebuggingDetailsByProcessId(UINT32 ProcessId);

PUSERMODE_DEBUGGING_THREAD_DETAILS
AttachingFindOrCreateThreadDebuggingDetail(UINT32 ThreadId, PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail);

PUSERMODE_DEBUGGING_THREAD_DETAILS
AttachingGetProcessThreadDetailsByProcessIdAndThreadId(UINT32 ProcessId, UINT32 ThreadId);
