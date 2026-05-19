/**
 * @file Pt.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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
//			    	  Constants	    			//
//////////////////////////////////////////////////

//
// Pool tag for PT contiguous allocations (ASCII "PtHd")
//
#define POOL_TAG_PT 'dHtP'

//////////////////////////////////////////////////
//                  Structures                  //
//////////////////////////////////////////////////

/**
 * @brief Narrow input descriptor for PtFilter.
 *
 *        These are the only fields a caller is allowed to set per-CPU
 *        when reconfiguring an active PT trace. Engine-internal options
 *        (BranchEn, TscEn, MtcEn, CycEn, RetCompression, *Freq, etc.)
 *        stay under the engine's control and are NOT exposed here.
 *
 *        BufferSize == 0 means "keep whatever the per-CPU slot already
 *        has" — pure filter changes don't touch the ToPA / output /
 *        overflow buffers and can run from a DPC.
 */
typedef struct _PT_FILTER_OPTIONS
{
    BOOLEAN       TraceUser;
    BOOLEAN       TraceKernel;
    UINT64        TargetCr3;
    UINT64        BufferSize;
    UINT32        NumAddrRanges;
    PT_ADDR_RANGE AddrRanges[PT_MAX_ADDR_RANGES];

} PT_FILTER_OPTIONS, *PPT_FILTER_OPTIONS;

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
PtSave();

VOID
PtDump();

VOID
PtFlush();

//
// LBR-style filter wrapper, one CPU at a time. Mirrors LbrFilter in shape:
// caller passes a PT_FILTER_OPTIONS describing only the user-tunable bits
// (TraceUser, TraceKernel, TargetCr3, BufferSize, NumAddrRanges, AddrRanges),
// and PtFilter handles the stop / config-update / start sequence on the
// CURRENT CPU. Engine-internal config (BranchEn, TscEn, etc.) is left
// untouched in the per-CPU PT_TRACE_CONFIG.
//
VOID
PtFilter(const PT_FILTER_OPTIONS * Options);

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
