/**
 * @file PlatformIntrinsics.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for intrinsic functions (x86 instructions)
 * @details
 * @version 0.19
 * @date 2026-05-05
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//             CR Registers                     //
//////////////////////////////////////////////////

//
// READCR0
//
extern inline ULONG_PTR
    CpuReadCr0(VOID);

//
// WRITECR0
//
extern inline VOID
CpuWriteCr0(ULONG_PTR Cr0Value);

//
// READCR2
//
extern inline ULONG_PTR
    CpuReadCr2(VOID);

//
// WRITECR2
//
extern inline VOID
CpuWriteCr2(ULONG_PTR Cr2Value);

//
// READCR3
//
extern inline ULONG_PTR
    CpuReadCr3(VOID);

//
// WRITECR3
//
extern inline VOID
CpuWriteCr3(ULONG_PTR Cr3Value);

//
// READCR4
//
extern inline ULONG_PTR
    CpuReadCr4(VOID);

//
// WRITECR4
//
extern inline VOID
CpuWriteCr4(ULONG_PTR Cr4Value);

//
// READCR8
//
extern inline ULONG_PTR
    CpuReadCr8(VOID);

//
// WRITECR8
//
extern inline VOID
CpuWriteCr8(ULONG_PTR Cr8Value);

//////////////////////////////////////////////////
//             MSR Instructions                 //
//////////////////////////////////////////////////

//
// RDMSR
//
extern inline UINT64
CpuReadMsr(ULONG MsrAddress);

//
// WRMSR
//
extern inline VOID
CpuWriteMsr(ULONG MsrAddress, UINT64 MsrValue);

//////////////////////////////////////////////////
//           Debug Register Instructions        //
//////////////////////////////////////////////////

//
// MOV DR (read)
//
#if defined(_WIN32) || defined(_WIN64)
#    define CpuReadDr(DrNumber) __readdr(DrNumber)
#elif defined(__linux__)
#    define CpuReadDr(DrNumber)                                  \
        ({                                                       \
            ULONG_PTR __val;                                     \
            switch (DrNumber)                                    \
            {                                                    \
            case 0:                                              \
                __asm__ volatile("mov %%dr0, %0" : "=r"(__val)); \
                break;                                           \
            case 1:                                              \
                __asm__ volatile("mov %%dr1, %0" : "=r"(__val)); \
                break;                                           \
            case 2:                                              \
                __asm__ volatile("mov %%dr2, %0" : "=r"(__val)); \
                break;                                           \
            case 3:                                              \
                __asm__ volatile("mov %%dr3, %0" : "=r"(__val)); \
                break;                                           \
            case 6:                                              \
                __asm__ volatile("mov %%dr6, %0" : "=r"(__val)); \
                break;                                           \
            case 7:                                              \
                __asm__ volatile("mov %%dr7, %0" : "=r"(__val)); \
                break;                                           \
            default:                                             \
                __val = 0;                                       \
                break;                                           \
            }                                                    \
            __val;                                               \
        })
#else
#    error "Unsupported platform"
#endif

//
// MOV DR (write)
//
#if defined(_WIN32) || defined(_WIN64)
#    define CpuWriteDr(DrNumber, DrValue) __writedr(DrNumber, DrValue)
#elif defined(__linux__)
#    define CpuWriteDr(DrNumber, DrValue)                                        \
        do                                                                       \
        {                                                                        \
            switch (DrNumber)                                                    \
            {                                                                    \
            case 0:                                                              \
                __asm__ volatile("mov %0, %%dr0" : : "r"((ULONG_PTR)(DrValue))); \
                break;                                                           \
            case 1:                                                              \
                __asm__ volatile("mov %0, %%dr1" : : "r"((ULONG_PTR)(DrValue))); \
                break;                                                           \
            case 2:                                                              \
                __asm__ volatile("mov %0, %%dr2" : : "r"((ULONG_PTR)(DrValue))); \
                break;                                                           \
            case 3:                                                              \
                __asm__ volatile("mov %0, %%dr3" : : "r"((ULONG_PTR)(DrValue))); \
                break;                                                           \
            case 6:                                                              \
                __asm__ volatile("mov %0, %%dr6" : : "r"((ULONG_PTR)(DrValue))); \
                break;                                                           \
            case 7:                                                              \
                __asm__ volatile("mov %0, %%dr7" : : "r"((ULONG_PTR)(DrValue))); \
                break;                                                           \
            default:                                                             \
                break;                                                           \
            }                                                                    \
        } while (0)
#else
#    error "Unsupported platform"
#endif

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

//
// RDTSCP
//
extern inline UINT64
CpuReadTscp(UINT32 * Aux);

//////////////////////////////////////////////////
//          Interlocked (Atomic) Operations     //
//////////////////////////////////////////////////

extern inline INT64
CpuInterlockedExchange64(INT64 volatile * Target, INT64 Value);

extern inline INT64
CpuInterlockedExchangeAdd64(INT64 volatile * Addend, INT64 Value);

extern inline INT64
CpuInterlockedIncrement64(INT64 volatile * Addend);

extern inline INT64
CpuInterlockedDecrement64(INT64 volatile * Addend);

extern inline INT64
CpuInterlockedCompareExchange64(INT64 volatile * Destination, INT64 ExChange, INT64 Comparand);

//////////////////////////////////////////////////
//           Descriptor Table Instructions      //
//////////////////////////////////////////////////

//
// SIDT
//
extern inline VOID
CpuSidt(VOID * Idtr);

//////////////////////////////////////////////////
//              TLB Instructions                //
//////////////////////////////////////////////////

//
// INVLPG
//
extern inline VOID
CpuInvlpg(VOID * Address);

//////////////////////////////////////////////////
//           String Store Instructions          //
//////////////////////////////////////////////////

//
// STOSQ
//
extern inline VOID
CpuStosQ(UINT64 * Destination, UINT64 Value, SIZE_T Count);

//////////////////////////////////////////////////
//              Bit Scan Instructions           //
//////////////////////////////////////////////////

//
// BSF 64
//
extern inline UCHAR
CpuBitScanForward64(ULONG * Index, UINT64 Mask);

//////////////////////////////////////////////////
//              Misc Instructions               //
//////////////////////////////////////////////////

//
// NOP
//
extern inline VOID
    CpuNop(VOID);

//
// PAUSE
//
extern inline VOID
    CpuPause(VOID);

//
// Segment Limit
//
extern inline ULONG
CpuSegmentLimit(UINT32 Selector);

//////////////////////////////////////////////////
//              I/O Port Instructions           //
//////////////////////////////////////////////////

//
// IN Byte
//
extern inline UINT8
CpuIoInByte(UINT16 Port);

//
// IN Word
//
extern inline UINT16
CpuIoInWord(UINT16 Port);

//
// IN Dword
//
extern inline UINT32
CpuIoInDword(UINT16 Port);

//
// IN Byte String
//
extern inline VOID
CpuIoInByteString(UINT16 Port, UINT8 * Data, UINT32 Size);

//
// IN Word String
//
extern inline VOID
CpuIoInWordString(UINT16 Port, UINT16 * Data, UINT32 Size);

//
// IN Dword String
//
extern inline VOID
CpuIoInDwordString(UINT16 Port, UINT32 * Data, UINT32 Size);

//
// OUT Byte
//
extern inline VOID
CpuIoOutByte(UINT16 Port, UINT8 Value);

//
// OUT Word
//
extern inline VOID
CpuIoOutWord(UINT16 Port, UINT16 Value);

//
// OUT Dword
//
extern inline VOID
CpuIoOutDword(UINT16 Port, UINT32 Value);

//
// OUT Byte String
//
extern inline VOID
CpuIoOutByteString(UINT16 Port, UINT8 * Data, UINT32 Count);

//
// OUT Word String
//
extern inline VOID
CpuIoOutWordString(UINT16 Port, UINT16 * Data, UINT32 Count);

//
// OUT Dword String
//
extern inline VOID
CpuIoOutDwordString(UINT16 Port, UINT32 * Data, UINT32 Count);
