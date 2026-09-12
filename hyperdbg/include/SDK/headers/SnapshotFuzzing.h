/**
 * @file SnapshotFuzzing.h
 * @author HyperDbg Dev Team
 * @brief High-performance hypervisor-assisted snapshot fuzzing and coverage engine definitions.
 * @details Shared data structures and IOCTLs for in-memory vCPU snapshotting,
 *          EPT Access/Dirty (A/D) hardware tracking, Intel PT edge coverage mapping,
 *          and IDT/LBR crash triage.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#include "BasicTypes.h"
#include "Constants.h"
#include "Ioctls.h"
#include "LbrDefinitions.h"

#ifndef DECLSPEC_ALIGN
#    if defined(_MSC_VER)
#        define DECLSPEC_ALIGN(x) __declspec(align(x))
#    elif defined(__GNUC__) || defined(__clang__)
#        define DECLSPEC_ALIGN(x) __attribute__((aligned(x)))
#    else
#        define DECLSPEC_ALIGN(x)
#    endif
#endif

typedef LBR_BRANCH_ENTRY LBR_ENTRY, *PLBR_ENTRY;

//////////////////////////////////////////////////
//                  Constants                   //
//////////////////////////////////////////////////

#define SNAPSHOT_AFL_MAP_SIZE              65536ULL        /* 64 KB AFL-compatible coverage map */
#define SNAPSHOT_MAX_DIRTY_PAGES           16384ULL        /* Max tracked dirty pages (64 MB)   */
#define SNAPSHOT_MAX_LBR_DEPTH             32              /* Intel LBR hardware branch depth   */
#define SNAPSHOT_MAX_INPUT_SIZE            (1024ULL * 1024ULL) /* 1 MB maximum testcase buffer  */
#define SNAPSHOT_XSAVE_AREA_SIZE           4096ULL         /* FPU/AVX/AVX-512 state save buffer */

//
// Fuzzing Execution Status Codes
//
#define FUZZ_STATUS_SUCCESS                0x00000000
#define FUZZ_STATUS_CRASH_EXCEPTION        0x00000001
#define FUZZ_STATUS_TIMEOUT_EXCEEDED       0x00000002
#define FUZZ_STATUS_MAX_BRANCHES_HIT       0x00000003
#define FUZZ_STATUS_INVALID_STATE          0x00000004
#define FUZZ_STATUS_RESTORE_FAILED         0x00000005

//
// Snapshot Target Execution Modes
//
#define SNAPSHOT_MODE_KERNEL_SUPERVISOR    0x00000001
#define SNAPSHOT_MODE_USER_APPLICATION     0x00000002
#define SNAPSHOT_MODE_HYBRID_TRANSITION    0x00000003

//
// Coverage Instrumentation Modes
//
#define COVERAGE_MODE_INTEL_PT_TOPA        0x00000001
#define COVERAGE_MODE_EPT_PML1_BREAKPOINTS 0x00000002
#define COVERAGE_MODE_LBR_EDGE_RECORDING   0x00000004

//
// Anti-Tamper & Evasion Modes (TSC Time Dilation & Anti-Cheat Stealth)
//
#define EVASION_MODE_NONE                     0x00000000
#define EVASION_MODE_FIXED_DELTA              0x00000001
#define EVASION_MODE_INSTRUCTION_DILATION     0x00000002
#define EVASION_MODE_MSR_LATENCY_COMPENSATION 0x00000004
#define EVASION_MODE_EPT_TIMING_SMOOTHING     0x00000008
#define EVASION_MODE_KTRAP_FRAME_CLEANSE      0x00000010

//////////////////////////////////////////////////
//                 IOCTL Codes                  //
//////////////////////////////////////////////////

#ifndef IOCTL_FUZZER_BASE
#    define IOCTL_FUZZER_BASE                  (IOCTL_START_CODE + 0x400)
#endif

#ifndef IOCTL_SNAPSHOT_TAKE
/**
 * @brief IOCTL to create an in-memory execution snapshot at current state.
 */
