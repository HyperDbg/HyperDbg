/**
 * @file Common.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header files for common functions
 * @details
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#pragma once
#include "Ept.h"
#include "Configuration.h"
#include "Trace.h"

//////////////////////////////////////////////////
//					Enums						//
//////////////////////////////////////////////////

/**
 * @brief Segment selector registers in x86
 * 
 */
typedef enum _SEGMENT_REGISTERS
{
    ES = 0,
    CS,
    SS,
    DS,
    FS,
    GS,
    LDTR,
    TR
};

//////////////////////////////////////////////////
//				 Spinlock Funtions				//
//////////////////////////////////////////////////
inline BOOLEAN
SpinlockTryLock(volatile LONG * Lock);
inline void
SpinlockLock(volatile LONG * Lock);
inline void
SpinlockUnlock(volatile LONG * Lock);

//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

/* @brief Intel CPU flags in CR0 */
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

/* Intel CPU features in CR4 */
#define X86_CR4_VME        0x0001 /* enable vm86 extensions */
#define X86_CR4_PVI        0x0002 /* virtual interrupts flag enable */
#define X86_CR4_TSD        0x0004 /* disable time stamp at ipl 3 */
#define X86_CR4_DE         0x0008 /* enable debugging extensions */
#define X86_CR4_PSE        0x0010 /* enable page size extensions */
#define X86_CR4_PAE        0x0020 /* enable physical address extensions */
#define X86_CR4_MCE        0x0040 /* Machine check enable */
#define X86_CR4_PGE        0x0080 /* enable global pages */
#define X86_CR4_PCE        0x0100 /* enable performance counters at ipl 3 */
#define X86_CR4_OSFXSR     0x0200 /* enable fast FPU save and restore */
#define X86_CR4_OSXMMEXCPT 0x0400 /* enable unmasked SSE exceptions */
#define X86_CR4_VMXE       0x2000 /* enable VMX */

/* EFLAGS/RFLAGS */
#define X86_FLAGS_CF            (1 << 0)
#define X86_FLAGS_PF            (1 << 2)
#define X86_FLAGS_AF            (1 << 4)
#define X86_FLAGS_ZF            (1 << 6)
#define X86_FLAGS_SF            (1 << 7)
#define X86_FLAGS_TF            (1 << 8)
#define X86_FLAGS_IF            (1 << 9)
#define X86_FLAGS_DF            (1 << 10)
#define X86_FLAGS_OF            (1 << 11)
#define X86_FLAGS_STATUS_MASK   (0xfff)
#define X86_FLAGS_IOPL_MASK     (3 << 12)
#define X86_FLAGS_IOPL_SHIFT    (12)
#define X86_FLAGS_NT            (1 << 14)
#define X86_FLAGS_RF            (1 << 16)
#define X86_FLAGS_VM            (1 << 17)
#define X86_FLAGS_AC            (1 << 18)
#define X86_FLAGS_VIF           (1 << 19)
#define X86_FLAGS_VIP           (1 << 20)
#define X86_FLAGS_ID            (1 << 21)
#define X86_FLAGS_RESERVED_ONES 0x2
#define X86_FLAGS_RESERVED      0xffc0802a

#define X86_FLAGS_RESERVED_BITS 0xffc38028
#define X86_FLAGS_FIXED         0x00000002

/* PCID Flags */
#define PCID_NONE 0x000
#define PCID_MASK 0x003

/* The Microsoft Hypervisor interface defined constants. */
#define CPUID_HV_VENDOR_AND_MAX_FUNCTIONS 0x40000000
#define CPUID_HV_INTERFACE                0x40000001

/* CPUID Features */
#define CPUID_PROCESSOR_AND_PROCESSOR_FEATURE_IDENTIFIERS 0x00000001

/* Hypervisor reserved range for RDMSR and WRMSR */
#define RESERVED_MSR_RANGE_LOW 0x40000000
#define RESERVED_MSR_RANGE_HI  0x400000F0

/* Alignment Size */
#define __CPU_INDEX__ KeGetCurrentProcessorNumberEx(NULL)

/* Alignment Size */
#define ALIGNMENT_PAGE_SIZE 4096

/* Maximum x64 Address */
#define MAXIMUM_ADDRESS 0xffffffffffffffff

/* Pool tag */
#define POOLTAG 0x48444247 // [H]yper[DBG] (HDBG)

/* System and User ring definitions */
#define DPL_USER   3
#define DPL_SYSTEM 0

