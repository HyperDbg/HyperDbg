/**
 * @file HyperEvadeStubs-linux.c
 * @brief Linux-only no-op stubs for the hyperevade (transparency) entry points.
 * @details hyperevade is Windows-only and is left out of the Linux kernel module
 * (ActivateHyperEvadeProject is FALSE; see include/config/Configuration.h and the
 * Kbuild). hyperhv still calls the hyperevade @c Transparent* symbols
 * unconditionally (Hv.c, Vmx.c, MsrHandlers.c, SyscallCallback.c, interface/
 * HyperEvade.c), so this file provides those symbols as no-ops purely so the
 * single .ko links — exactly the behavior of hyperevade's own stub branch when
 * the feature flag is off. There is no Windows counterpart (this TU is never in
 * hyperevade.vcxproj); it exists only for the Linux link, matching the module's
 * existing *-linux.c stub convention (e.g. ZydisKernel-linux.c).
 */
#include "pch.h"

BOOLEAN
TransparentHideDebugger(HYPEREVADE_CALLBACKS *                        HyperevadeCallbacks,
                        DEBUGGER_HIDE_AND_TRANSPARENT_DEBUGGER_MODE * TransparentModeRequest)
{
    UNREFERENCED_PARAMETER(HyperevadeCallbacks);
    UNREFERENCED_PARAMETER(TransparentModeRequest);
    return FALSE;
}

BOOLEAN
TransparentUnhideDebugger()
{
    return FALSE;
}

VOID
TransparentCheckAndModifyCpuid(PGUEST_REGS Regs, INT32 CpuInfo[])
{
    UNREFERENCED_PARAMETER(Regs);
    UNREFERENCED_PARAMETER(CpuInfo);
}

VOID
TransparentCheckAndMitigateVmResumeFootprints(UINT64 * ResumeRIP)
{
    UNREFERENCED_PARAMETER(ResumeRIP);
}

BOOLEAN
TransparentCheckAndModifyMsrRead(PGUEST_REGS Regs, UINT32 TargetMsr)
{
    UNREFERENCED_PARAMETER(Regs);
    UNREFERENCED_PARAMETER(TargetMsr);
    return FALSE;
}

BOOLEAN
TransparentCheckAndModifyMsrWrite(PGUEST_REGS Regs, UINT32 TargetMsr)
{
    UNREFERENCED_PARAMETER(Regs);
    UNREFERENCED_PARAMETER(TargetMsr);
    return FALSE;
}

VOID
TransparentHandleSystemCallHook(GUEST_REGS * Regs)
{
    UNREFERENCED_PARAMETER(Regs);
}

VOID
TransparentCallbackHandleAfterSyscall(GUEST_REGS *                      Regs,
                                      UINT32                            ProcessId,
                                      UINT32                            ThreadId,
                                      UINT64                            Context,
                                      SYSCALL_CALLBACK_CONTEXT_PARAMS * Params)
{
    UNREFERENCED_PARAMETER(Regs);
    UNREFERENCED_PARAMETER(ProcessId);
    UNREFERENCED_PARAMETER(ThreadId);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Params);
}
