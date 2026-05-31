/**
 * @file PtApi.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Tracing routines for HyperTrace module (Intel Processor Trace)
 * @details
 * @version 0.19
 * @date 2026-04-29
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

/**
 * @brief Translate a user-mode HYPERTRACE_PT_OPERATION_PACKETS into a
 *        kernel-side PT_TRACE_CONFIG, falling back to defaults when the
 *        caller left fields zero (e.g. a bare `!pt filter`).
 *
 *        The returned config is value-typed; HyperTracePtFilter's slow
 *        path copies it into every per-CPU slot via
 *        HyperTracePtSeedConfigOnAllCpus.
 * @return VOID
 */
static VOID
HyperTracePtBuildConfig(const HYPERTRACE_PT_OPERATION_PACKETS * Req,
                        PT_TRACE_CONFIG *                       OutCfg)
{
    UINT32 Copy;

    //
    // Start from defaults; only the fields the user can drive are then
    // overridden.
    //
    PtEngineInitDefaultConfig(OutCfg);

    //
    // If the request specifies neither user nor kernel, default to both
    // (matches LBR behaviour when filter is empty).
    //
    if (Req->TraceUser || Req->TraceKernel)
    {
        OutCfg->TraceUser   = (Req->TraceUser != 0) ? TRUE : FALSE;
        OutCfg->TraceKernel = (Req->TraceKernel != 0) ? TRUE : FALSE;
    }

    OutCfg->TargetCr3 = Req->TargetCr3;

    if (Req->BufferSize != 0)
    {
        OutCfg->BufferSize = Req->BufferSize;
    }

    OutCfg->NumAddrRanges = Req->NumAddrRanges;
    if (OutCfg->NumAddrRanges > PT_MAX_ADDR_RANGES)
        OutCfg->NumAddrRanges = PT_MAX_ADDR_RANGES;

    for (Copy = 0; Copy < OutCfg->NumAddrRanges; Copy++)
    {
        OutCfg->AddrRanges[Copy] = Req->AddrRanges[Copy];
    }
}

/**
 * @brief Push a fully-formed PT_TRACE_CONFIG into every active per-CPU
 *        slot. Used by the Enable path so PtAllocateAllCpuBuffers picks up
 *        the right BufferSize on every core. Runs at PASSIVE_LEVEL —
 *        does NOT start or stop tracing (callers handle that via DPC)
 * @return VOID
 */
static VOID
HyperTracePtSeedConfigOnAllCpus(const PT_TRACE_CONFIG * Cfg)
{
    UINT32 ProcessorsCount;
    UINT32 i;

    if (g_PtStateList == NULL || Cfg == NULL)
        return;

    ProcessorsCount = KeQueryActiveProcessorCount(0);

    for (i = 0; i < ProcessorsCount; i++)
    {
        g_PtStateList[i].Config = *Cfg;
    }
}

