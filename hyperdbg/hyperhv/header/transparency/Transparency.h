/**
 * @file Transparency.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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
TransparentModeStart(VIRTUAL_MACHINE_STATE * VCpu, UINT32 ExitReason);

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
 * @brief The measurements from user-mode and kernel-mode
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