#define IOCTL_SNAPSHOT_TAKE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief IOCTL to roll back system state to the baseline snapshot.
 */
#define IOCTL_SNAPSHOT_RESTORE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief IOCTL to clear and deallocate snapshot buffers.
 */
#define IOCTL_SNAPSHOT_CLEAR \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x03, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief IOCTL to inject testcase input and execute a single fuzzing iteration.
 */
#define IOCTL_FUZZ_ITERATE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x04, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief IOCTL to map the shared AFL 64KB coverage bitmap into user-mode address space.
 */
#define IOCTL_FUZZ_MAP_COVERAGE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x05, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief IOCTL to retrieve the last crash report (registers, stack, LBR trace, exception code).
 */
#define IOCTL_FUZZ_GET_CRASH_REPORT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x06, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief IOCTL to execute an autonomous batch of fuzzing iterations directly in-kernel.
 */
#define IOCTL_FUZZ_RUN_BATCH \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x07, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief IOCTL to clear the last captured crash telemetry report.
 */
#define IOCTL_FUZZ_CLEAR_CRASH_REPORT \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x08, METHOD_BUFFERED, FILE_ANY_ACCESS)

/**
 * @brief IOCTL to fetch the raw Intel PT ToPA packet stream buffer.
 */
#define IOCTL_FUZZ_GET_PT_STREAM \
    CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x09, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif



//////////////////////////////////////////////////
//             Hardware State Structures        //
//////////////////////////////////////////////////

/**
 * @brief Architectural segment descriptor hardware state
 */
typedef struct _SNAPSHOT_SEGMENT_DESCRIPTOR
{
    UINT16 Selector;
    UINT16 Attributes;
    UINT32 Limit;
    UINT64 Base;

} SNAPSHOT_SEGMENT_DESCRIPTOR, *PSNAPSHOT_SEGMENT_DESCRIPTOR;

/**
 * @brief Table register state (GDTR / IDTR)
 */
typedef struct _SNAPSHOT_DESCRIPTOR_TABLE
{
    UINT16 Limit;
    UINT64 Base;

} SNAPSHOT_DESCRIPTOR_TABLE, *PSNAPSHOT_DESCRIPTOR_TABLE;

/**
 * @brief Complete vCPU Architectural Snapshot
 */
typedef struct _SNAPSHOT_VCPU_CONTEXT
{
    //
    // General Purpose Registers (GPRs)
    //
    UINT64 Rax;
    UINT64 Rcx;
    UINT64 Rdx;
    UINT64 Rbx;
    UINT64 Rsp;
    UINT64 Rbp;
    UINT64 Rsi;
    UINT64 Rdi;
    UINT64 R8;
    UINT64 R9;
    UINT64 R10;
    UINT64 R11;
    UINT64 R12;
    UINT64 R13;
    UINT64 R14;
    UINT64 R15;
    UINT64 Rip;
    UINT64 Rflags;

    //
    // Control Registers
    //
    UINT64 Cr0;
    UINT64 Cr2;
    UINT64 Cr3;
    UINT64 Cr4;
    UINT64 Cr8;

    //
    // Debug Registers
    //
    UINT64 Dr0;
    UINT64 Dr1;
    UINT64 Dr2;
    UINT64 Dr3;
    UINT64 Dr6;
    UINT64 Dr7;

    //
    // Segment Registers & Tables
    //
    SNAPSHOT_SEGMENT_DESCRIPTOR Cs;
    SNAPSHOT_SEGMENT_DESCRIPTOR Ss;
    SNAPSHOT_SEGMENT_DESCRIPTOR Ds;
    SNAPSHOT_SEGMENT_DESCRIPTOR Es;
    SNAPSHOT_SEGMENT_DESCRIPTOR Fs;
    SNAPSHOT_SEGMENT_DESCRIPTOR Gs;
    SNAPSHOT_SEGMENT_DESCRIPTOR Tr;
    SNAPSHOT_SEGMENT_DESCRIPTOR Ldtr;
    SNAPSHOT_DESCRIPTOR_TABLE   Gdtr;
    SNAPSHOT_DESCRIPTOR_TABLE   Idtr;

    //
    // Essential Model-Specific Registers (MSRs)
    //
    UINT64 MsrEfer;
    UINT64 MsrStar;
    UINT64 MsrLstar;
    UINT64 MsrCstar;
    UINT64 MsrSysenterCs;
    UINT64 MsrSysenterEsp;
    UINT64 MsrSysenterEip;
    UINT64 MsrFsBase;
    UINT64 MsrGsBase;
    UINT64 MsrKernelGsBase;
    UINT64 MsrTsc;
    UINT64 TscOffset;

    //
    // Reserved padding to ensure strict 64-byte hardware alignment for XSAVE/XRSTOR
    // (Total offset prior to padding: 488 bytes; 488 + 24 = 512 bytes = 8 * 64)
    //
    UINT8 ReservedAlignmentPadding[24];

    //
    // Extended State (AVX/SSE/FPU) - Strictly 64-byte aligned
    //
    DECLSPEC_ALIGN(64) UINT8 XsaveArea[SNAPSHOT_XSAVE_AREA_SIZE];

} SNAPSHOT_VCPU_CONTEXT, *PSNAPSHOT_VCPU_CONTEXT;

