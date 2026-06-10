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
 VOID
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
 VOID
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
 UINT64
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
 * @brief Read Time-Stamp Counter (serializing)
 *
 * @param Aux processor ID output (may be NULL)
 * @return UINT64
 */
UINT64
CpuReadTscp(UINT32 * Aux)
{
#if defined(_WIN32) || defined(_WIN64)
    return __rdtscp(Aux);
#elif defined(__linux__)
    UINT32 __lo, __hi, __aux;
    __asm__ __volatile__("rdtscp" : "=a"(__lo), "=d"(__hi), "=c"(__aux));
    if (Aux)
        *Aux = __aux;
    return ((UINT64)__hi << 32) | __lo;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Execute PAUSE (spin-wait hint)
 */
VOID
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

//////////////////////////////////////////////////
//          Interlocked (Atomic) Operations     //
//////////////////////////////////////////////////

INT64
CpuInterlockedExchange64(INT64 volatile * Target, INT64 Value)
{
#if defined(_WIN32) || defined(_WIN64)
    return InterlockedExchange64(Target, Value);
#elif defined(__linux__)
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
#else
#    error "Unsupported platform"
#endif
}

INT64
CpuInterlockedExchangeAdd64(INT64 volatile * Addend, INT64 Value)
{
#if defined(_WIN32) || defined(_WIN64)
    return InterlockedExchangeAdd64(Addend, Value);
#elif defined(__linux__)
    return __atomic_fetch_add(Addend, Value, __ATOMIC_SEQ_CST);
#else
#    error "Unsupported platform"
#endif
}

INT64
CpuInterlockedIncrement64(INT64 volatile * Addend)
{
#if defined(_WIN32) || defined(_WIN64)
    return InterlockedIncrement64(Addend);
#elif defined(__linux__)
    return __atomic_add_fetch(Addend, 1LL, __ATOMIC_SEQ_CST);
#else
#    error "Unsupported platform"
#endif
}

INT64
CpuInterlockedDecrement64(INT64 volatile * Addend)
{
#if defined(_WIN32) || defined(_WIN64)
    return InterlockedDecrement64(Addend);
#elif defined(__linux__)
    return __atomic_sub_fetch(Addend, 1LL, __ATOMIC_SEQ_CST);
#else
#    error "Unsupported platform"
#endif
}

INT64
CpuInterlockedCompareExchange64(INT64 volatile * Destination, INT64 ExChange, INT64 Comparand)
{
#if defined(_WIN32) || defined(_WIN64)
    return InterlockedCompareExchange64(Destination, ExChange, Comparand);
#elif defined(__linux__)
    INT64 Expected = Comparand;
    __atomic_compare_exchange_n(Destination, &Expected, ExChange, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return Expected;
#else
#    error "Unsupported platform"
#endif
}

UCHAR
CpuInterlockedBitTestAndSet(volatile LONG * Base, LONG Bit)
{
#if defined(_WIN32) || defined(_WIN64)
    return _interlockedbittestandset(Base, Bit);
#elif defined(__linux__)
    LONG Mask = (1L << Bit);
    LONG Old  = __atomic_fetch_or(Base, Mask, __ATOMIC_SEQ_CST);
    return (UCHAR)((Old >> Bit) & 1);
#else
#    error "Unsupported platform"
#endif
}