/**
 * @brief Enable PT tracing for HyperTrace
 *
 * @param PtOperationRequest Pointer to the HyperTrace PT operation request packet
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtEnable(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest)
{
    //
    // Check if PT is already enabled or not
    //
    if (g_ProcessorTraceEnabled)
    {
        PtOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_ENABLED;
        return FALSE;
    }

    //
    // Check PT support on CPU
    //
    if (!PtCheck())
    {
        PtOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_NOT_SUPPORTED;
        return FALSE;
    }

    //
    // Allocate per-CPU ToPA / output / overflow buffers at PASSIVE_LEVEL
    // (the contiguous-memory allocator cannot run from a DPC).
    // PtAllocateAllCpuBuffers reads Cpu->Config.BufferSize, which was set
    // either at module init (default 2MB) or by a prior !pt filter — so
    // any pre-enable filter settings are preserved here.
    //
    if (!PtAllocateAllCpuBuffers())
    {
        //
        // Roll back any partial allocations so we don't leak on next try.
        //
        PtFreeAllCpuBuffers();
        PtOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_NOT_SUPPORTED;
        return FALSE;
    }

    //
    // Broadcast enabling PT on all cores
    //
    BroadcastEnablePtOnAllCores();

    //
    // Set the flag to indicate that PT tracing is enabled
    //
    g_ProcessorTraceEnabled = TRUE;

    //
    // Set successful status
    //
    PtOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    return TRUE;
}

/**
 * @brief Disable PT tracing for HyperTrace
 *
 * @param PtOperationRequest Pointer to the HyperTrace PT operation request packet
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtDisable(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest)
{
    //
    // Check if PT is already disabled or not
    //
    if (!g_ProcessorTraceEnabled)
    {
        if (PtOperationRequest != NULL)
        {
            PtOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_DISABLED;
        }

        return FALSE;
    }

    //
    // Disabling PT
    //
    g_ProcessorTraceEnabled = FALSE;

    //
    // Broadcast disabling PT on all cores (DPC stops tracing per-core)
    //
    BroadcastDisablePtOnAllCores();

    //
    // Free per-CPU buffers at PASSIVE_LEVEL after the broadcast has stopped
    // tracing on every core.
    //
    PtFreeAllCpuBuffers();

    //
    // Set successful status
    //
    if (PtOperationRequest != NULL)
    {
        PtOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}

/**
 * @brief Pause PT tracing on every core. Buffers stay allocated and the
 *        per-CPU CTL is preserved, so HyperTracePtResume can restart the
 *        trace exactly where it stopped.
 *
 * @param HyperTraceOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtPause(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest)
{
    if (!g_ProcessorTraceEnabled)
    {
        if (HyperTraceOperationRequest != NULL)
        {
            HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_DISABLED;
        }
        return FALSE;
    }

    BroadcastPausePtOnAllCores();

    if (HyperTraceOperationRequest != NULL)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}

/**
 * @brief Resume PT tracing on every core after a prior HyperTracePtPause.
 *
 * @param HyperTraceOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtResume(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest)
{
    if (!g_ProcessorTraceEnabled)
    {
        if (HyperTraceOperationRequest != NULL)
        {
            HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_DISABLED;
        }
        return FALSE;
    }

    BroadcastResumePtOnAllCores();

    if (HyperTraceOperationRequest != NULL)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}

/**
 * @brief Snapshot the current PT output position on every core and write
 *        the per-CPU byte counts into HyperTraceOperationRequest->BytesPerCpu.
 *        The returned counts are the decode window — bytes [0, BytesPerCpu[i])
 *        in CPU i's user mapping currently hold valid trace data.
 *
 * @param HyperTraceOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtSize(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest)
{
    UINT32 ProcessorsCount;

    if (!g_ProcessorTraceEnabled)
    {
        if (HyperTraceOperationRequest != NULL)
        {
            HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_DISABLED;
        }
        return FALSE;
    }

    if (HyperTraceOperationRequest == NULL)
        return FALSE;

    ProcessorsCount = KeQueryActiveProcessorCount(0);
    if (ProcessorsCount > PT_MAX_CPUS_FOR_MMAP)
        ProcessorsCount = PT_MAX_CPUS_FOR_MMAP;

    RtlZeroMemory(HyperTraceOperationRequest->BytesPerCpu,
                  sizeof(HyperTraceOperationRequest->BytesPerCpu));

    BroadcastSizePtOnAllCores(HyperTraceOperationRequest->BytesPerCpu);

    HyperTraceOperationRequest->NumCpus      = ProcessorsCount;
    HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    return TRUE;
}

/**
 * @brief Dump PT trace state for HyperTrace
 *
 * @param HyperTraceOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtDump(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest)
{
    if (!g_ProcessorTraceEnabled)
    {
        if (HyperTraceOperationRequest != NULL)
        {
            HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_DISABLED;
        }
        return FALSE;
    }

    LogInfo("Dumping PT trace summary...\n");

    BroadcastDumpPtOnAllCores();

    if (HyperTraceOperationRequest != NULL)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}

/**
 * @brief Flush PT trace state on all cores (free buffers)
 *
 * @param HyperTraceOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtFlush(HYPERTRACE_PT_OPERATION_PACKETS * HyperTraceOperationRequest)
{
    if (!g_ProcessorTraceEnabled)
    {
        if (HyperTraceOperationRequest != NULL)
        {
            HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_DISABLED;
        }
        return FALSE;
    }

    //
    // Stop tracing on all cores via DPC (so MSRs are quiesced)
    //
    BroadcastFlushPtOnAllCores();

    //
    // Then free contiguous buffers at PASSIVE_LEVEL
    //
    PtFreeAllCpuBuffers();

    //
    // Reallocate buffers so subsequent !pt save / !pt dump / !pt enable
    // continue to work — Flush in LBR keeps tracing alive, so we mirror
    // that by leaving PT primed for the next !pt enable.
    //
    if (!PtAllocateAllCpuBuffers())
    {
        PtFreeAllCpuBuffers();
        if (HyperTraceOperationRequest != NULL)
        {
            HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_PT_NOT_SUPPORTED;
        }
        g_ProcessorTraceEnabled = FALSE;
        return FALSE;
    }

    //
    // Resume tracing on all cores
    //
    BroadcastEnablePtOnAllCores();

    if (HyperTraceOperationRequest != NULL)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}

/**
 * @brief Apply a new PT trace configuration (TraceUser / TraceKernel /
 *        TargetCr3 / BufferSize / NumAddrRanges + AddrRanges) on all cores.
 *
 *        Mirrors HyperTraceLbrUpdateFilterOptions / LbrFilter:
 *
 *        - If PT is currently enabled, stop tracing on all cores, free
 *          and reallocate buffers (because BufferSize may have changed),
 *          push the new config into every per-CPU slot, and resume
 *          tracing on all cores.
 *        - If PT is currently disabled, only update the per-CPU config
 *          so that the next `!pt enable` picks it up.
 *
 *        Must be called at IRQL == PASSIVE_LEVEL because of the
 *        contiguous-memory allocator.
 */
