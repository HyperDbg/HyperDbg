/**
 * @file Hooks.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Hook headers
 * @details 
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#pragma once
#include "Common.h"

//////////////////////////////////////////////////
//				   Syscall Hook					//
//////////////////////////////////////////////////

/**
 * @brief As we have just on sysret in all the Windows,
 * we use the following variable to hold its address
 * this way, we're not force to check for the instruction
 * so we remove the memory access to check for sysret 
 * in this case.
 * 
 */

/* Check for instruction sysret and syscall */
#define IS_SYSRET_INSTRUCTION(Code)   \
    (*((PUINT8)(Code) + 0) == 0x48 && \
     *((PUINT8)(Code) + 1) == 0x0F && \
     *((PUINT8)(Code) + 2) == 0x07)
#define IS_SYSCALL_INSTRUCTION(Code)  \
    (*((PUINT8)(Code) + 0) == 0x0F && \
     *((PUINT8)(Code) + 1) == 0x05)

#define IMAGE_DOS_SIGNATURE    0x5A4D     // MZ
#define IMAGE_OS2_SIGNATURE    0x454E     // NE
#define IMAGE_OS2_SIGNATURE_LE 0x454C     // LE
#define IMAGE_VXD_SIGNATURE    0x454C     // LE
#define IMAGE_NT_SIGNATURE     0x00004550 // PE00

//////////////////////////////////////////////////
//				   Structure					//
//////////////////////////////////////////////////

/**
 * @brief SSDT structure
 * 
 */
typedef struct _SSDTStruct
{
    LONG * pServiceTable;
    PVOID  pCounterTable;
#ifdef _WIN64
    ULONGLONG NumberOfServices;
#else
    ULONG NumberOfServices;
#endif
    PCHAR pArgumentTable;
} SSDTStruct, *PSSDTStruct;

typedef struct _HIDDEN_HOOKS_DETOUR_DETAILS
{
    LIST_ENTRY OtherHooksList;
    PVOID      HookedFunctionAddress;
    PVOID      ReturnAddress;
} HIDDEN_HOOKS_DETOUR_DETAILS, *PHIDDEN_HOOKS_DETOUR_DETAILS;

typedef struct _SYSTEM_MODULE_ENTRY
{
    HANDLE Section;
    PVOID  MappedBase;
    PVOID  ImageBase;
    ULONG  ImageSize;
    ULONG  Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR  FullPathName[256];
} SYSTEM_MODULE_ENTRY, *PSYSTEM_MODULE_ENTRY;

typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG               Count;
    SYSTEM_MODULE_ENTRY Module[0];
} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemModuleInformation         = 11,
    SystemKernelDebuggerInformation = 35
} SYSTEM_INFORMATION_CLASS,
    *PSYSTEM_INFORMATION_CLASS;

typedef NTSTATUS(NTAPI * ZWQUERYSYSTEMINFORMATION)(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID                   SystemInformation,
    IN ULONG                    SystemInformationLength,
    OUT PULONG ReturnLength     OPTIONAL);

NTSTATUS(*NtCreateFileOrig)
(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength);

//////////////////////////////////////////////////
//				   Hidden Hooks					//
//////////////////////////////////////////////////

PVOID(*ExAllocatePoolWithTagOrig)
(
    POOL_TYPE PoolType,
    SIZE_T    NumberOfBytes,
    ULONG     Tag);

// ----------------------------------------------------------------------

/* Hook in VMX Root Mode with hidden breakpoints (A pre-allocated buffer should be available) */
BOOLEAN
EptHookPerformPageHook(PVOID TargetAddress, UINT32 ProcessId);
/* Hook in VMX Root Mode with hidden detours and monitor (A pre-allocated buffer should be available) */
BOOLEAN
EptHookPerformPageHook2(PVOID TargetAddress, PVOID HookFunction, UINT32 ProcessId, BOOLEAN UnsetRead, BOOLEAN UnsetWrite, BOOLEAN UnsetExecute);
/* Hook in VMX Non Root Mode (hidden breakpoint) */
BOOLEAN
EptHook(PVOID TargetAddress, UINT32 ProcessId);
/* Hook in VMX Non Root Mode (hidden detours) */
BOOLEAN
EptHook2(PVOID TargetAddress, PVOID HookFunction, UINT32 ProcessId, BOOLEAN SetHookForRead, BOOLEAN SetHookForWrite, BOOLEAN SetHookForExec);
/* Handle hooked pages in Vmx-root mode */
BOOLEAN
EptHookHandleHookedPage(PGUEST_REGS Regs, EPT_HOOKED_PAGE_DETAIL * HookedEntryDetails, VMX_EXIT_QUALIFICATION_EPT_VIOLATION ViolationQualification, SIZE_T PhysicalAddress);
/* Remove a special hook from the hooked pages lists */
BOOLEAN
EptHookUnHookSinglePage(SIZE_T PhysicalAddress);
/* Remove all hooks from the hooked pages lists */
VOID
EptHookUnHookAllPages();
