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

//////////////////////////////////////////////////
//					Enums						//
//////////////////////////////////////////////////

/**
 * @brief Segment selector registers in x86
 *
 */
typedef enum _SEGMENT_REGISTERS {
    ES = 0,
    CS,
    SS,
    DS,
    FS,
    GS,
    LDTR,
    TR
} SEGMENT_REGISTERS;

//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

/*
 * @brief Windows IRQ Levels
 */
#define PASSIVE_LEVEL 0 // Passive release level
#define LOW_LEVEL 0 // Lowest interrupt level
#define APC_LEVEL 1 // APC interrupt level
#define DISPATCH_LEVEL 2 // Dispatcher level
#define CMCI_LEVEL 5 // CMCI handler level
#define CLOCK_LEVEL 13 // Interval clock level
#define IPI_LEVEL 14 // Interprocessor interrupt level
#define DRS_LEVEL 14 // Deferred Recovery Service level
#define POWER_LEVEL 14 // Power failure level
#define PROFILE_LEVEL 15 // timer used for profiling.
#define HIGH_LEVEL 15 // Highest interrupt level

/**
 * @brief Intel CPU flags in CR0
 */
#define X86_CR0_PE 0x00000001 /* Enable Protected Mode    (RW) */
#define X86_CR0_MP 0x00000002 /* Monitor Coprocessor      (RW) */
#define X86_CR0_EM 0x00000004 /* Require FPU Emulation    (RO) */
#define X86_CR0_TS 0x00000008 /* Task Switched            (RW) */
#define X86_CR0_ET 0x00000010 /* Extension type           (RO) */
#define X86_CR0_NE 0x00000020 /* Numeric Error Reporting  (RW) */
#define X86_CR0_WP 0x00010000 /* Supervisor Write Protect (RW) */
#define X86_CR0_AM 0x00040000 /* Alignment Checking       (RW) */
#define X86_CR0_NW 0x20000000 /* Not Write-Through        (RW) */
#define X86_CR0_CD 0x40000000 /* Cache Disable            (RW) */
#define X86_CR0_PG 0x80000000 /* Paging                         */

/**
 * @brief Intel CPU features in CR4
 *
 */
#define X86_CR4_VME 0x0001 /* enable vm86 extensions */
#define X86_CR4_PVI 0x0002 /* virtual interrupts flag enable */
#define X86_CR4_TSD 0x0004 /* disable time stamp at ipl 3 */
#define X86_CR4_DE 0x0008 /* enable debugging extensions */
#define X86_CR4_PSE 0x0010 /* enable page size extensions */
#define X86_CR4_PAE 0x0020 /* enable physical address extensions */
#define X86_CR4_MCE 0x0040 /* Machine check enable */
#define X86_CR4_PGE 0x0080 /* enable global pages */
#define X86_CR4_PCE 0x0100 /* enable performance counters at ipl 3 */
#define X86_CR4_OSFXSR 0x0200 /* enable fast FPU save and restore */
#define X86_CR4_OSXMMEXCPT 0x0400 /* enable unmasked SSE exceptions */
#define X86_CR4_VMXE 0x2000 /* enable VMX */

/**
 * @brief EFLAGS/RFLAGS
 *
 */
#define X86_FLAGS_CF (1 << 0)
#define X86_FLAGS_PF (1 << 2)
#define X86_FLAGS_AF (1 << 4)
#define X86_FLAGS_ZF (1 << 6)
#define X86_FLAGS_SF (1 << 7)
#define X86_FLAGS_TF (1 << 8)
#define X86_FLAGS_IF (1 << 9)
#define X86_FLAGS_DF (1 << 10)
#define X86_FLAGS_OF (1 << 11)
#define X86_FLAGS_STATUS_MASK (0xfff)
#define X86_FLAGS_IOPL_MASK (3 << 12)
#define X86_FLAGS_IOPL_SHIFT (12)
#define X86_FLAGS_IOPL_SHIFT_2ND_BIT (13)
#define X86_FLAGS_NT (1 << 14)
#define X86_FLAGS_RF (1 << 16)
#define X86_FLAGS_VM (1 << 17)
#define X86_FLAGS_AC (1 << 18)
#define X86_FLAGS_VIF (1 << 19)
#define X86_FLAGS_VIP (1 << 20)
#define X86_FLAGS_ID (1 << 21)
#define X86_FLAGS_RESERVED_ONES 0x2
#define X86_FLAGS_RESERVED 0xffc0802a

#define X86_FLAGS_RESERVED_BITS 0xffc38028
#define X86_FLAGS_FIXED 0x00000002

/**
 * @brief PCID Flags
 *
 */
#define PCID_NONE 0x000
#define PCID_MASK 0x003

/**
 * @brief The Microsoft Hypervisor interface defined constants
 *
 */
#define CPUID_HV_VENDOR_AND_MAX_FUNCTIONS 0x40000000
#define CPUID_HV_INTERFACE 0x40000001

/**
 * @brief Cpuid to get virtual address width
 *
 */
#define CPUID_ADDR_WIDTH 0x80000008

/**
 * @brief CPUID Features
 *
 */
#define CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS 0x00000001

/**
 * @brief Hypervisor reserved range for RDMSR and WRMSR
 *
 */
#define RESERVED_MSR_RANGE_LOW 0x40000000
#define RESERVED_MSR_RANGE_HI 0x400000F0

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
#define DPL_USER 3
#define DPL_SYSTEM 0

