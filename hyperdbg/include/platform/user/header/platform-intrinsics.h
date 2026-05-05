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
extern inline VOID
CpuCpuId(INT32 * CpuInfo, INT32 FunctionId);

//
// CPUID (with sub-leaf)
//
extern inline VOID
CpuCpuIdEx(INT32 * CpuInfo, INT32 FunctionId, INT32 SubFunctionId);

//////////////////////////////////////////////////
//               TSC Instructions               //
//////////////////////////////////////////////////

//
// RDTSC
//
extern inline UINT64
    CpuReadTsc(VOID);

//////////////////////////////////////////////////
//              Misc Instructions               //
//////////////////////////////////////////////////

//
// PAUSE
//
extern inline VOID
    CpuPause(VOID);
