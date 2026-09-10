/**
 * @file HyperFuzz.h
 * @author HyperDbg Dev Team
 * @brief HyperDbg's SDK module for hypervisor-assisted snapshot fuzzing.
 * @details High-level API contracts, mutation engine hooks, and coverage definitions
 *          for user-mode harnesses, LibAFL integrations, and kernel callbacks.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#include "SDK/headers/BasicTypes.h"
#include "SDK/headers/SnapshotFuzzing.h"

#ifdef __cplusplus
extern "C" {
#endif

//////////////////////////////////////////////////
//              Mutation Types & Enums          //
//////////////////////////////////////////////////

typedef enum _FUZZ_MUTATION_STRATEGY
{
    MUTATION_STRATEGY_BIT_FLIPS           = 0x01,
    MUTATION_STRATEGY_BYTE_FLIPS          = 0x02,
    MUTATION_STRATEGY_ARITHMETIC          = 0x04,
    MUTATION_STRATEGY_INTERESTING_VALUES  = 0x08,
    MUTATION_STRATEGY_BLOCK_DELETION      = 0x10,
    MUTATION_STRATEGY_BLOCK_INSERTION     = 0x20,
    MUTATION_STRATEGY_RANDOM_HAVOC        = 0x40
} FUZZ_MUTATION_STRATEGY;

//////////////////////////////////////////////////
//              Callback Function Types         //
//////////////////////////////////////////////////

/**
 * @brief Custom input mutator callback type
 */
typedef UINT32 (*PFUZZ_CUSTOM_MUTATOR_CALLBACK)(
    PUCHAR Buffer,
    UINT32 CurrentSize,
    UINT32 MaxSize,
    PVOID  UserData
);

/**
 * @brief Crash notification callback type
 */
typedef VOID (*PFUZZ_CRASH_HANDLER_CALLBACK)(
    PFUZZ_CRASH_REPORT CrashReport,
    PVOID              UserData
);

//////////////////////////////////////////////////
//              Fuzzing Session Config          //
//////////////////////////////////////////////////

typedef struct _HYPERFUZZ_SESSION_CONFIG
{
    UINT32                        TargetProcessId;
    UINT64                        TargetEntryAddress;
    UINT64                        ExitBreakpointAddress;
    UINT64                        MaxInstructionCount;
    UINT32                        MutationStrategyFlags;
    PFUZZ_CUSTOM_MUTATOR_CALLBACK CustomMutator;
    PFUZZ_CRASH_HANDLER_CALLBACK  CrashHandler;
    PVOID                         UserData;
} HYPERFUZZ_SESSION_CONFIG, *PHYPERFUZZ_SESSION_CONFIG;

#ifndef IMPORT_EXPORT_LIBHYPERDBG
#    ifdef _WIN32
#        ifdef HYPERDBG_LIBHYPERDBG
#            define IMPORT_EXPORT_LIBHYPERDBG __declspec(dllexport)
#        else
#            define IMPORT_EXPORT_LIBHYPERDBG __declspec(dllimport)
#        endif
#    else
#        ifdef HYPERDBG_LIBHYPERDBG
#            define IMPORT_EXPORT_LIBHYPERDBG __attribute__((visibility("default")))
#        else
#            define IMPORT_EXPORT_LIBHYPERDBG
#        endif
#    endif
#endif

//////////////////////////////////////////////////
//              High-Level SDK APIs             //
//////////////////////////////////////////////////

/**
 * @brief Arms an in-memory execution snapshot at current state.
 */
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
HyperFuzzTakeSnapshot(
    UINT32 TargetProcessId,
    UINT64 EntryAddress
);

/**
 * @brief Restores execution state and dirty pages to pristine baseline.
 */
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
HyperFuzzRestoreSnapshot(VOID);

/**
 * @brief Deallocates snapshot shadow buffers and disarms hardware tracking.
 */
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
HyperFuzzClearSnapshot(VOID);

/**
 * @brief Executes a single mutated testcase iteration.
 */
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
HyperFuzzRunIteration(
    UINT64  TargetVa,
    UINT64  ExitVa,
    PUCHAR  InputBuffer,
    UINT32  InputSize,
    UINT64  MaxInstructions,
    PUINT32 OutStatus,
    PUINT64 OutElapsedCycles
);

/**
 * @brief Retrieves the current 64KB AFL edge coverage map.
 */
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
HyperFuzzGetCoverageMap(
    PFUZZ_AFL_COVERAGE_MAP DestinationMap
);

/**
 * @brief Retrieves details for the most recent crash or exception.
 */
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
HyperFuzzGetCrashReport(
    PFUZZ_CRASH_REPORT DestinationReport
);

/**
 * @brief Resets the last intercepted hardware crash report in the kernel.
 */
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
HyperFuzzClearCrashReport(VOID);

/**
 * @brief Mutates an input buffer using the specified mutation strategy flags.
 */
IMPORT_EXPORT_LIBHYPERDBG UINT32
HyperFuzzMutateBuffer(
    PUCHAR Buffer,
    UINT32 CurrentSize,
    UINT32 MaxSize,
    UINT32 StrategyFlags
);

#define HYPERFUZZ_OPTION_AUTONOMOUS_KERNEL_LOOP   0x00000001
#define HYPERFUZZ_OPTION_PRE_IDT_CRASH_INTERCEPT  0x00000002
#define HYPERFUZZ_OPTION_MONITOR_TRAP_FLAG_RETURN 0x00000004
#define HYPERFUZZ_OPTION_SYNTHETIC_TSC_DILATION   0x00000008

/**
 * @brief Executes an autonomous fuzzing batch within the kernel engine.
 */
IMPORT_EXPORT_LIBHYPERDBG BOOLEAN
HyperFuzzRunBatch(
    UINT32             IterationCount,
    PFUZZ_CRASH_REPORT OutCrashReport,
    PUINT32            OutExecutedCount
);


#ifdef __cplusplus
}
#endif