/**
 * @brief RPL Mask
 *
 */
#define RPL_MASK 3

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define ORDER_LONG (sizeof(unsigned long) == 4 ? 5 : 6)

#define BITMAP_ENTRY(_nr, _bmap) ((_bmap))[(_nr) / BITS_PER_LONG]
#define BITMAP_SHIFT(_nr) ((_nr) % BITS_PER_LONG)

/**
 * @brief Offset from a page's 4096 bytes
 *
 */
#define PAGE_OFFSET(Va) ((PVOID)((ULONG_PTR)(Va) & (PAGE_SIZE - 1)))

/**
 * @brief Intel TSX Constants
 *
 */
#define _XBEGIN_STARTED (~0u)
#define _XABORT_EXPLICIT (1 << 0)
#define _XABORT_RETRY (1 << 1)
#define _XABORT_CONFLICT (1 << 2)
#define _XABORT_CAPACITY (1 << 3)
#define _XABORT_DEBUG (1 << 4)
#define _XABORT_NESTED (1 << 5)
#define _XABORT_CODE(x) (((x) >> 24) & 0xFF)

//////////////////////////////////////////////////
//					 Structures					//
//////////////////////////////////////////////////

typedef SEGMENT_DESCRIPTOR_32* PSEGMENT_DESCRIPTOR;

/**
 * @brief CPUID Registers
 *
 */
typedef struct _CPUID {
    int eax;
    int ebx;
    int ecx;
    int edx;
} CPUID, *PCPUID;

/**
 * @brief Page-Fault Error Code
 *
 */
typedef union _PAGE_FAULT_ERROR_CODE {
    UINT32 Flags;
    struct
    {
        UINT32 Present : 1; // 0 = NotPresent
        UINT32 Write : 1; // 0 = Read
        UINT32 User : 1; // 0 = CPL==0
        UINT32 Reserved : 1; //
        UINT32 Fetch : 1; //
    } Fields;
} PAGE_FAULT_ERROR_CODE, *PPAGE_FAULT_ERROR_CODE;

typedef union _CR_FIXED {
    UINT64 Flags;

    struct
    {
        unsigned long Low;
        long High;

    } Fields;

} CR_FIXED, *PCR_FIXED;

//////////////////////////////////////////////////
//         Windows-specific structures          //
//////////////////////////////////////////////////

/**
 * @brief KPROCESS Brief structure
 *
 */
typedef struct _NT_KPROCESS {
    DISPATCHER_HEADER Header;
    LIST_ENTRY ProfileListHead;
    ULONG_PTR DirectoryTableBase;
    UCHAR Data[1];
} NT_KPROCESS, *PNT_KPROCESS;

//////////////////////////////////////////////////
//				 Function Types					//
//////////////////////////////////////////////////

/**
 * @brief Prototype to run a function on a logical core
 *
 */
typedef void (*RunOnLogicalCoreFunc)(ULONG ProcessorID);

//////////////////////////////////////////////////
//				External Functions				//
//////////////////////////////////////////////////

UCHAR*
PsGetProcessImageFileName(IN PEPROCESS Process);

//////////////////////////////////////////////////
//			 Function Definitions				//
//////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// Private Interfaces
//

static NTSTATUS
GetHandleFromProcess(_In_ UINT32 ProcessId, _Out_ PHANDLE Handle);

// ----------------------------------------------------------------------------
// Public Interfaces
//

int TestBit(int nth, unsigned long* addr);

void ClearBit(int nth, unsigned long* addr);

void SetBit(int nth, unsigned long* addr);

BOOLEAN
BroadcastToProcessors(_In_ ULONG ProcessorNumber, _In_ RunOnLogicalCoreFunc Routine);

BOOLEAN
StartsWith(const char* pre, const char* str);

VOID GetCpuid(UINT32 Func, UINT32 SubFunc, int* CpuInfo);

UINT64*
AllocateInvalidMsrBimap();

BOOLEAN
CheckCanonicalVirtualAddress(UINT64 VAddr, PBOOLEAN IsKernelAddress);

PCHAR
GetProcessNameFromEprocess(PEPROCESS eprocess);

UINT64
FindSystemDirectoryTableBase();

CR3_TYPE
GetCr3FromProcessId(_In_ UINT32 ProcessId);

//////////////////////////////////////////////////
//			         Functions  				//
//////////////////////////////////////////////////

#define MAX_EXEC_TRAMPOLINE_SIZE 100

VOID SyscallHookTest();

VOID SyscallHookConfigureEFER(VIRTUAL_MACHINE_STATE* VCpu, BOOLEAN EnableEFERSyscallHook);

BOOLEAN
SyscallHookHandleUD(_Inout_ VIRTUAL_MACHINE_STATE* VCpu);

BOOLEAN
SyscallHookEmulateSYSRET(_Inout_ VIRTUAL_MACHINE_STATE* VCpu);

BOOLEAN
SyscallHookEmulateSYSCALL(_Inout_ VIRTUAL_MACHINE_STATE* VCpu);

_Success_(return)
    BOOLEAN
    GetSegmentDescriptor(_In_ PUCHAR GdtBase, _In_ UINT16 Selector, _Out_ PVMX_SEGMENT_SELECTOR SegmentSelector);

UINT32
VmxCompatibleStrlen(const CHAR* S);

UINT32
VmxCompatibleWcslen(const wchar_t* S);
