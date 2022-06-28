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
} SEGMENT_REGISTERS;

/**
 * @brief Different methods of killing a process
 *
 */
typedef enum _PROCESS_KILL_METHODS
{
    PROCESS_KILL_METHOD_1 = 0,
    PROCESS_KILL_METHOD_2,
    PROCESS_KILL_METHOD_3,

} PROCESS_KILL_METHODS;

//////////////////////////////////////////////////
//				 Spinlock Funtions				//
//////////////////////////////////////////////////

BOOLEAN
SpinlockTryLock(volatile LONG * Lock);

void
SpinlockLock(volatile LONG * Lock);

void
SpinlockLockWithCustomWait(volatile LONG * Lock, unsigned MaxWait);

void
SpinlockUnlock(volatile LONG * Lock);

void
SpinlockInterlockedCompareExchange(
    LONG volatile * Destination,
    LONG            Exchange,
    LONG            Comperand);

#define ScopedSpinlock(LockObject, CodeToRun)   \
    MetaScopedExpr(SpinlockLock(&LockObject),   \
                   SpinlockUnlock(&LockObject), \
                   CodeToRun)

//////////////////////////////////////////////////
//					Constants					//
//////////////////////////////////////////////////

/*
 * @brief Windows IRQ Levels
 */
#define PASSIVE_LEVEL  0  // Passive release level
#define LOW_LEVEL      0  // Lowest interrupt level
#define APC_LEVEL      1  // APC interrupt level
#define DISPATCH_LEVEL 2  // Dispatcher level
#define CMCI_LEVEL     5  // CMCI handler level
#define CLOCK_LEVEL    13 // Interval clock level
#define IPI_LEVEL      14 // Interprocessor interrupt level
#define DRS_LEVEL      14 // Deferred Recovery Service level
#define POWER_LEVEL    14 // Power failure level
#define PROFILE_LEVEL  15 // timer used for profiling.
#define HIGH_LEVEL     15 // Highest interrupt level

/*
 * @brief Segment register and corresponding GDT meaning in Windows
 */
#define KGDT64_NULL      (0 * 16)     // NULL descriptor
#define KGDT64_R0_CODE   (1 * 16)     // kernel mode 64-bit code
#define KGDT64_R0_DATA   (1 * 16) + 8 // kernel mode 64-bit data (stack)
#define KGDT64_R3_CMCODE (2 * 16)     // user mode 32-bit code
#define KGDT64_R3_DATA   (2 * 16) + 8 // user mode 32-bit data
#define KGDT64_R3_CODE   (3 * 16)     // user mode 64-bit code
#define KGDT64_SYS_TSS   (4 * 16)     // kernel mode system task state
#define KGDT64_R3_CMTEB  (5 * 16)     // user mode 32-bit TEB
#define KGDT64_R0_CMCODE (6 * 16)     // kernel mode 32-bit code
#define KGDT64_LAST      (7 * 16)     // last entry

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

/**
 * @brief EFLAGS/RFLAGS
 *
 */
#define X86_FLAGS_CF                 (1 << 0)
#define X86_FLAGS_PF                 (1 << 2)
#define X86_FLAGS_AF                 (1 << 4)
#define X86_FLAGS_ZF                 (1 << 6)
#define X86_FLAGS_SF                 (1 << 7)
#define X86_FLAGS_TF                 (1 << 8)
#define X86_FLAGS_IF                 (1 << 9)
#define X86_FLAGS_DF                 (1 << 10)
#define X86_FLAGS_OF                 (1 << 11)
#define X86_FLAGS_STATUS_MASK        (0xfff)
#define X86_FLAGS_IOPL_MASK          (3 << 12)
#define X86_FLAGS_IOPL_SHIFT         (12)
#define X86_FLAGS_IOPL_SHIFT_2ND_BIT (13)
#define X86_FLAGS_NT                 (1 << 14)
#define X86_FLAGS_RF                 (1 << 16)
#define X86_FLAGS_VM                 (1 << 17)
#define X86_FLAGS_AC                 (1 << 18)
#define X86_FLAGS_VIF                (1 << 19)
#define X86_FLAGS_VIP                (1 << 20)
#define X86_FLAGS_ID                 (1 << 21)
#define X86_FLAGS_RESERVED_ONES      0x2
#define X86_FLAGS_RESERVED           0xffc0802a

#define X86_FLAGS_RESERVED_BITS 0xffc38028
#define X86_FLAGS_FIXED         0x00000002

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
#define CPUID_HV_INTERFACE                0x40000001

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
#define RESERVED_MSR_RANGE_HI  0x400000F0

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
 * @brief Pool tag
 *
 */
#define POOLTAG 0x48444247 // [H]yper[DBG] (HDBG)

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
#define _XABORT_CODE(x)  (((x) >> 24) & 0xFF)

//////////////////////////////////////////////////
//					 Structures					//
//////////////////////////////////////////////////

typedef SEGMENT_DESCRIPTOR_32 * PSEGMENT_DESCRIPTOR;

/**
 * @brief Segment selector
 *
 */