/* RPL Mask */
#define RPL_MASK 3

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define ORDER_LONG    (sizeof(unsigned long) == 4 ? 5 : 6)

#define BITMAP_ENTRY(_nr, _bmap) ((_bmap))[(_nr) / BITS_PER_LONG]
#define BITMAP_SHIFT(_nr)        ((_nr) % BITS_PER_LONG)

//////////////////////////////////////////////////
//					 Structures					//
//////////////////////////////////////////////////

/**
 * @brief R/EFlags structure
 * 
 */
typedef union _RFLAGS
{
    struct
    {
        unsigned Reserved1 : 10;
        unsigned ID : 1;  // Identification flag
        unsigned VIP : 1; // Virtual interrupt pending
        unsigned VIF : 1; // Virtual interrupt flag
        unsigned AC : 1;  // Alignment check
        unsigned VM : 1;  // Virtual 8086 mode
        unsigned RF : 1;  // Resume flag
        unsigned Reserved2 : 1;
        unsigned NT : 1;   // Nested task flag
        unsigned IOPL : 2; // I/O privilege level
        unsigned OF : 1;
        unsigned DF : 1;
        unsigned IF : 1; // Interrupt flag
        unsigned TF : 1; // Task flag
        unsigned SF : 1; // Sign flag
        unsigned ZF : 1; // Zero flag
        unsigned Reserved3 : 1;
        unsigned AF : 1; // Borrow flag
        unsigned Reserved4 : 1;
        unsigned PF : 1; // Parity flag
        unsigned Reserved5 : 1;
        unsigned CF : 1; // Carry flag [Bit 0]
        unsigned Reserved6 : 32;
    };

    ULONG64 Content;
} RFLAGS, *PRFLAGS;

/**
 * @brief Segment attributes
 * 
 */
typedef union _SEGMENT_ATTRIBUTES
{
    USHORT UCHARs;
    struct
    {
        USHORT TYPE : 4; /* 0;  Bit 40-43 */
        USHORT S : 1;    /* 4;  Bit 44 */
        USHORT DPL : 2;  /* 5;  Bit 45-46 */
        USHORT P : 1;    /* 7;  Bit 47 */

        USHORT AVL : 1; /* 8;  Bit 52 */
        USHORT L : 1;   /* 9;  Bit 53 */
        USHORT DB : 1;  /* 10; Bit 54 */
        USHORT G : 1;   /* 11; Bit 55 */
        USHORT GAP : 4;

    } Fields;
} SEGMENT_ATTRIBUTES, *PSEGMENT_ATTRIBUTES;

/**
 * @brief Segment selector
 * 
 */
typedef struct _SEGMENT_SELECTOR
{
    USHORT             SEL;
    SEGMENT_ATTRIBUTES ATTRIBUTES;
    ULONG32            LIMIT;
    ULONG64            BASE;
} SEGMENT_SELECTOR, *PSEGMENT_SELECTOR;

/**
 * @brief Segment Descriptor
 * 
 */
typedef struct _SEGMENT_DESCRIPTOR
{
    USHORT LIMIT0;
    USHORT BASE0;
    UCHAR  BASE1;
    UCHAR  ATTR0;
    UCHAR  LIMIT1ATTR1;
    UCHAR  BASE2;
} SEGMENT_DESCRIPTOR, *PSEGMENT_DESCRIPTOR;

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

/**
 * @brief See: Page-Fault Error Code
 * 
 */
typedef union _PAGE_FAULT_ERROR_CODE
{
    ULONG32 All;
    struct
    {
        ULONG32 Present : 1;  // 0 = NotPresent
        ULONG32 Write : 1;    // 0 = Read
        ULONG32 User : 1;     // 0 = CPL==0
        ULONG32 Reserved : 1; //
        ULONG32 Fetch : 1;    //
    } Fields;
} PAGE_FAULT_ERROR_CODE, *PPAGE_FAULT_ERROR_CODE;

//////////////////////////////////////////////////
//				 Function Types					//
//////////////////////////////////////////////////

typedef void (*RunOnLogicalCoreFunc)(ULONG ProcessorID);

//////////////////////////////////////////////////
//					Logging						//
//////////////////////////////////////////////////

/**
 * @brief Types of log messages
 * 
 */
typedef enum _LOG_TYPE
{
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR

} LOG_TYPE;

/* Define log variables */

