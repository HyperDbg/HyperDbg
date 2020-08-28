/**
 * @file Transparency.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief hide the debugger from anti-debugging and anti-hypervisor methods (headers)
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
TransparentModeStart(PGUEST_REGS GuestRegs, ULONG ProcessorIndex, UINT32 ExitReason);

NTSTATUS
TransparentHideDebugger(PDEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE Measurements);

NTSTATUS
TransparentUnhideDebugger();

//////////////////////////////////////////////////
//				   Definitions					//
//////////////////////////////////////////////////

/**
 * @brief IA32_TIME_STAMP_COUNTER MSR (rcx)
 * 
 */
#define MSR_IA32_TIME_STAMP_COUNTER 0x10

/**
 * @brief Maximum value that can be returned by the rand function
 * 
 */
#define RAND_MAX 0x7fff

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief The status of transparency of each core after and before VMX
 * 
 */
typedef struct _VM_EXIT_TRANSPARENCY
{
    UINT64 PreviousTimeStampCounter;

    HANDLE  ThreadId;
    UINT64  RevealedTimeStampCounterByRdtsc;
    BOOLEAN CpuidAfterRdtscDetected;

} VM_EXIT_TRANSPARENCY, *PVM_EXIT_TRANSPARENCY;

/**
 * @brief The measurments from user-mode and kernel-mode
 * 
 */
typedef struct _TRANSPARENCY_MEASUREMENTS
{
    UINT64 CpuidAverage;
    UINT64 CpuidStandardDeviation;
    UINT64 CpuidMedian;

    UINT64 RdtscAverage;
    UINT64 RdtscStandardDeviation;
    UINT64 RdtscMedian;

    LIST_ENTRY ProcessList;

} TRANSPARENCY_MEASUREMENTS, *PTRANSPARENCY_MEASUREMENTS;

/**
 * @brief The ProcessList of TRANSPARENCY_MEASUREMENTS is from this architecture
 * 
 */
typedef struct _TRANSPARENCY_PROCESS
{
    UINT32     ProcessId;
    PVOID      ProcessName;
    PVOID      BufferAddress;
    BOOLEAN    TrueIfProcessIdAndFalseIfProcessName;
    LIST_ENTRY OtherProcesses;

} TRANSPARENCY_PROCESS, *PTRANSPARENCY_PROCESS;