typedef struct _VMX_SEGMENT_SELECTOR
{
    UINT16                    Selector;
    VMX_SEGMENT_ACCESS_RIGHTS Attributes;
    UINT32                    Limit;
    UINT64                    Base;
} VMX_SEGMENT_SELECTOR, *PVMX_SEGMENT_SELECTOR;

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
 * @brief Page-Fault Error Code
 *
 */
typedef union _PAGE_FAULT_ERROR_CODE
{
    UINT32 Flags;
    struct
    {
        UINT32 Present : 1;  // 0 = NotPresent
        UINT32 Write : 1;    // 0 = Read
        UINT32 User : 1;     // 0 = CPL==0
        UINT32 Reserved : 1; //
        UINT32 Fetch : 1;    //
    } Fields;
} PAGE_FAULT_ERROR_CODE, *PPAGE_FAULT_ERROR_CODE;

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
//				 Function Types					//
//////////////////////////////////////////////////

/**
 * @brief Prototype to run a function on a logical core
 *
 */
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

/**
 * @brief Define log variables
 *
 */
#if UseDbgPrintInsteadOfUsermodeMessageTracking
/* Use DbgPrint */
#    define Logformat, ...)                           \
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

/**
 * @brief Log without any prefix
 *
 */
#    define Log(format, ...) \
        DbgPrint(format, __VA_ARGS__)

#else

/**
 * @brief Log, general
 *
 */
#    define LogInfo(format, ...)                                                  \
        LogPrepareAndSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE,               \
                                        UseImmediateMessaging,                    \
                                        ShowSystemTimeOnDebugMessages,            \
                                        FALSE,                                    \
                                        "[+] Information (%s:%d) | " format "\n", \
                                        __func__,                                 \
                                        __LINE__,                                 \
                                        __VA_ARGS__)

/**
 * @brief Log in the case of priority message
 *
 */
#    define LogInfoPriority(format, ...)                                          \
        LogPrepareAndSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE,               \
                                        TRUE,                                     \
                                        ShowSystemTimeOnDebugMessages,            \
                                        TRUE,                                     \
                                        "[+] Information (%s:%d) | " format "\n", \
                                        __func__,                                 \
                                        __LINE__,                                 \
                                        __VA_ARGS__)

/**
 * @brief Log in the case of warning
 *
 */
#    define LogWarning(format, ...)                                           \
        LogPrepareAndSendMessageToQueue(OPERATION_LOG_WARNING_MESSAGE,        \
                                        UseImmediateMessaging,                \
                                        ShowSystemTimeOnDebugMessages,        \
                                        TRUE,                                 \
                                        "[-] Warning (%s:%d) | " format "\n", \
                                        __func__,                             \
                                        __LINE__,                             \
                                        __VA_ARGS__)

/**
 * @brief Log in the case of error
 *
 */
#    define LogError(format, ...)                                           \
        LogPrepareAndSendMessageToQueue(OPERATION_LOG_ERROR_MESSAGE,        \
                                        UseImmediateMessaging,              \
                                        ShowSystemTimeOnDebugMessages,      \
                                        TRUE,                               \
                                        "[!] Error (%s:%d) | " format "\n", \
                                        __func__,                           \
                                        __LINE__,                           \
                                        __VA_ARGS__);                       \
        if (DebugMode)                                                      \
        DbgBreakPoint()

/**
 * @brief Log without any prefix
 *
 */
#    define Log(format, ...)                                        \
        LogPrepareAndSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE, \
                                        TRUE,                       \
                                        FALSE,                      \
                                        FALSE,                      \
                                        format,                     \
                                        __VA_ARGS__)

/**
 * @brief Log without any prefix and bypass the stack
 * problem (getting two temporary stacks in preparing phase)
 *
 */
#    define LogSimpleWithTag(tag, isimmdte, buffer, len) \
        LogSendMessageToQueue(tag,                       \
                              isimmdte,                  \
                              buffer,                    \
                              len,                       \
                              FALSE)

#endif // UseDbgPrintInsteadOfUsermodeMessageTracking

/**
 * @brief Log, initilize boot information and debug information
 *
 */
#define LogDebugInfo(format, ...)                                             \
    if (DebugMode)                                                            \
    LogPrepareAndSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE,               \
                                    UseImmediateMessaging,                    \
                                    ShowSystemTimeOnDebugMessages,            \
                                    FALSE,                                    \
                                    "[+] Information (%s:%d) | " format "\n", \
                                    __func__,                                 \
                                    __LINE__,                                 \
                                    __VA_ARGS__)

//////////////////////////////////////////////////
//				External Functions				//
//////////////////////////////////////////////////

UCHAR *
PsGetProcessImageFileName(IN PEPROCESS Process);

NTKERNELAPI NTSTATUS NTAPI
SeCreateAccessState(
    PACCESS_STATE    AccessState,
    PVOID            AuxData,
    ACCESS_MASK      DesiredAccess,
    PGENERIC_MAPPING Mapping);

NTKERNELAPI VOID NTAPI
SeDeleteAccessState(
    PACCESS_STATE AccessState);