BOOLEAN
HyperTracePtFilter(HYPERTRACE_PT_OPERATION_PACKETS * Req)
{
    PT_FILTER_OPTIONS FilterOptions       = {0};
    BOOLEAN           WasEnabled    = g_ProcessorTraceEnabled;
    BOOLEAN           BufferChanged = FALSE;
    UINT64            ExistingSize  = 0;
    UINT32            Copy;

    //
    // Translate the user-mode packet into PT_FILTER_OPTIONS — the narrow
    // surface PtFilter operates on. Default to user+kernel when the
    // caller specified neither (matches LBR's empty-filter behaviour).
    //
    if (Req->TraceUser || Req->TraceKernel)
    {
        FilterOptions.TraceUser   = (Req->TraceUser != 0) ? TRUE : FALSE;
        FilterOptions.TraceKernel = (Req->TraceKernel != 0) ? TRUE : FALSE;
    }
    else
    {
        FilterOptions.TraceUser   = TRUE;
        FilterOptions.TraceKernel = TRUE;
    }
    FilterOptions.TargetCr3     = Req->TargetCr3;
    FilterOptions.BufferSize    = Req->BufferSize;
    FilterOptions.NumAddrRanges = Req->NumAddrRanges;
    if (FilterOptions.NumAddrRanges > PT_MAX_ADDR_RANGES)
        FilterOptions.NumAddrRanges = PT_MAX_ADDR_RANGES;
    for (Copy = 0; Copy < FilterOptions.NumAddrRanges; Copy++)
    {
        FilterOptions.AddrRanges[Copy] = Req->AddrRanges[Copy];
    }

    //
    // Decide between fast (filter-only) and slow (buffer-resize) paths.
    //
    if (g_PtStateList != NULL)
    {
        ExistingSize = g_PtStateList[0].Config.BufferSize;
    }
    if (FilterOptions.BufferSize != 0 && FilterOptions.BufferSize != ExistingSize)
    {
        BufferChanged = TRUE;
    }

    if (!WasEnabled || BufferChanged)
    {
        //
        // Slow path — PT is off and/or BufferSize changed. We have to
        // free + reallocate per-CPU buffers at PASSIVE_LEVEL, so we
        // can't go through DPC for the config seeding either.
        //
        PT_TRACE_CONFIG Cfg;

        if (WasEnabled)
        {
            BroadcastFlushPtOnAllCores();
            PtFreeAllCpuBuffers();
        }

        //
        // Build a full PT_TRACE_CONFIG (defaults + the user-tunable
        // fields from the request) and seed it on every CPU so the next
        // PtAllocateAllCpuBuffers picks up the right BufferSize and
        // PtEngineStart programs the right RTIT_CTL bits.
        //
        HyperTracePtBuildConfig(Req, &Cfg);
        HyperTracePtSeedConfigOnAllCpus(&Cfg);

        if (WasEnabled)
        {
            if (!PtAllocateAllCpuBuffers())
            {
                PtFreeAllCpuBuffers();
                g_ProcessorTraceEnabled = FALSE;
                if (Req != NULL)
                {
                    Req->KernelStatus = DEBUGGER_ERROR_PT_NOT_SUPPORTED;
                }
                return FALSE;
            }

            BroadcastEnablePtOnAllCores();
        }
    }
    else
    {
        //
        // Fast filter-only path: PT is running and BufferSize is
        // unchanged. Force FilterOptions.BufferSize=0 so PtFilter on each core
        // keeps the buffer that's already allocated, then broadcast.
        //
        FilterOptions.BufferSize = 0;
        BroadcastFilterPtOnAllCores(&FilterOptions);
    }

    if (Req != NULL)
    {
        Req->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }

    return TRUE;
}

