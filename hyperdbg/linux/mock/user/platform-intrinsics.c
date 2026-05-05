/**
 * @file platform-intrinsics.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for intrinsic functions (x86 instructions)
 * @details
 * @version 0.19
 * @date 2026-05-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/platform-intrinsics.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//               CPUID Instructions             //
//////////////////////////////////////////////////

/**
 * @brief Execute CPUID
 *
 * @param CpuInfo
 * @param FunctionId
 */
inline VOID
CpuCpuId(INT32 * CpuInfo, INT32 FunctionId)
{
#if defined(_WIN32) || defined(_WIN64)
    __cpuid(CpuInfo, FunctionId);
#elif defined(__linux__)
    __asm__ __volatile__("cpuid" : "=a"(CpuInfo[0]), "=b"(CpuInfo[1]), "=c"(CpuInfo[2]), "=d"(CpuInfo[3]) : "a"(FunctionId), "c"(0));
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Execute CPUID with sub-leaf
 *
 * @param CpuInfo
 * @param FunctionId
 * @param SubFunctionId
 */
inline VOID
CpuCpuIdEx(INT32 * CpuInfo, INT32 FunctionId, INT32 SubFunctionId)
{
#if defined(_WIN32) || defined(_WIN64)
    __cpuidex(CpuInfo, FunctionId, SubFunctionId);
#elif defined(__linux__)
    __asm__ __volatile__("cpuid" : "=a"(CpuInfo[0]), "=b"(CpuInfo[1]), "=c"(CpuInfo[2]), "=d"(CpuInfo[3]) : "a"(FunctionId), "c"(SubFunctionId));
#else
#    error "Unsupported platform"
#endif
}

//////////////////////////////////////////////////
//               TSC Instructions               //
//////////////////////////////////////////////////

/**
 * @brief Read Time-Stamp Counter
 *
 * @return UINT64
 */
inline UINT64
CpuReadTsc(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    return __rdtsc();
#elif defined(__linux__)
    UINT32 __lo, __hi;
    __asm__ __volatile__("rdtsc" : "=a"(__lo), "=d"(__hi));
    return ((UINT64)__hi << 32) | __lo;
#else
#    error "Unsupported platform"
#endif
}

//////////////////////////////////////////////////
//              Misc Instructions               //
//////////////////////////////////////////////////

/**
 * @brief Execute PAUSE (spin-wait hint)
 */
inline VOID
CpuPause(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    _mm_pause();
#elif defined(__linux__)
    __asm__ __volatile__("pause");
#else
#    error "Unsupported platform"
#endif
}
