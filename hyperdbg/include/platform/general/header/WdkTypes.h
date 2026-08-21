/**
 * @file WdkTypes.h
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Linux stand-ins for the WDK / NT kernel types the shared sources use
 *
 * @details The Windows kernel projects get these from <ntddk.h> / <ntifs.h>.
 * On Linux there is no such header, so every NT type, status code and ntdef
 * macro that shared code mentions is declared here instead — in ONE place,
 * rather than scattered through the SDK headers.
 *
 * Three kinds of declaration live here, and the difference matters:
 *
 *   1. Faithful   — the type means the same thing on both sides (NTSTATUS,
 *                   CLIENT_ID, UNICODE_STRING). Safe to rely on.
 *   2. Structural — the member NAMES match the WDK so shared code compiles
 *                   unchanged, but the LAYOUT is not the NT one (IRP,
 *                   IO_STACK_LOCATION). Nothing on Linux produces one of these
 *                   yet; the Platform* wrappers that would fill them in are
 *                   still stubs.
 *   3. Opaque     — a correctly-sized blob for objects only ever passed by
 *                   pointer into a wrapper (KEVENT, KAPC_STATE, EPROCESS).
 *
 * Included from SDK/headers/BasicTypes.h, so only the scalar types declared
 * above that point are available here — deliberately no dependency on
 * DataTypes.h (which comes later in HyperDbgSdk.h).
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Guarded on the compiler's own macro rather than HYPERDBG_ENV_LINUX, because
// BasicTypes.h is reached before Environment.h has run in some include chains
//
#if defined(__linux__)

//////////////////////////////////////////////////
//         Annotations / linkage keywords       //
//////////////////////////////////////////////////

//
// Export/calling-convention markers on the WDK prototypes. They carry no meaning
// in a single Linux module, but they sit in front of declarations, so they have
// to expand to nothing rather than be missing.
//
#    define NTKERNELAPI
#    define NTSYSAPI
#    define DECLSPEC_NORETURN

//
// Alignment attribute on the VMX region / per-CPU state structures. Real effect
// on both sides, just a different spelling.
//
#    define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))

//
// Pre-SAL2 annotations (__in / __out family). The modern _In_ / _Out_ spellings
// are in Environment.h with the rest of the SAL set; these older ones only ever
// appear in the NT headers' own prototypes.
//
#    define __in
#    define __in_opt
#    define __out
#    define __out_opt
#    define __inout
#    define __inout_opt
#    define __in_bcount(x)
#    define __out_bcount(x)
#    define _Strict_type_match_

//
// NOT defined here: __reserved. Nothing in the tree annotates with it, and
// glibc's <linux/stat.h> uses that exact spelling as a STRUCT MEMBER name
// (struct statx_timestamp), so defining it away breaks any TU that includes
// <sys/stat.h> after this header.
//

//
// The original (pre-__in) annotation spelling, still used across the NT process
// and object prototypes
//
#    define IN
#    define OUT
#    define OPTIONAL

//////////////////////////////////////////////////
//                 Scalar types                 //
//////////////////////////////////////////////////

typedef LONG    NTSTATUS;
typedef LONG    KPRIORITY;
typedef ULONG   ACCESS_MASK;
typedef CHAR    KPROCESSOR_MODE;
typedef CHAR    CCHAR;
typedef UCHAR   KIRQL; // IRQL token; the raise/lower maps to preempt_disable/enable
typedef KIRQL * PKIRQL;
typedef INT64   SSIZE_T;

//
// Processor-affinity bitmask (ntdef.h). FAITHFUL for <= 64 cores, which is the
// same ceiling the Windows type has inside one processor group.
//
typedef ULONG_PTR KAFFINITY;

//
// A physical address is a LARGE_INTEGER on Windows; keeping the union means the
// .QuadPart / .LowPart accesses in the memory code compile unchanged.
//
typedef LARGE_INTEGER PHYSICAL_ADDRESS, *PPHYSICAL_ADDRESS;
typedef WCHAR * PWSTR;

//
// Processor mode discriminator (ntdef.h MODE). Passed to the object/handle
// wrappers; only the two enumerators are ever named.
//
typedef enum _MODE
{
    KernelMode,
    UserMode,
    MaximumMode
} MODE;

//
// Pool type passed to the (already wrapped) allocation routines. Values match
// the WDK so any stored/compared constant keeps its meaning.
//
typedef enum _POOL_TYPE
{
    NonPagedPool             = 0,
    PagedPool                = 1,
    NonPagedPoolMustSucceed  = 2,
    NonPagedPoolCacheAligned = 4,
    PagedPoolCacheAligned    = 5,
    NonPagedPoolNx           = 512
} POOL_TYPE;

//////////////////////////////////////////////////
//                Status codes                  //
//////////////////////////////////////////////////

#    define STATUS_SUCCESS                ((NTSTATUS)0x00000000L)
#    define STATUS_PENDING                ((NTSTATUS)0x00000103L)
#    define STATUS_NOT_IMPLEMENTED        ((NTSTATUS)0xC0000002L)
#    define STATUS_INVALID_PARAMETER      ((NTSTATUS)0xC000000DL)
#    define STATUS_INFO_LENGTH_MISMATCH   ((NTSTATUS)0xC0000004L)
#    define STATUS_INSUFFICIENT_RESOURCES ((NTSTATUS)0xC000009AL)
#    define STATUS_UNSUCCESSFUL           ((NTSTATUS)0xC0000001L)
#    define STATUS_ACCESS_DENIED          ((NTSTATUS)0xC0000022L)
#    define NT_SUCCESS(Status)            (((NTSTATUS)(Status)) >= 0)

//////////////////////////////////////////////////
//               Access rights                  //
//////////////////////////////////////////////////

#    define SYNCHRONIZE         0x00100000L
#    define EVENT_MODIFY_STATE  0x0002
#    define PROCESS_ALL_ACCESS  0x001FFFFF

//////////////////////////////////////////////////
//        Memory protection / allocation        //
//////////////////////////////////////////////////

//
// Page protection and allocation-type flags (winnt.h). Kept at their Windows
// values because the shared code both passes and stores them.
//
#    define PAGE_NOCACHE            0x200
#    define PAGE_READWRITE          0x04
#    define PAGE_EXECUTE_READWRITE  0x40

#    define MEM_COMMIT              0x00001000
#    define MEM_RESERVE             0x00002000
#    define MEM_RELEASE             0x00008000

//
// Type maxima (ntdef.h / winnt.h)
//
#    define MAXDWORD64 0xffffffffffffffffULL

//////////////////////////////////////////////////
//                ntdef macros                  //
//////////////////////////////////////////////////

//
// Element count (ntdef.h)
//
#    define RTL_NUMBER_OF(A) (sizeof(A) / sizeof((A)[0]))

//
// Element count of an ARRAY MEMBER of a struct type (ntdef.h)
//
#    define RTL_NUMBER_OF_FIELD(type, field) \
        (RTL_NUMBER_OF(((type *)0)->field))

//
// Recover the containing struct from a pointer to one of its members (ntdef.h).
// The Linux kernel's own container_of is the same idea but takes its arguments
// in a different order, so the NT spelling is kept.
//
#    define CONTAINING_RECORD(address, type, field) \
        ((type *)((PCHAR)(address) - (ULONG_PTR)(&((type *)0)->field)))

//
// The Rtl memory helpers are macros over memset/memcpy on Windows too, so these
// are the same mapping rather than a behavioural substitution
//
#    define RtlZeroMemory(Destination, Length) memset((Destination), 0, (Length))
#    define RtlZeroBytes(Destination, Length)  memset((Destination), 0, (Length))
#    define RtlCopyMemory(Destination, Source, Length) memcpy((Destination), (Source), (Length))
#    define RtlCopyBytes(Destination, Source, Length)  memcpy((Destination), (Source), (Length))

#    ifdef HYPERDBG_KERNEL_MODE

//
// Truncate a virtual address to its page base (ntddk.h)
//
#        define PAGE_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(ULONG_PTR)(PAGE_SIZE - 1)))

//
// Pageable-code annotation. It is a driver-verifier IRQL assertion on Windows
// and has no Linux equivalent — the kernel has no paged pool for code.
//
#        define PAGED_CODE()

//
// NT ASSERT. On a checked Windows build this bugchecks; the closest Linux analog
// that keeps the machine alive is WARN_ON (splat + continue).
//
#        define ASSERT(Expression) WARN_ON(!(Expression))

//
// The ntddk.h spelling of the same assertion (it differs from ASSERT only in
// how the Windows debugger reports it).
//
#        define NT_ASSERT(Expression) ASSERT(Expression)

//
// The Linux kernel has no wchar_t at all (it is a libc typedef, and <wchar.h> is
// user space only). Windows kernel code builds with /Zc:wchar_t-, where wchar_t
// IS unsigned short, so UINT16 matches that side exactly. User-mode Linux keeps
// the real 4-byte builtin — hence the kernel-only guard.
//
typedef UINT16 wchar_t;

#    endif // HYPERDBG_KERNEL_MODE

//////////////////////////////////////////////////
//                  Strings                     //
//////////////////////////////////////////////////

//
// NT counted string (ntdef.h). Faithful.
//
typedef struct _UNICODE_STRING
{
    USHORT  Length;
    USHORT  MaximumLength;
    WCHAR * Buffer;
} UNICODE_STRING, *PUNICODE_STRING;

//
// 32-bit views of the same structures, used when the debugger walks a WoW64
// process's structures from 64-bit code. Faithful: the pointer members are
// deliberately ULONG on both platforms, since they describe a 32-bit address
// space rather than this one.
//
typedef struct _UNICODE_STRING32
{
    USHORT Length;
    USHORT MaximumLength;
    ULONG  Buffer;
} UNICODE_STRING32, *PUNICODE_STRING32;

typedef struct _LIST_ENTRY32
{
    ULONG Flink;
    ULONG Blink;
} LIST_ENTRY32, *PLIST_ENTRY32;

//////////////////////////////////////////////////
//           Kernel objects (opaque)            //
//////////////////////////////////////////////////

//
// Event / object-manager handles. Nothing in the shared tree dereferences these;
// they travel to the PlatformEvent wrappers, which are stubbed until an eventfd
// backing lands.
//
//
// KEVENT is embedded BY VALUE (hyperkd's globals hold one), so unlike the other
// object handles it has to be a complete type. Sized opaque blob: 0x18 bytes on
// x64, matching the NT dispatcher object. A real version maps it onto a Linux
// wait_queue_head; until PlatformEvent stops being a stub, nothing reads it.
//
typedef struct _KEVENT
{
    UCHAR Reserved[0x18];
} KEVENT, *PKEVENT;
typedef PKEVENT        PRKEVENT; // the "R" spelling is the same object, referenced
typedef PVOID          POBJECT_TYPE;
typedef PVOID          POBJECT_HANDLE_INFORMATION;

//
// Dispatcher object header, embedded by value at the front of every waitable NT
// object. Sized blob (0x18 on x64) rather than the real field set, which would
// need LIST_ENTRY — that lives in DataTypes.h, included after this header.
//
typedef struct _DISPATCHER_HEADER
{
    UCHAR Reserved[0x18];
} DISPATCHER_HEADER, *PDISPATCHER_HEADER;

//
// I/O manager device/driver objects. Opaque — the char device that will back
// them does not exist yet, and no shared code reads their members.
//
typedef struct _DEVICE_OBJECT DEVICE_OBJECT, *PDEVICE_OBJECT;
typedef struct _DRIVER_OBJECT DRIVER_OBJECT, *PDRIVER_OBJECT;

//
// Process / thread objects. Opaque: the Linux backing is task_struct, reached
// through the PlatformProcess wrappers.
//
typedef struct _EPROCESS EPROCESS, *PEPROCESS;
typedef struct _ETHREAD  ETHREAD, *PETHREAD;

//
// Process + thread id pair (ntdef.h). Faithful.
//
typedef struct _CLIENT_ID
{
    HANDLE UniqueProcess;
    HANDLE UniqueThread;
} CLIENT_ID, *PCLIENT_ID;

//
// APC state saved across KeStackAttachProcess/KeUnstackDetachProcess. Only ever
// passed by pointer into those (stubbed) wrappers, so this is a correctly-sized
// opaque blob — 0x30 bytes on x64 — instead of a LIST_ENTRY-dependent struct
// (LIST_ENTRY lives in DataTypes.h, which is included after this header).
//
typedef struct _KAPC_STATE
{
    UCHAR Reserved[0x30];
} KAPC_STATE, *PKAPC_STATE;

//////////////////////////////////////////////////
//              Object attributes               //
//////////////////////////////////////////////////

#    define OBJ_INHERIT          0x00000002L
#    define OBJ_CASE_INSENSITIVE 0x00000040L
#    define OBJ_KERNEL_HANDLE    0x00000200L

typedef struct _OBJECT_ATTRIBUTES
{
    ULONG            Length;
    HANDLE           RootDirectory;
    PUNICODE_STRING  ObjectName;
    ULONG            Attributes;
    PVOID            SecurityDescriptor;
    PVOID            SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;

//
// ntdef.h defines this as a macro, not a function — same here
//
#    define InitializeObjectAttributes(p, n, a, r, s) \
        {                                             \
            (p)->Length                   = sizeof(OBJECT_ATTRIBUTES); \
            (p)->RootDirectory            = (r);      \
            (p)->Attributes               = (a);      \
            (p)->ObjectName               = (n);      \
            (p)->SecurityDescriptor       = (s);      \
            (p)->SecurityQualityOfService = NULL;     \
        }

//
// Object-type access mapping, and the access state threaded through
// SeCreateAccessState -> ObOpenObjectByPointer. Structural: shared code touches
// only the two masks below, and the Se* wrappers that would populate the rest
// are stubs, so the trailing reserve is padding rather than a real layout.
//
typedef struct _GENERIC_MAPPING
{
    ACCESS_MASK GenericRead;
    ACCESS_MASK GenericWrite;
    ACCESS_MASK GenericExecute;
    ACCESS_MASK GenericAll;
} GENERIC_MAPPING, *PGENERIC_MAPPING;

typedef struct _ACCESS_STATE
{
    ACCESS_MASK RemainingDesiredAccess;
    ACCESS_MASK PreviouslyGrantedAccess;
    UCHAR       Reserved[0x58];
} ACCESS_STATE, *PACCESS_STATE;

//////////////////////////////////////////////////
//             Process information              //
//////////////////////////////////////////////////

typedef enum _PROCESSINFOCLASS
{
    ProcessBasicInformation = 0,
    ProcessImageFileName    = 27
} PROCESSINFOCLASS;

typedef struct _PROCESS_BASIC_INFORMATION
{
    NTSTATUS  ExitStatus;
    PVOID     PebBaseAddress;
    ULONG_PTR AffinityMask;
    KPRIORITY BasePriority;
    ULONG_PTR UniqueProcessId;
    ULONG_PTR InheritedFromUniqueProcessId;
} PROCESS_BASIC_INFORMATION, *PPROCESS_BASIC_INFORMATION;

//////////////////////////////////////////////////
//                 I/O manager                  //
//////////////////////////////////////////////////

//
// Structural: the member names are the WDK's so the shared IRP code compiles
// unchanged, but the layout is NOT the NT one and nothing on Linux builds one of
// these yet (a real version maps the IRP onto a char-device read/ioctl request,
// with PlatformIo filling it in).
//

#    define IO_NO_INCREMENT 0

typedef struct _IO_STATUS_BLOCK
{
    NTSTATUS  Status;
    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;

typedef struct _IO_STACK_LOCATION
{
    UCHAR MajorFunction;
    UCHAR MinorFunction;

    union
    {
        struct
        {
            ULONG OutputBufferLength;
            ULONG InputBufferLength;
            ULONG IoControlCode;
            PVOID Type3InputBuffer;
        } DeviceIoControl;
    } Parameters;
} IO_STACK_LOCATION, *PIO_STACK_LOCATION;

typedef struct _IRP
{
    CCHAR StackCount;
    CCHAR CurrentLocation;

    union
    {
        PVOID SystemBuffer;
    } AssociatedIrp;

    IO_STATUS_BLOCK IoStatus;
    KPROCESSOR_MODE RequestorMode;
    PVOID           UserBuffer;
} IRP, *PIRP;

//////////////////////////////////////////////////
//                Memory manager                //
//////////////////////////////////////////////////

//
// Faithful: the WDK caching enum. Only the members shared code names are listed.
// The raw Mm* wrappers that consume it are stubs (PlatformWdk.c), so the value
// is not acted upon yet.
//
typedef enum _MEMORY_CACHING_TYPE
{
    MmNonCached      = 0,
    MmCached         = 1,
    MmWriteCombined  = 2,
} MEMORY_CACHING_TYPE;

//
// Structural: member names match the WDK so MmCopyMemory-style call sites
// compile. Nothing on Linux fills one in yet.
//
typedef struct _MM_COPY_ADDRESS
{
    union
    {
        PVOID            VirtualAddress;
        PHYSICAL_ADDRESS PhysicalAddress;
    };
} MM_COPY_ADDRESS, *PMMPFN_IDENTITY;

//
// Flags for MmCopyMemory's source-address interpretation (WDK values).
//
#    define MM_COPY_MEMORY_PHYSICAL 0x1
#    define MM_COPY_MEMORY_VIRTUAL  0x2

//
// Structural: one span returned by MmGetPhysicalMemoryRanges (a NULL-terminated
// array). The wrapper is a stub, so no array is produced yet.
//
typedef struct _PHYSICAL_MEMORY_RANGE
{
    PHYSICAL_ADDRESS BaseAddress;
    LARGE_INTEGER    NumberOfBytes;
} PHYSICAL_MEMORY_RANGE, *PPHYSICAL_MEMORY_RANGE;

//
// Faithful: the WDK's (group, number) processor coordinate. Single-group on the
// platforms we target, so Group is always 0.
//
typedef struct _PROCESSOR_NUMBER
{
    USHORT Group;
    UCHAR  Number;
    UCHAR  Reserved;
} PROCESSOR_NUMBER, *PPROCESSOR_NUMBER;

#endif // defined(__linux__)
