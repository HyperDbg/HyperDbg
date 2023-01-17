/**
 * @file ThreadHolder.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief User debugger's thread holder headers
 * @details 
 * @version 0.1
 * @date 2022-01-28
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				        Locks       			//
//////////////////////////////////////////////////

/**
 * @brief Vmx-root lock for thread holding
 * 
 */
volatile LONG VmxRootThreadHoldingLock;

//////////////////////////////////////////////////
//				      Structures     			//
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
 * @brief The holder for detail of each thread in process
 * 
 */
typedef struct _USERMODE_DEBUGGING_THREAD_HOLDER
{
    LIST_ENTRY                        ThreadHolderList;
    USERMODE_DEBUGGING_THREAD_DETAILS Threads[MAX_THREADS_IN_A_PROCESS_HOLDER];

} USERMODE_DEBUGGING_THREAD_HOLDER, *PUSERMODE_DEBUGGING_THREAD_HOLDER;

//////////////////////////////////////////////////
//				      Functions     			//
//////////////////////////////////////////////////

VOID
ThreadHolderAllocateThreadHoldingBuffers();

BOOLEAN
ThreadHolderAssignThreadHolderToProcessDebuggingDetails(PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail);

BOOLEAN
ThreadHolderIsAnyPausedThreadInProcess(PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail);

PUSERMODE_DEBUGGING_THREAD_DETAILS
ThreadHolderGetProcessThreadDetailsByProcessIdAndThreadId(UINT32 ProcessId, UINT32 ThreadId);

PUSERMODE_DEBUGGING_THREAD_DETAILS
ThreadHolderGetProcessFirstThreadDetailsByProcessId(UINT32 ProcessId);

PUSERMODE_DEBUGGING_PROCESS_DETAILS
ThreadHolderGetProcessDebuggingDetailsByThreadId(UINT32 ThreadId);

PUSERMODE_DEBUGGING_THREAD_DETAILS
ThreadHolderFindOrCreateThreadDebuggingDetail(UINT32 ThreadId, PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail);

BOOLEAN
ThreadHolderApplyActionToPausedThreads(PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails,
                                       PDEBUGGER_UD_COMMAND_PACKET         ActionRequest);

VOID
ThreadHolderFreeHoldingStructures(PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail);

UINT32
ThreadHolderQueryCountOfActiveDebuggingThreadsAndProcesses();

VOID
ThreadHolderQueryDetailsOfActiveDebuggingThreadsAndProcesses(
    USERMODE_DEBUGGING_THREAD_OR_PROCESS_STATE_DETAILS * BufferToStoreDetails,
    UINT32                                               MaxCount);
