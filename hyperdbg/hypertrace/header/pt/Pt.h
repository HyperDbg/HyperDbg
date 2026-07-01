/**
 * @file Pt.h
 * @author Masoud Rahimi Jafari (Masoodrahimy1379@gmail.com)
 * @brief Header for Processor Trace (PT) tracing routines for HyperTrace module
 * @details Engine that programs Intel PT MSRs from VMX root or kernel context.
 *          Buffer / ToPA management is kept here; user-visible PT structures
 *          live in the SDK header [PtDefinitions.h].
 * @version 0.19
 * @date 2026-04-29
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#pragma once

//////////////////////////////////////////////////
//                  Structures                  //
//////////////////////////////////////////////////

/**
 * @brief Per-CPU bookkeeping for the user-mode mmap surface.
 *
 *        One MDL + user VA per CPU describes the main output buffer
 *        immediately followed by the 4 KB overflow page as a single
 *        virtually contiguous region in the mapping process. Lives in
 *        g_PtUserMappings; lifetime tied to the PT enable cycle.
 */
typedef struct _PT_USER_MAPPING
{
    PMDL  Mdl;
    PVOID UserVa;

} PT_USER_MAPPING, *PPT_USER_MAPPING;

/**
 * @brief PT apply core filter requests.
 */
typedef struct _PT_APPLY_CORE_FILTER_REQUEST
{
    PT_FILTER_OPTIONS FilterOptions;
    PT_ENABLE_OPTIONS EnableOptions;
    UINT64            BufferSize; /* Output buffer size (0 = default / PT_DEFAULT_BUFFER_SIZE)  */

} PT_APPLY_CORE_FILTER_REQUEST, *PPT_APPLY_CORE_FILTER_REQUEST;

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

//
// HyperDbg-style wrappers (mirroring Lbr*)
//

BOOLEAN
PtCheck();

BOOLEAN
PtStart();

VOID
PtStop();

VOID
PtPause();

VOID
PtResume();

UINT64
PtSize();

VOID
PtDump();

VOID
PtFlush();

//
// LBR-style filter wrapper, one CPU at a time. Mirrors LbrFilter in shape:
// caller passes a PT_APPLY_CORE_FILTER_REQUEST describing only the user-tunable bits
// (TraceUser, TraceKernel, TargetCr3, BufferSize, NumAddrRanges, AddrRanges),
// and PtFilter handles the stop / config-update / start sequence on the
// CURRENT CPU. Engine-internal config (BranchEn, TscEn, etc.) is left
// untouched in the per-CPU PT_TRACE_CONFIG.
//
VOID
PtFilter(const PT_APPLY_CORE_FILTER_REQUEST * FilterRequest);

//
// PASSIVE_LEVEL helpers — call before / after the per-core DPC broadcasts.
// Required because MmAllocateContiguousMemorySpecifyCache and
// MmFreeContiguousMemory must run at IRQL == PASSIVE_LEVEL.
//

BOOLEAN
PtAllocateAllCpuBuffers();

VOID
PtFreeAllCpuBuffers();

//
// User-mode mmap surface: map every per-CPU main output + overflow
// buffer into the calling user process. Idempotent within an enable
// cycle; torn down by PtFreeAllCpuBuffers (i.e. PT disable / flush).
//
INT32
PtMmapAllCpuBuffersToUser(PT_USER_BUFFER_DESC * OutDescs, UINT32 MaxDescs, UINT32 * OutNumCpus);

VOID
PtUnmapAllCpuBuffersFromUser();

//
// Engine routines (operate on a specific PT_PER_CPU instance)
//

INT32
PtEngineQueryCapabilities(PT_CAPABILITIES * OutCaps);

VOID
PtEngineInitDefaultConfig(PT_TRACE_CONFIG * Config);

INT32
PtEngineAllocateBuffers(PT_PER_CPU * Cpu, const PT_TRACE_CONFIG * Config);

VOID
PtEngineFreeBuffers(PT_PER_CPU * Cpu);

INT32
PtEngineStart(PT_PER_CPU * Cpu);

UINT64
PtEngineStop(PT_PER_CPU * Cpu, PT_OUTPUT_BUFFER * Out);

INT32
PtEnginePause(PT_PER_CPU * Cpu);

INT32
PtEngineResume(PT_PER_CPU * Cpu);

UINT64
PtEngineHandlePmi(PT_PER_CPU * Cpu, PT_OUTPUT_BUFFER * Out);

BOOLEAN
PtEngineIsPtPmi();

INT32
PtEngineSizeToTopaEncoding(UINT64 SizeInBytes);
