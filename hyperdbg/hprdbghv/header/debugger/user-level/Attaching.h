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
/**
 * @brief Maximum number of CR3 registers that a process can have
 * @details Generally, a process has one cr3 but after meltdown KPTI
 * another Cr3 is added and separated the kernel and user CR3s, recently
 * I've noticed from a tweet from Petr Benes that there might be other cr3s
 * https://twitter.com/PetrBenes/status/1310642455352672257?s=20
 * So, I assume two extra cr3 for each process
 */
#define MAX_CR3_IN_A_PROCESS 4

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
    CR3_TYPE   InterceptedCr3[MAX_CR3_IN_A_PROCESS];
    LIST_ENTRY ThreadsListHead;

} USERMODE_DEBUGGING_PROCESS_DETAILS, *PUSERMODE_DEBUGGING_PROCESS_DETAILS;

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
AttachingInitialize();

BOOLEAN
AttachingCheckPageFaultsWithUserDebugger(UINT32                       CurrentProcessorIndex,
                                         PGUEST_REGS                  GuestRegs,
                                         VMEXIT_INTERRUPT_INFORMATION InterruptExit,
                                         UINT64                       Address,
                                         ULONG                        ErrorCode);

BOOLEAN
AttachingConfigureInterceptingThreads(UINT64 ProcessDebuggingToken, BOOLEAN Enable);

BOOLEAN
AttachingHandleCr3VmexitsForThreadInterception(UINT32 CurrentCoreIndex, CR3_TYPE NewCr3);

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

BOOLEAN
AttachingQueryDetailsOfActiveDebuggingThreadsAndProcesses(PVOID BufferToStoreDetails, UINT32 BufferSize);
