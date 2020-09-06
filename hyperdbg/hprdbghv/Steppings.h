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
SteppingsHandleTargetThreadForTheFirstTime(PGUEST_REGS GuestRegs, UINT32 ProcessorIndex, CR3_TYPE KernelCr3, UINT32 ProcessId, UINT32 ThreadId);

BOOLEAN
SteppingsSwapPageWithInfiniteLoop(PVOID TargetAddress, CR3_TYPE ProcessCr3, UINT32 LogicalCoreIndex);