//////////////////////////////////////////////////
//         EPT Dirty Tracking Structures        //
//////////////////////////////////////////////////

/**
 * @brief Entry for tracking modified physical frames
 */
typedef struct _SNAPSHOT_DIRTY_PAGE_ENTRY
{
    UINT64 PhysicalAddress;   /* GPA of the modified page frame       */
    PVOID  PristineShadowVa;  /* Kernel VA of original 4KB frame copy  */
    PVOID  Pml1EntryVa;       /* Pointer to EPT PML1 entry descriptor */
    UINT64 OriginalPml1Value; /* Authentic PML1 configuration bits    */

} SNAPSHOT_DIRTY_PAGE_ENTRY, *PSNAPSHOT_DIRTY_PAGE_ENTRY;

/**
 * @brief Memory tracker managing snapshot page rollbacks
 */
typedef struct _SNAPSHOT_MEMORY_TRACKER
{
    UINT32                     DirtyCount;
    UINT32                     MaxCapacity;
    BOOLEAN                    AdBitsEnabled;
    SNAPSHOT_DIRTY_PAGE_ENTRY  DirtyPages[SNAPSHOT_MAX_DIRTY_PAGES];

} SNAPSHOT_MEMORY_TRACKER, *PSNAPSHOT_MEMORY_TRACKER;

//////////////////////////////////////////////////
//           Coverage & Crash Structures        //
//////////////////////////////////////////////////

/**
 * @brief Shared 64KB AFL-compatible coverage bitmap layout
 */
typedef struct _FUZZ_AFL_COVERAGE_MAP
{
    UINT8  TraceBits[SNAPSHOT_AFL_MAP_SIZE];
    UINT64 PreviousIp;
    UINT64 TotalEdgesCovered;
    UINT64 TotalExecutions;

} FUZZ_AFL_COVERAGE_MAP, *PFUZZ_AFL_COVERAGE_MAP;

/**
 * @brief Crash & Exception Telemetry Report
 */
typedef struct _FUZZ_CRASH_REPORT
{
    UINT32                ExceptionVector;      /* Vector (e.g. 13 = #GP, 14 = #PF, 6 = #UD) */
    UINT32                HardwareErrorCode;    /* Hardware error code pushed by CPU         */
    UINT64                FaultingAddress;      /* CR2 for #PF or memory operand             */
    UINT64                FaultingRip;          /* Instruction pointer where crash occurred  */
    SNAPSHOT_VCPU_CONTEXT RegistersAtCrash;     /* CPU register dump at crash timestamp      */
    UINT32                LbrEntryCount;        /* Number of valid LBR branches captured     */
    LBR_ENTRY             LbrStack[SNAPSHOT_MAX_LBR_DEPTH]; /* Hardware branch ring trace     */
    UINT64                CrashHash;            /* Unique callstack & fault location hash    */

} FUZZ_CRASH_REPORT, *PFUZZ_CRASH_REPORT;

