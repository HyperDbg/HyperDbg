/**
 * @file TraceApi-linux.c
 * @brief Linux placeholder stubs for the hypertrace (LBR / Intel PT) entry points.
 *
 * @details hypertrace is DEFERRED, not dropped. It is real cross-platform hardware
 * functionality (Intel LBR + Intel PT) that the Linux module will want — not
 * Windows-only code like hyperevade. It is simply not ported yet: ~4700 lines across
 * lbr/Lbr.c, pt/Pt.c and the api/* files. Its MSR programming already routes through
 * the platform layer (PlatformCpu / PlatformDpc wrappers exist); the genuinely new
 * Linux work is Intel PT's ToPA output buffers (contiguous DMA memory + mapping into
 * the target process, today Mm*ContiguousMemory / MDL calls). LBR is the natural
 * first milestone.
 *
 * Until then, active TUs in the single .ko still reference ~10 of hypertrace's
 * exported entry points (hyperkd's Kd.c/Ioctl.c/Loader.c, hyperhv's HyperEvade.c and
 * script-eval's Functions.c), so this TU provides no-op stubs purely so the .ko
 * links. Only the symbols that are actually referenced are defined here. This is
 * Kbuild-only; there is no Windows counterpart (Windows builds the real hypertrace
 * module).
 *
 * TODO(Linux): port the hypertrace module (LBR save/restore via MSR/VMX controls,
 * then Intel PT ToPA output buffers) and drop this stub.
 */
#include "pch.h"

BOOLEAN
HyperTraceInitCallback(HYPERTRACE_CALLBACKS * HyperTraceCallbacks, BOOLEAN RunningOnHypervisorEnvironment)
{
    UNREFERENCED_PARAMETER(HyperTraceCallbacks);
    UNREFERENCED_PARAMETER(RunningOnHypervisorEnvironment);

    return FALSE;
}

VOID
HyperTraceUninit()
{
}

BOOLEAN
HyperTraceLbrIsSupported(UINT32 * Capacity, BOOLEAN * IsArchLbr)
{
    if (Capacity != NULL)
    {
        *Capacity = 0;
    }

    if (IsArchLbr != NULL)
    {
        *IsArchLbr = FALSE;
    }

    return FALSE;
}

BOOLEAN
HyperTraceLbrCheck()
{
    return FALSE;
}

BOOLEAN
HyperTraceLbrRestore()
{
    return FALSE;
}

BOOLEAN
HyperTraceLbrRestoreByFilter(UINT64 FilterOptions)
{
    UNREFERENCED_PARAMETER(FilterOptions);

    return FALSE;
}

BOOLEAN
HyperTraceLbrQueryStateOfLbrSaveAndLoadVmExitAndEntryControls(UINT32 CoreId)
{
    UNREFERENCED_PARAMETER(CoreId);

    return FALSE;
}

BOOLEAN
HyperTraceLbrSave(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest)
{
    UNREFERENCED_PARAMETER(HyperTraceOperationRequest);

    return FALSE;
}

BOOLEAN
HyperTraceLbrPrint(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest)
{
    UNREFERENCED_PARAMETER(HyperTraceOperationRequest);

    return FALSE;
}

BOOLEAN
HyperTraceLbrPerformDump(HYPERTRACE_LBR_DUMP_PACKETS * LbrDumpRequest)
{
    UNREFERENCED_PARAMETER(LbrDumpRequest);

    return FALSE;
}

BOOLEAN
HyperTraceLbrPerformOperation(HYPERTRACE_LBR_OPERATION_PACKETS * LbrOperationRequest)
{
    UNREFERENCED_PARAMETER(LbrOperationRequest);

    return FALSE;
}

BOOLEAN
HyperTracePtPerformOperation(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest)
{
    UNREFERENCED_PARAMETER(PtOperationRequest);

    return FALSE;
}

BOOLEAN
HyperTracePtMmap(HYPERTRACE_PT_MMAP_PACKETS * Req)
{
    UNREFERENCED_PARAMETER(Req);

    return FALSE;
}
