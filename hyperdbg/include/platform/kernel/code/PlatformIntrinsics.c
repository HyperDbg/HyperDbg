/**
 * @file PlatformIntrinsics.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of cross platform APIs for intrinsic functions (x86 instructions)
 * @details
 * @version 0.19
 * @date 2026-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/PlatformIntrinsics.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//             CR Registers                     //
//////////////////////////////////////////////////

/**
 * @brief Read CR0
 *
 * @return ULONG_PTR
 */
inline ULONG_PTR
CpuReadCr0(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    return __readcr0();
#elif defined(__linux__)
    ULONG_PTR __val; __asm__ __volatile__("mov %%cr0, %0" : "=r"(__val)); return __val;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write CR0
 *
 * @param Cr0Value
 */
inline VOID
CpuWriteCr0(ULONG_PTR Cr0Value)
{
#if defined(_WIN32) || defined(_WIN64)
    __writecr0(Cr0Value);
#elif defined(__linux__)
    __asm__ __volatile__("mov %0, %%cr0" : : "r"(Cr0Value) : "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read CR2
 *
 * @return ULONG_PTR
 */
inline ULONG_PTR
CpuReadCr2(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    return __readcr2();
#elif defined(__linux__)
    ULONG_PTR __val; __asm__ __volatile__("mov %%cr2, %0" : "=r"(__val)); return __val;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write CR2
 *
 * @param Cr2Value
 */
inline VOID
CpuWriteCr2(ULONG_PTR Cr2Value)
{
#if defined(_WIN32) || defined(_WIN64)
    __writecr2(Cr2Value);
#elif defined(__linux__)
    __asm__ __volatile__("mov %0, %%cr2" : : "r"(Cr2Value) : "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read CR3
 *
 * @return ULONG_PTR
 */
inline ULONG_PTR
CpuReadCr3(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    return __readcr3();
#elif defined(__linux__)
    ULONG_PTR __val; __asm__ __volatile__("mov %%cr3, %0" : "=r"(__val)); return __val;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write CR3
 *
 * @param Cr3Value
 */
inline VOID
CpuWriteCr3(ULONG_PTR Cr3Value)
{
#if defined(_WIN32) || defined(_WIN64)
    __writecr3(Cr3Value);
#elif defined(__linux__)
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(Cr3Value) : "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read CR4
 *
 * @return ULONG_PTR
 */
inline ULONG_PTR
CpuReadCr4(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    return __readcr4();
#elif defined(__linux__)
    ULONG_PTR __val; __asm__ __volatile__("mov %%cr4, %0" : "=r"(__val)); return __val;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write CR4
 *
 * @param Cr4Value
 */
inline VOID
CpuWriteCr4(ULONG_PTR Cr4Value)
{
#if defined(_WIN32) || defined(_WIN64)
    __writecr4(Cr4Value);
#elif defined(__linux__)
    __asm__ __volatile__("mov %0, %%cr4" : : "r"(Cr4Value) : "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read CR8
 *
 * @return ULONG_PTR
 */
inline ULONG_PTR
CpuReadCr8(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    return __readcr8();
#elif defined(__linux__)
    ULONG_PTR __val; __asm__ __volatile__("mov %%cr8, %0" : "=r"(__val)); return __val;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write CR8
 *
 * @param Cr8Value
 */
inline VOID
CpuWriteCr8(ULONG_PTR Cr8Value)
{
#if defined(_WIN32) || defined(_WIN64)
    __writecr8(Cr8Value);
#elif defined(__linux__)
    __asm__ __volatile__("mov %0, %%cr8" : : "r"(Cr8Value) : "memory");
#else
#    error "Unsupported platform"
#endif
}

//////////////////////////////////////////////////
//             MSR Instructions                 //
//////////////////////////////////////////////////

/**
 * @brief Read an MSR
 *
 * @param MsrAddress
 * @return UINT64
 */
inline UINT64
CpuReadMsr(ULONG MsrAddress)
{
#if defined(_WIN32) || defined(_WIN64)
    return __readmsr(MsrAddress);
#elif defined(__linux__)
    UINT32 __lo, __hi; __asm__ __volatile__("rdmsr" : "=a"(__lo), "=d"(__hi) : "c"(MsrAddress)); return ((UINT64)__hi << 32) | __lo;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write an MSR
 *
 * @param MsrAddress
 * @param MsrValue
 */
inline VOID
CpuWriteMsr(ULONG MsrAddress, UINT64 MsrValue)
{
#if defined(_WIN32) || defined(_WIN64)
    __writemsr(MsrAddress, MsrValue);
#elif defined(__linux__)
    __asm__ __volatile__("wrmsr" : : "c"(MsrAddress), "a"((UINT32)MsrValue), "d"((UINT32)(MsrValue >> 32)));
#else
#    error "Unsupported platform"
#endif
}

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
    UINT32 __lo, __hi; __asm__ __volatile__("rdtsc" : "=a"(__lo), "=d"(__hi)); return ((UINT64)__hi << 32) | __lo;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read Time-Stamp Counter and Processor ID
 *
 * @param Aux
 * @return UINT64
 */
inline UINT64
CpuReadTscp(UINT32 * Aux)
{
#if defined(_WIN32) || defined(_WIN64)
    return __rdtscp(Aux);
#elif defined(__linux__)
    UINT32 __lo, __hi; __asm__ __volatile__("rdtscp" : "=a"(__lo), "=d"(__hi), "=c"(*Aux)); return ((UINT64)__hi << 32) | __lo;
#else
#    error "Unsupported platform"
#endif
}

//////////////////////////////////////////////////
//           Descriptor Table Instructions      //
//////////////////////////////////////////////////

/**
 * @brief Store Interrupt Descriptor Table Register
 *
 * @param Idtr
 */
inline VOID
CpuSidt(VOID * Idtr)
{
#if defined(_WIN32) || defined(_WIN64)
    __sidt(Idtr);
#elif defined(__linux__)
    __asm__ __volatile__("sidt %0" : "=m"(*(char *)Idtr) : : "memory");
#else
#    error "Unsupported platform"
#endif
}

//////////////////////////////////////////////////
//              TLB Instructions                //
//////////////////////////////////////////////////

/**
 * @brief Invalidate TLB entry for a virtual address
 *
 * @param Address
 */
inline VOID
CpuInvlpg(VOID * Address)
{
#if defined(_WIN32) || defined(_WIN64)
    __invlpg(Address);
#elif defined(__linux__)
    __asm__ __volatile__("invlpg (%0)" : : "r"(Address) : "memory");
#else
#    error "Unsupported platform"
#endif
}

//////////////////////////////////////////////////
//           String Store Instructions          //
//////////////////////////////////////////////////

/**
 * @brief Store UINT64 value to memory Count times
 *
 * @param Destination
 * @param Value
 * @param Count
 */
inline VOID
CpuStosQ(UINT64 * Destination, UINT64 Value, SIZE_T Count)
{
#if defined(_WIN32) || defined(_WIN64)
    __stosq((unsigned __int64 *)Destination, Value, Count);
#elif defined(__linux__)
    __asm__ __volatile__("rep stosq" : "+D"(Destination), "+c"(Count) : "a"(Value) : "memory");
#else
#    error "Unsupported platform"
#endif
}

//////////////////////////////////////////////////
//              Bit Scan Instructions           //
//////////////////////////////////////////////////

/**
 * @brief Bit scan forward (64-bit)
 *
 * @param Index
 * @param Mask
 * @return UCHAR
 */
inline UCHAR
CpuBitScanForward64(ULONG * Index, UINT64 Mask)
{
#if defined(_WIN32) || defined(_WIN64)
    return (UCHAR)_BitScanForward64((unsigned long *)Index, Mask);
#elif defined(__linux__)
    if (!Mask) return 0; *Index = (ULONG)__builtin_ctzll(Mask); return 1;
#else
#    error "Unsupported platform"
#endif
}

//////////////////////////////////////////////////
//              Misc Instructions               //
//////////////////////////////////////////////////

/**
 * @brief Execute NOP
 */
inline VOID
CpuNop(VOID)
{
#if defined(_WIN32) || defined(_WIN64)
    __nop();
#elif defined(__linux__)
    __asm__ __volatile__("nop");
#else
#    error "Unsupported platform"
#endif
}

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

/**
 * @brief Return the segment limit for a selector
 *
 * @param Selector
 * @return ULONG
 */
inline ULONG
CpuSegmentLimit(UINT32 Selector)
{
#if defined(_WIN32) || defined(_WIN64)
    return __segmentlimit(Selector);
#elif defined(__linux__)
    ULONG __limit; __asm__ __volatile__("lsl %1, %0" : "=r"(__limit) : "r"(Selector)); return __limit;
#else
#    error "Unsupported platform"
#endif
}

//////////////////////////////////////////////////
//              I/O Port Instructions           //
//////////////////////////////////////////////////

/**
 * @brief Read a byte from an I/O port
 *
 * @param Port
 * @return UINT8
 */
inline UINT8
CpuIoInByte(UINT16 Port)
{
#if defined(_WIN32) || defined(_WIN64)
    return __inbyte(Port);
#elif defined(__linux__)
    UINT8 __val; __asm__ __volatile__("inb %1, %0" : "=a"(__val) : "Nd"(Port)); return __val;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read a word from an I/O port
 *
 * @param Port
 * @return UINT16
 */
inline UINT16
CpuIoInWord(UINT16 Port)
{
#if defined(_WIN32) || defined(_WIN64)
    return __inword(Port);
#elif defined(__linux__)
    UINT16 __val; __asm__ __volatile__("inw %1, %0" : "=a"(__val) : "Nd"(Port)); return __val;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read a dword from an I/O port
 *
 * @param Port
 * @return UINT32
 */
inline UINT32
CpuIoInDword(UINT16 Port)
{
#if defined(_WIN32) || defined(_WIN64)
    return __indword(Port);
#elif defined(__linux__)
    UINT32 __val; __asm__ __volatile__("inl %1, %0" : "=a"(__val) : "Nd"(Port)); return __val;
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read a byte string from an I/O port
 *
 * @param Port
 * @param Data
 * @param Size
 */
inline VOID
CpuIoInByteString(UINT16 Port, UINT8 * Data, UINT32 Size)
{
#if defined(_WIN32) || defined(_WIN64)
    __inbytestring(Port, Data, Size);
#elif defined(__linux__)
    __asm__ __volatile__("rep insb" : "+D"(Data), "+c"(Size) : "d"(Port) : "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read a word string from an I/O port
 *
 * @param Port
 * @param Data
 * @param Size
 */
inline VOID
CpuIoInWordString(UINT16 Port, UINT16 * Data, UINT32 Size)
{
#if defined(_WIN32) || defined(_WIN64)
    __inwordstring(Port, Data, Size);
#elif defined(__linux__)
    __asm__ __volatile__("rep insw" : "+D"(Data), "+c"(Size) : "d"(Port) : "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Read a dword string from an I/O port
 *
 * @param Port
 * @param Data
 * @param Size
 */
inline VOID
CpuIoInDwordString(UINT16 Port, UINT32 * Data, UINT32 Size)
{
#if defined(_WIN32) || defined(_WIN64)
    __indwordstring(Port, (unsigned long *)Data, Size);
#elif defined(__linux__)
    __asm__ __volatile__("rep insl" : "+D"(Data), "+c"(Size) : "d"(Port) : "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write a byte to an I/O port
 *
 * @param Port
 * @param Value
 */
inline VOID
CpuIoOutByte(UINT16 Port, UINT8 Value)
{
#if defined(_WIN32) || defined(_WIN64)
    __outbyte(Port, Value);
#elif defined(__linux__)
    __asm__ __volatile__("outb %0, %1" : : "a"(Value), "Nd"(Port));
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write a word to an I/O port
 *
 * @param Port
 * @param Value
 */
inline VOID
CpuIoOutWord(UINT16 Port, UINT16 Value)
{
#if defined(_WIN32) || defined(_WIN64)
    __outword(Port, Value);
#elif defined(__linux__)
    __asm__ __volatile__("outw %0, %1" : : "a"(Value), "Nd"(Port));
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write a dword to an I/O port
 *
 * @param Port
 * @param Value
 */
inline VOID
CpuIoOutDword(UINT16 Port, UINT32 Value)
{
#if defined(_WIN32) || defined(_WIN64)
    __outdword(Port, Value);
#elif defined(__linux__)
    __asm__ __volatile__("outl %0, %1" : : "a"(Value), "Nd"(Port));
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write a byte string to an I/O port
 *
 * @param Port
 * @param Data
 * @param Count
 */
inline VOID
CpuIoOutByteString(UINT16 Port, UINT8 * Data, UINT32 Count)
{
#if defined(_WIN32) || defined(_WIN64)
    __outbytestring(Port, Data, Count);
#elif defined(__linux__)
    __asm__ __volatile__("rep outsb" : "+S"(Data), "+c"(Count) : "d"(Port) : "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write a word string to an I/O port
 *
 * @param Port
 * @param Data
 * @param Count
 */
inline VOID
CpuIoOutWordString(UINT16 Port, UINT16 * Data, UINT32 Count)
{
#if defined(_WIN32) || defined(_WIN64)
    __outwordstring(Port, Data, Count);
#elif defined(__linux__)
    __asm__ __volatile__("rep outsw" : "+S"(Data), "+c"(Count) : "d"(Port) : "memory");
#else
#    error "Unsupported platform"
#endif
}

/**
 * @brief Write a dword string to an I/O port
 *
 * @param Port
 * @param Data
 * @param Count
 */
inline VOID
CpuIoOutDwordString(UINT16 Port, UINT32 * Data, UINT32 Count)
{
#if defined(_WIN32) || defined(_WIN64)
    __outdwordstring(Port, (unsigned long *)Data, Count);
#elif defined(__linux__)
    __asm__ __volatile__("rep outsl" : "+S"(Data), "+c"(Count) : "d"(Port) : "memory");
#else
#    error "Unsupported platform"
#endif
}