//////////////////////////////////////////////////
//             IOCTL Request Packets            //
//////////////////////////////////////////////////

/**
 * @brief Request packet for IOCTL_SNAPSHOT_TAKE
 */
typedef struct _DEBUGGER_SNAPSHOT_TAKE_REQUEST
{
    UINT32 TargetProcessId;         /* PID to isolate snapshot to (0 for kernel) */
    UINT32 ExecutionMode;           /* SNAPSHOT_MODE_*                           */
    UINT64 SnapshotRip;             /* RIP where snapshot was initialized        */
    UINT32 KernelStatus;            /* Status returned by driver                 */

} DEBUGGER_SNAPSHOT_TAKE_REQUEST, *PDEBUGGER_SNAPSHOT_TAKE_REQUEST;

/**
 * @brief Request packet for IOCTL_SNAPSHOT_RESTORE
 */
typedef struct _DEBUGGER_SNAPSHOT_RESTORE_REQUEST
{
    UINT32 RestoredPageCount;       /* Number of dirty pages rolled back         */
    UINT64 RestoredRip;             /* Restored instruction pointer              */
    UINT32 KernelStatus;            /* Status returned by driver                 */

} DEBUGGER_SNAPSHOT_RESTORE_REQUEST, *PDEBUGGER_SNAPSHOT_RESTORE_REQUEST;

/**
 * @brief Request packet for IOCTL_FUZZ_ITERATE
 */
typedef struct _DEBUGGER_FUZZ_ITERATE_REQUEST
{
    //
    // Input Mutation Injection
    //
    UINT64 TargetVirtualAddress;    /* In-guest VA to write mutated payload to   */
    UINT32 InputSize;               /* Length of mutated payload in bytes        */
    UINT8  InputBuffer[SNAPSHOT_MAX_INPUT_SIZE]; /* Raw testcase bytes           */

    //
    // Iteration Limits
    //
    UINT64 MaxInstructionCount;     /* Branch or cycle execution timeout limit   */
    UINT64 ExitBreakpointAddress;   /* Function end VA that triggers loop reset  */

    //
    // Execution Output
    //
    UINT32 ExecutionStatus;         /* FUZZ_STATUS_*                             */
    UINT64 ElapsedCycles;           /* Time taken by iteration                   */
    UINT32 KernelStatus;            /* Driver communication result               */

} DEBUGGER_FUZZ_ITERATE_REQUEST, *PDEBUGGER_FUZZ_ITERATE_REQUEST;

/**
 * @brief Request packet for IOCTL_FUZZ_RUN_BATCH
 */
typedef struct _DEBUGGER_FUZZ_RUN_BATCH_REQUEST
{
    UINT32           IterationCount;
    UINT32           TargetProcessId;
    UINT64           TargetVirtualAddress;
    UINT32           InputSize;
    UINT8            InputBuffer[SNAPSHOT_MAX_INPUT_SIZE];
    UINT32           MutationStrategyFlags;
    UINT32           ExecutedCount;
    UINT32           ExecutionStatus;
    UINT64           ElapsedCycles;
    FUZZ_CRASH_REPORT CrashReport;
    UINT32           KernelStatus;

} DEBUGGER_FUZZ_RUN_BATCH_REQUEST, *PDEBUGGER_FUZZ_RUN_BATCH_REQUEST;

/**
 * @brief Request packet for IOCTL_FUZZ_GET_PT_STREAM
 */
typedef struct _DEBUGGER_FUZZ_GET_PT_STREAM_REQUEST
{
    UINT32 BufferSize;
    UINT32 TransferredBytes;
    UINT8  PacketBuffer[SNAPSHOT_MAX_INPUT_SIZE];
    UINT32 KernelStatus;

} DEBUGGER_FUZZ_GET_PT_STREAM_REQUEST, *PDEBUGGER_FUZZ_GET_PT_STREAM_REQUEST;

