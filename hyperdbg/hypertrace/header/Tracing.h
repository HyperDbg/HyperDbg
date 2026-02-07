/**
 * @file Tracing.h
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Message logging and tracing implementation
 * @details Modified from LIBIHT project (Thomasaon Zhao et al) with Windows style updates.
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */

#pragma once

// Intel MSR Constants
#define MSR_IA32_DEBUGCTLMSR 0x000001D9
#define DEBUGCTLMSR_LBR      (1ULL << 0)
#define MSR_LBR_SELECT       0x000001C8
#define MSR_LBR_TOS          0x000001C9
#define MSR_LBR_NHM_FROM     0x00000680
#define MSR_LBR_NHM_TO       0x000006C0
#define LBR_SELECT           0x0

//////////////////////////////////////////////////
//                  Structures                  //
//////////////////////////////////////////////////

typedef struct _LBR_STACK_ENTRY
{
    ULONGLONG from;
    ULONGLONG to;

} LBR_STACK_ENTRY, PLBR_STACK_ENTRY;

typedef struct _LBR_DATA
{
    ULONGLONG         lbr_tos;
    LBR_STACK_ENTRY * entries;

} LBR_DATA, *PLBR_DATA;

typedef struct _LBR_CONFIG
{
    ULONG     pid;
    ULONGLONG lbr_select;

} LBR_CONFIG, *PLBR_CONFIG;

typedef struct _LBR_STATE
{
    LBR_CONFIG config;
    LBR_DATA * data;
    PVOID     parent;
    LIST_ENTRY list;

} LBR_STATE, *PLBR_STATE;

typedef struct _LBR_IOCTL_REQUEST
{
    LBR_CONFIG lbr_config;
    LBR_DATA * buffer;

} LBR_IOCTL_REQUEST, *PLBR_IOCTL_REQUEST;

typedef struct _XIOCTL_REQUEST
{
    ULONG cmd;
    union
    {
        LBR_IOCTL_REQUEST lbr;
    } body;

} XIOCTL_REQUEST, *PXIOCTL_REQUEST;

// IOCTL Commands
#define LIBIHT_IOCTL_ENABLE_LBR  0x1
#define LIBIHT_IOCTL_DISABLE_LBR 0x2
#define LIBIHT_IOCTL_DUMP_LBR    0x3
#define LIBIHT_IOCTL_CONFIG_LBR  0x4

//////////////////////////////////////////////////
//             Platform Wrappers                //
//////////////////////////////////////////////////

#define xmalloc(sz)            PlatformMemAllocateZeroedNonPagedPool(sz)
#define xfree(p)               PlatformMemFreePool(p)
#define xmemset(ptr, sz)       RtlZeroMemory(ptr, sz)
#define xmemcpy                RtlCopyMemory
#define xrdmsr(msr, pval)      (*(pval) = __readmsr(msr))
#define xwrmsr(msr, val)       __writemsr(msr, val)
#define xprintdbg(format, ...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, format, __VA_ARGS__)
#define xcoreid()              KeGetCurrentProcessorNumber()
#define xgetcurrent_pid()      (ULONG)(ULONG_PTR) PsGetCurrentProcessId()

// List Handling
#define xlist_next(ptr)        (ptr)->Flink
#define xlist_add(entry, head) InsertTailList(&(head), &(entry))
#define xlist_del(entry)       RemoveEntryList(&(entry))

// Spinlock & IRQL (Using Windows Native to fix VCR001)
#define xacquire_lock(Lock, Irql) KeAcquireSpinLock((PKSPIN_LOCK)(Lock), (Irql))
#define xrelease_lock(Lock, Irql) KeReleaseSpinLock((PKSPIN_LOCK)(Lock), *(Irql))

#define xlock_core(irql)    KeRaiseIrql(DISPATCH_LEVEL, irql)
#define xrelease_core(irql) KeLowerIrql(*(irql))

// Buffer Copy
#define xcopy_from_user(dest, src, sz) (RtlCopyMemory(dest, src, sz), 0)
#define xcopy_to_user(dest, src, sz)   (RtlCopyMemory(dest, src, sz), 0)

// CPUID (Fixed C6001: initialized cpuInfo)
#define xcpuid(code, a, b, c, d) \
    {                            \
        int cpuInfo[4] = {0};    \
        __cpuid(cpuInfo, code);  \
        *a = cpuInfo[0];         \
        *b = cpuInfo[1];         \
        *c = cpuInfo[2];         \
        *d = cpuInfo[3];         \
    }

//////////////////////////////////////////////////
//                Global Variables              //
//////////////////////////////////////////////////

extern ULONGLONG  lbr_capacity;
extern LIST_ENTRY lbr_state_head;
extern KSPIN_LOCK lbr_state_lock; // Standardized to KSPIN_LOCK

typedef struct _CPU_LBR_MAP
{
    ULONG model;
    ULONG lbr_capacity;
} CPU_LBR_MAP, *PCPU_LBR_MAP;

extern CPU_LBR_MAP CPU_LBR_MAPS[];

//////////////////////////////////////////////////
//                  Prototypes                  //
//////////////////////////////////////////////////

VOID
LbrGetLbr(LBR_STATE * State);

VOID
LbrPutLbr(LBR_STATE * State);

LBR_STATE *
LbrCreateLbrState();

LBR_STATE *
LbrFindLbrState(ULONG Pid);

VOID
LbrInsertLbrState(LBR_STATE * NewState);

VOID
LbrRemoveLbrState(LBR_STATE * OldState);

VOID
LbrFreeLbrStatList();

NTSTATUS
LbrCheck();
