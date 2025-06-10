/**
 * @file Common.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header files for common functions
 * @details
 * @version 0.1
 * @date 2020-04-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

/**
 * @brief Core Id
 *
 */
#define __CPU_INDEX__ KeGetCurrentProcessorNumberEx(NULL)

/**
 * @brief Alignment Size
 *
 */
#define ALIGNMENT_PAGE_SIZE 4096

/**
 * @brief Maximum x64 Address
 *
 */
#define MAXIMUM_ADDRESS 0xffffffffffffffff

/**
 * @brief System and User ring definitions
 *
 */
#define DPL_USER   3
#define DPL_SYSTEM 0

/**
 * @brief RPL Mask
 *
 */
#define RPL_MASK 3

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define ORDER_LONG    (sizeof(unsigned long) == 4 ? 5 : 6)

#define BITMAP_ENTRY(_nr, _bmap) ((_bmap))[(_nr) / BITS_PER_LONG]
#define BITMAP_SHIFT(_nr)        ((_nr) % BITS_PER_LONG)

/**
 * @brief Offset from a page's 4096 bytes
 *
 */
#define PAGE_OFFSET(Va) ((PVOID)((ULONG_PTR)(Va) & (PAGE_SIZE - 1)))

/**
 * @brief Intel TSX Constants
 *
 */
#define _XBEGIN_STARTED  (~0u)
#define _XABORT_EXPLICIT (1 << 0)
#define _XABORT_RETRY    (1 << 1)
#define _XABORT_CONFLICT (1 << 2)
#define _XABORT_CAPACITY (1 << 3)
#define _XABORT_DEBUG    (1 << 4)
#define _XABORT_NESTED   (1 << 5)

#ifndef _XABORT_CODE
#    define _XABORT_CODE(x) (((x) >> 24) & 0xFF)
#endif // !_XABORT_CODE

//////////////////////////////////////////////////
//					 Structures					//
//////////////////////////////////////////////////

typedef SEGMENT_DESCRIPTOR_32 * PSEGMENT_DESCRIPTOR;

/**
 * @brief CPUID Registers
 *
 */
typedef struct _CPUID
{
    int eax;
    int ebx;
    int ecx;
    int edx;
} CPUID, *PCPUID;

typedef union _CR_FIXED
{
    UINT64 Flags;

    struct
    {
        unsigned long Low;
        long          High;

    } Fields;

} CR_FIXED, *PCR_FIXED;

//////////////////////////////////////////////////
//         Windows-specific structures          //
//////////////////////////////////////////////////

/**
 * @brief KPROCESS Brief structure
 *
 */
typedef struct _NT_KPROCESS
{
    DISPATCHER_HEADER Header;
    LIST_ENTRY        ProfileListHead;
    ULONG_PTR         DirectoryTableBase;
    UCHAR             Data[1];
} NT_KPROCESS, *PNT_KPROCESS;

//////////////////////////////////////////////////
//				 Function Types					//
//////////////////////////////////////////////////

/**
 * @brief Prototype to run a function on a logical core
 *
 */
typedef void (*RunOnLogicalCoreFunc)(ULONG ProcessorId);

//////////////////////////////////////////////////
//				External Functions				//
//////////////////////////////////////////////////

UCHAR *
PsGetProcessImageFileName(IN PEPROCESS Process);

//////////////////////////////////////////////////
//			 Function Definitions				//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// Private Interfaces
//

static NTSTATUS
CommonGetHandleFromProcess(_In_ UINT32 ProcessId, _Out_ PHANDLE Handle);

// ----------------------------------------------------------------------------
// Public Interfaces
//

BOOLEAN
CommonAffinityBroadcastToProcessors(_In_ ULONG ProcessorNumber, _In_ RunOnLogicalCoreFunc Routine);

BOOLEAN
CommonIsStringStartsWith(const char * pre, const char * str);

BOOLEAN
CommonIsGuestOnUsermode32Bit();

PCHAR
CommonGetProcessNameFromProcessControlBlock(PEPROCESS eprocess);

VOID
CommonCpuidInstruction(UINT32 Func, UINT32 SubFunc, int * CpuInfo);

VOID
CommonWriteDebugInformation(VIRTUAL_MACHINE_STATE * VCpu);

BOOLEAN
CommonIsXCr0Valid(XCR0 XCr0);
