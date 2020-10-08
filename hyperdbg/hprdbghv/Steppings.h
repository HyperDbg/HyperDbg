/**
 * @file Steppings.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Headers of Debugger Steppings Mechanisms
 * @details Used in debugger
 * 
 * @version 0.1
 * @date 2020-08-30
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

/**
 * @brief Pointer to buffers containing the stepping's nop sled
 * 
 */
typedef struct _DEBUGGER_STEPPINGS_NOP_SLED
{
    BOOLEAN          IsNopSledInitialized;
    UINT64           NopSledVirtualAddress;
    PHYSICAL_ADDRESS NopSledPhysicalAddress;

} DEBUGGER_STEPPINGS_NOP_SLED, *PDEBUGGER_STEPPINGS_NOP_SLED;

/**
 * @brief Requests pool for performing action on debugging threads 
 * 
 */
typedef struct _DEBUGGER_STEPPINGS_POOL_REQUESTS
{
    DEBUGGER_STEPPINGS_ACTIONS_ENUM ACTION;

} DEBUGGER_STEPPINGS_POOL_REQUESTS, *PDEBUGGER_STEPPINGS_POOL_REQUESTS;

/**
 * @brief Structure to save the states of the thread that are 
 * a currently debugging and stepping
 * 
 */
typedef struct _DEBUGGER_STEPPING_THREAD_DETAILS
{
    BOOLEAN                          Enabled;
    LIST_ENTRY                       DebuggingThreadsList;
    UINT64                           BufferAddressToFree;
    UINT32                           ProcessId;
    UINT32                           ThreadId;
    PETHREAD                         ThreadStructure;
    CR3_TYPE                         ThreadKernelCr3;
    CR3_TYPE                         ThreadUserCr3;
    UINT64                           ThreadRip;
    GUEST_REGS                       ThreadRegisters;
    PEPT_PML1_ENTRY                  TargetEntryOnSecondaryPageTable;
    EPT_PML1_ENTRY                   OriginalEntryContent;
    EPT_PML1_ENTRY                   NopedEntryContent;
    DEBUGGER_STEPPINGS_POOL_REQUESTS SteppingAction;

} DEBUGGER_STEPPING_THREAD_DETAILS, *PDEBUGGER_STEPPING_THREAD_DETAILS;

//////////////////////////////////////////////////
//				   	Enums 		     			//
//////////////////////////////////////////////////

typedef enum _DEBUG_REGISTER_TYPE
{
    BREAK_ON_INSTRUCTION_FETCH,
    BREAK_ON_WRITE_ONLY,
    BREAK_ON_IO_READ_OR_WRITE_NOT_SUPPORTED,
    BREAK_ON_READ_AND_WRITE_BUT_NOT_FETCH
} DEBUG_REGISTER_TYPE;

//////////////////////////////////////////////////
//					Variables					//
//////////////////////////////////////////////////

/**
 * @brief Lock for finding process and thread on
 * external-interrupt exiting
 * 
 */
volatile LONG ExternalInterruptFindProcessAndThreadId;

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
SteppingsInitialize();

VOID
SteppingsUninitialize();

VOID
SteppingsHandleClockInterruptOnTargetProcess(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex, PVMEXIT_INTERRUPT_INFO InterruptExit);

VOID
SteppingsStartDebuggingThread(UINT32 ProcessId, UINT32 ThreadId);

VOID
SteppingsHandleTargetThreadForTheFirstTime(PGUEST_REGS GuestRegs, PDEBUGGER_STEPPING_THREAD_DETAILS ThreadDetailsBuffer, UINT32 ProcessorIndex, CR3_TYPE KernelCr3, UINT32 ProcessId, UINT32 ThreadId);

BOOLEAN
SteppingsSwapPageWithInfiniteLoop(PVOID TargetAddress, CR3_TYPE ProcessCr3, UINT32 LogicalCoreIndex, PDEBUGGER_STEPPING_THREAD_DETAILS ThreadDetailsBuffer);

VOID
SteppingsHandleCr3Vmexits(CR3_TYPE NewCr3, UINT32 ProcessorIndex);

VOID
SteppingsHandlesDebuggedThread(PDEBUGGER_STEPPING_THREAD_DETAILS ThreadSteppingDetail, UINT32 ProcessorIndex);

BOOLEAN
SteppingsSetDebugRegister(UINT32 DebugRegNum, DEBUG_REGISTER_TYPE ActionType, BOOLEAN ApplyToVmcs, UINT64 TargetAddress);

VOID
SteppingsAttachOrDetachToThread(PDEBUGGER_ATTACH_DETACH_USER_MODE_PROCESS AttachOrDetachRequest);

NTSTATUS
SteppingsPerformAction(PDEBUGGER_STEPPINGS DebuggerSteppingRequest);

VOID
SteppingsDpcEnableOrDisableThreadChangeMonitorOnCurrentCore(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
SteppingsEnableOrDisableThreadChangeMonitorOnAllCores(BOOLEAN Enable);

VOID
SteppingsHandleThreadChanges(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex);