#if UseDbgPrintInsteadOfUsermodeMessageTracking
/* Use DbgPrint */
#    define LogInfo(format, ...)                           \
        DbgPrint("[+] Information (%s:%d) | " format "\n", \
                 __func__,                                 \
                 __LINE__,                                 \
                 __VA_ARGS__)

#    define LogWarning(format, ...)                    \
        DbgPrint("[-] Warning (%s:%d) | " format "\n", \
                 __func__,                             \
                 __LINE__,                             \
                 __VA_ARGS__)

#    define LogError(format, ...)                    \
        DbgPrint("[!] Error (%s:%d) | " format "\n", \
                 __func__,                           \
                 __LINE__,                           \
                 __VA_ARGS__);                       \
        DbgBreakPoint()

/* Log without any prefix */
#    define Log(format, ...) \
        DbgPrint(format "\n", __VA_ARGS__)

#else

#    define LogInfo(format, ...)                                        \
        LogSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE,               \
                              UseImmediateMessaging,                    \
                              ShowSystemTimeOnDebugMessages,            \
                              "[+] Information (%s:%d) | " format "\n", \
                              __func__,                                 \
                              __LINE__,                                 \
                              __VA_ARGS__)

#    define LogInfoImmediate(format, ...)                               \
        LogSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE,               \
                              TRUE,                                     \
                              ShowSystemTimeOnDebugMessages,            \
                              "[+] Information (%s:%d) | " format "\n", \
                              __func__,                                 \
                              __LINE__,                                 \
                              __VA_ARGS__)

#    define LogWarning(format, ...)                                 \
        LogSendMessageToQueue(OPERATION_LOG_WARNING_MESSAGE,        \
                              UseImmediateMessaging,                \
                              ShowSystemTimeOnDebugMessages,        \
                              "[-] Warning (%s:%d) | " format "\n", \
                              __func__,                             \
                              __LINE__,                             \
                              __VA_ARGS__)

#    define LogError(format, ...)                                 \
        LogSendMessageToQueue(OPERATION_LOG_ERROR_MESSAGE,        \
                              UseImmediateMessaging,              \
                              ShowSystemTimeOnDebugMessages,      \
                              "[!] Error (%s:%d) | " format "\n", \
                              __func__,                           \
                              __LINE__,                           \
                              __VA_ARGS__);                       \
        DbgBreakPoint()

/* Log without any prefix */
#    define Log(format, ...)                                 \
        LogSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE,    \
                              UseImmediateMessaging,         \
                              ShowSystemTimeOnDebugMessages, \
                              format "\n",                   \
                              __VA_ARGS__)

#endif // UseDbgPrintInsteadOfUsermodeMessageTracking

//////////////////////////////////////////////////
//			 Function Definitions				//
//////////////////////////////////////////////////

int
TestBit(int nth, unsigned long * addr);

void
ClearBit(int nth, unsigned long * addr);

void
SetBit(int nth, unsigned long * addr);

BOOLEAN
BroadcastToProcessors(ULONG ProcessorNumber, RunOnLogicalCoreFunc Routine);

UINT64
VirtualAddressToPhysicalAddress(PVOID VirtualAddress);

UINT64
PhysicalAddressToVirtualAddress(UINT64 PhysicalAddress);

int
MathPower(int Base, int Exp);

UINT64
FindSystemDirectoryTableBase();

//////////////////////////////////////////////////
//			 WDK Major Functions				//
//////////////////////////////////////////////////

/* Load & Unload */
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);
VOID
DrvUnload(PDRIVER_OBJECT DriverObject);

/* IRP Major Functions */
NTSTATUS
DrvCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS
DrvRead(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS
DrvWrite(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS
DrvClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS
DrvUnsupported(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS
DrvDispatchIoControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

//////////////////////////////////////////////////
//			         Functions  				//
//////////////////////////////////////////////////

#define MAX_EXEC_TRAMPOLINE_SIZE 100

/* A test function for Syscall hook */
VOID
SyscallHookTest();
/* Enable or Disable Syscall Hook for EFER MSR */
VOID
SyscallHookConfigureEFER(BOOLEAN EnableEFERSyscallHook);
/* Manage #UD Exceptions for EFER Syscall */
BOOLEAN
SyscallHookHandleUD(PGUEST_REGS Regs, UINT32 CoreIndex);
/* SYSRET instruction emulation routine */
BOOLEAN
SyscallHookEmulateSYSRET(PGUEST_REGS Regs);
/* SYSCALL instruction emulation routine */
BOOLEAN
SyscallHookEmulateSYSCALL(PGUEST_REGS Regs);