PVOID
PsGetProcessSectionBaseAddress(PEPROCESS Process); // Used to get the base address of process's executable image

NTSTATUS
MmUnmapViewOfSection(PEPROCESS Process, PVOID BaseAddress); // Used to unmap process's executable image

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

int
TestBit(int nth, unsigned long * addr);

void
ClearBit(int nth, unsigned long * addr);

void
SetBit(int nth, unsigned long * addr);

CR3_TYPE
GetCr3FromProcessId(_In_ UINT32 ProcessId);

BOOLEAN
BroadcastToProcessors(_In_ ULONG ProcessorNumber, _In_ RunOnLogicalCoreFunc Routine);

UINT64
PhysicalAddressToVirtualAddress(_In_ UINT64 PhysicalAddress);

UINT64
VirtualAddressToPhysicalAddress(_In_ PVOID VirtualAddress);

UINT64
VirtualAddressToPhysicalAddressByProcessId(_In_ PVOID  VirtualAddress,
                                           _In_ UINT32 ProcessId);

UINT64
VirtualAddressToPhysicalAddressByProcessCr3(_In_ PVOID    VirtualAddress,
                                            _In_ CR3_TYPE TargetCr3);

UINT64
VirtualAddressToPhysicalAddressOnTargetProcess(_In_ PVOID VirtualAddress);

UINT64
PhysicalAddressToVirtualAddressByProcessId(_In_ PVOID PhysicalAddress, _In_ UINT32 ProcessId);

UINT64
PhysicalAddressToVirtualAddressByCr3(_In_ PVOID PhysicalAddress, _In_ CR3_TYPE TargetCr3);

UINT64
PhysicalAddressToVirtualAddressOnTargetProcess(_In_ PVOID PhysicalAddress);

CR3_TYPE
GetRunningCr3OnTargetProcess();

int
MathPower(int Base, int Exp);

UINT64
FindSystemDirectoryTableBase();

CR3_TYPE
SwitchOnAnotherProcessMemoryLayout(_In_ UINT32 ProcessId);

CR3_TYPE
SwitchOnMemoryLayoutOfTargetProcess();

CR3_TYPE
SwitchOnAnotherProcessMemoryLayoutByCr3(_In_ CR3_TYPE TargetCr3);

VOID
RestoreToPreviousProcess(_In_ CR3_TYPE PreviousProcess);

PCHAR
GetProcessNameFromEprocess(PEPROCESS eprocess);

BOOLEAN
StartsWith(const char * pre, const char * str);

BOOLEAN
IsProcessExist(UINT32 ProcId);

BOOLEAN
CheckIfAddressIsValidUsingTsx(CHAR * Address);

VOID
GetCpuid(UINT32 Func, UINT32 SubFunc, int * CpuInfo);

BOOLEAN
CheckCpuSupportRtm();

UINT64 *
AllocateInvalidMsrBimap();

UINT32
Getx86VirtualAddressWidth();

BOOLEAN
CheckCanonicalVirtualAddress(UINT64 VAddr, PBOOLEAN IsKernelAddress);

BOOLEAN
CheckMemoryAccessSafety(UINT64 TargetAddress, UINT32 Size);

//////////////////////////////////////////////////
//			 WDK Major Functions				//
//////////////////////////////////////////////////

/**
 * @brief Load & Unload
 */
NTSTATUS
DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath);

VOID
DrvUnload(PDRIVER_OBJECT DriverObject);

/**
 * @brief IRP Major Functions
 */
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

/**
 * @brief A test function for Syscall hook
 *
 * @return VOID
 */
VOID
SyscallHookTest();

/**
 * @brief Enable or Disable Syscall Hook for EFER MSR
 *
 */
VOID
SyscallHookConfigureEFER(BOOLEAN EnableEFERSyscallHook);

/**
 * @brief Manage #UD Exceptions for EFER Syscall
 *
 */
BOOLEAN
SyscallHookHandleUD(_Inout_ PGUEST_REGS Regs, _In_ UINT32 CoreIndex);

/**
 * @brief SYSRET instruction emulation routine
 *
 */
BOOLEAN
SyscallHookEmulateSYSRET(_In_ PGUEST_REGS Regs);

/**
 * @brief SYSCALL instruction emulation routine
 *
 */
BOOLEAN
SyscallHookEmulateSYSCALL(_Out_ PGUEST_REGS Regs);

/**
 * @brief Get Segment Descriptor
 *
 */
_Success_(return )
BOOLEAN
GetSegmentDescriptor(_In_ PUCHAR GdtBase, _In_ UINT16 Selector, _Out_ PVMX_SEGMENT_SELECTOR SegmentSelector);

/**
 * @brief Kill a process using different methods
 *
 */
BOOLEAN
KillProcess(_In_ UINT32 ProcessId, _In_ PROCESS_KILL_METHODS KillingMethod);

UINT32
VmxrootCompatibleStrlen(const CHAR * S);

UINT32
VmxrootCompatibleWcslen(const wchar_t * S);