/**
 * @brief Map every per-CPU PT main output + overflow buffer into the
 *        calling user-mode process. See HYPERTRACE_PT_MMAP_PACKETS for
 *        the full lifetime / single-process contract.
 */
BOOLEAN
HyperTracePtMmap(HYPERTRACE_PT_MMAP_PACKETS * Req)
{
    if (Req == NULL)
        return FALSE;

    Req->NumCpus = 0;

    if (!g_HyperTraceCallbacksInitialized)
    {
        Req->KernelStatus = DEBUGGER_ERROR_HYPERTRACE_NOT_INITIALIZED;
        return FALSE;
    }

    if (!g_ProcessorTraceEnabled)
    {
        Req->KernelStatus = DEBUGGER_ERROR_PT_ALREADY_DISABLED;
        return FALSE;
    }

    if (PtMmapAllCpuBuffersToUser(Req->Cpus, PT_MAX_CPUS_FOR_MMAP, &Req->NumCpus) != 0)
    {
        Req->KernelStatus = DEBUGGER_ERROR_PT_NOT_SUPPORTED;
        return FALSE;
    }

    Req->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    return TRUE;
}

/**
 * @brief Perform actions related to HyperTrace PT
 *
 * @param PtOperationRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePtPerformOperation(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest)
{
    BOOLEAN Status = TRUE;

    //
    // Check if the hypertrace module is initialized before performing any operation
    //
    if (!g_HyperTraceCallbacksInitialized)
    {
        PtOperationRequest->KernelStatus = DEBUGGER_ERROR_HYPERTRACE_NOT_INITIALIZED;
        return FALSE;
    }

    //
    // Perform the requested operation
    //
    switch (PtOperationRequest->PtOperationType)
    {
    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_ENABLE:

        LogInfo("HyperTrace: Enabling PT tracing...\n");

        HyperTracePtEnable(PtOperationRequest);

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE:

        LogInfo("HyperTrace: Disabling PT tracing...\n");

        HyperTracePtDisable(PtOperationRequest);

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_PAUSE:

        LogInfo("HyperTrace: Pausing PT tracing...\n");

        HyperTracePtPause(PtOperationRequest);

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_RESUME:

        LogInfo("HyperTrace: Resuming PT tracing...\n");

        HyperTracePtResume(PtOperationRequest);

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_SIZE:

        LogInfo("HyperTrace: Snapshotting PT buffer sizes...\n");

        HyperTracePtSize(PtOperationRequest);

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DUMP:

        LogInfo("HyperTrace: Showing PT tracing...\n");

        HyperTracePtDump(PtOperationRequest);

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FLUSH:

        LogInfo("HyperTrace: Flushing PT tracing...\n");

        HyperTracePtFlush(PtOperationRequest);

        break;

    case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FILTER:

        LogInfo("HyperTrace: Updating PT filter / config...\n");

        HyperTracePtFilter(PtOperationRequest);

        break;

    default:
        Status                           = FALSE;
        PtOperationRequest->KernelStatus = DEBUGGER_ERROR_INVALID_HYPERTRACE_OPERATION_TYPE;
        break;
    }

    return Status;
}
