/**
 * @file platform-intrinsics.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief User mode Cross platform APIs for intrinsic functions (x86 instructions)
 * @details
 * @version 0.19
 * @date 2026-05-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//               CPUID Instructions             //
//////////////////////////////////////////////////

//
// CPUID
//
VOID
CpuCpuId(INT32 * CpuInfo, INT32 FunctionId);

//
// CPUID (with sub-leaf)
//
VOID
CpuCpuIdEx(INT32 * CpuInfo, INT32 FunctionId, INT32 SubFunctionId);

//////////////////////////////////////////////////
//               TSC Instructions               //
//////////////////////////////////////////////////

//
// RDTSC
//
UINT64
CpuReadTsc(VOID);

//
// RDTSCP
//
UINT64
CpuReadTscp(UINT32 * Aux);

//////////////////////////////////////////////////
//              Misc Instructions               //
//////////////////////////////////////////////////

//
// PAUSE
//
VOID
CpuPause(VOID);

//////////////////////////////////////////////////
//          Interlocked (Atomic) Operations     //
//////////////////////////////////////////////////

INT64
CpuInterlockedExchange64(INT64 volatile * Target, INT64 Value);

INT64
CpuInterlockedExchangeAdd64(INT64 volatile * Addend, INT64 Value);

INT64
CpuInterlockedIncrement64(INT64 volatile * Addend);

INT64
CpuInterlockedDecrement64(INT64 volatile * Addend);

INT64
CpuInterlockedCompareExchange64(INT64 volatile * Destination, INT64 ExChange, INT64 Comparand);

UCHAR
CpuInterlockedBitTestAndSet(volatile LONG * Base, LONG Bit);
