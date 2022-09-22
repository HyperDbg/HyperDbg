/**
 * @file Dispatch.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of debugger functions for dispatching, triggering and
 * emulating events
 * @details
 *
 * @version 0.1
 * @date 2022-09-21
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Handling debugger functions related to SYSRET events
 *
 * @param CoreIndex Current core's index
 * @param Regs Guest's gp register
 * @param Context Context of triggering the event
 * @return VOID
 */
VOID
DispatchEventEferSysret(UINT32 CoreIndex, PGUEST_REGS Regs, PVOID Context)
{
    BOOLEAN                               PostEventTriggerReq = FALSE;
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    VIRTUAL_MACHINE_STATE *               CurrentVmState = &g_GuestState[CoreIndex];

    //
    // We should trigger the event of SYSRET here
    //
    EventTriggerResult = DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSRET,
                                               DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                               Regs,
                                               Context,
                                               &PostEventTriggerReq);

    //
    // Check whether we need to ignore event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        SyscallHookEmulateSYSRET(Regs);
        CurrentVmState->IncrementRip = FALSE;
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSRET,
                              DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                              Regs,
                              Context,
                              NULL);
    }
}

/**
 * @brief Handling debugger functions related to SYSCALL events
 *
 * @param CoreIndex Current core's index
 * @param Regs Guest's gp register
 * @param Context Context of triggering the event
 * @return VOID
 */
VOID
DispatchEventEferSyscall(UINT32 CoreIndex, PGUEST_REGS Regs, PVOID Context)
{
    BOOLEAN                               PostEventTriggerReq = FALSE;
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    VIRTUAL_MACHINE_STATE *               CurrentVmState = &g_GuestState[CoreIndex];

    //
    // We should trigger the event of SYSCALL here, we send the
    // syscall number in rax
    //
    EventTriggerResult = DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSCALL,
                                               DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                               Regs,
                                               Regs->rax,
                                               &PostEventTriggerReq);

    //
    // Check whether we need to ignore event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        SyscallHookEmulateSYSCALL(Regs);
        CurrentVmState->IncrementRip = FALSE;
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSCALL,
                              DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                              Regs,
                              Regs->rax,
                              NULL);
    }
}

/**
 * @brief Handling debugger functions related to CPUID events
 *
 * @param Regs Guest's gp register
 * @return VOID
 */
VOID
DispatchEventCpuid(PGUEST_REGS Regs)
{
    UINT64                                Context;
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // Check if attaching is for command dispatching in user debugger
    // or a regular CPUID
    //
    if (g_UserDebuggerState && UdCheckForCommand())
    {
        //
        // It's a thread command for user debugger, no need to run the
        // actual CPUID instruction and change the registers
        //
        return;
    }

    //
    // As the context to event trigger, we send the eax before the cpuid
    // so that the debugger can both read the eax as it's now changed by
    // the cpuid instruction and also can modify the results
    //
    if (g_TriggerEventForCpuids)
    {
        //
        // Adjusting the core context (save eax for the debugger)
        //
        Context = Regs->rax;

        //
        // Triggering the pre-event
        //
        EventTriggerResult = DebuggerTriggerEvents(CPUID_INSTRUCTION_EXECUTION,
                                                   DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                   Regs,
                                                   Context,
                                                   &PostEventTriggerReq);

        //
        // Check whether we need to ignore event emulation or not
        //
        if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
        {
            //
            // Handle the CPUID event in the case of triggering event
            //
            HvHandleCpuid(Regs);
        }

        //
        // Check for the post-event triggering needs
        //
        if (PostEventTriggerReq)
        {
            DebuggerTriggerEvents(CPUID_INSTRUCTION_EXECUTION,
                                  DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                  Regs,
                                  Context,
                                  NULL);
        }
    }
    else
    {
        //
        // Otherwise and if there is no event, we should handle the CPUID
        // normally
        //
        HvHandleCpuid(Regs);
    }
}

/**
 * @brief Handling debugger functions related to RDTSC/RDTSCP events
 *
 * @param Regs Guest's gp register
 * @param ShouldEmulateTsc Whether or not the debugger is allowed to
 * emulate RDTSC/RDTSCP
 * @return VOID
 */
VOID
DispatchEventTsc(PGUEST_REGS Regs, BOOLEAN IsRdtscp)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // As the context to event trigger, we send the false which means
    // it's an rdtsc (for rdtscp we set Context to true)
    // Note : Using !tsc command in transparent-mode is not allowed
    //
    EventTriggerResult = DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION,
                                               DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                               Regs,
                                               IsRdtscp,
                                               &PostEventTriggerReq);

    //
    // Check whether we need to ignore event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Handle rdtsc (emulate rdtsc/p)
        //
        CounterEmulateRdtsc(Regs);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION,
                              DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                              Regs,
                              IsRdtscp,
                              NULL);
    }
}

/**
 * @brief Handling debugger functions related to VMCALL events
 *
 * @param CoreIndex Current core's index
 * @param Regs Guest's gp register
 * @return VOID
 */
VOID
DispatchEventVmcall(UINT32 CoreIndex, PGUEST_REGS Regs)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // As the context to event trigger, we send NULL
    // Registers are the best source to know the purpose
    //
    if (g_TriggerEventForVmcalls)
    {
        //
        // Triggering the pre-event
        //
        EventTriggerResult = DebuggerTriggerEvents(VMCALL_INSTRUCTION_EXECUTION,
                                                   DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                   Regs,
                                                   NULL,
                                                   &PostEventTriggerReq);

        //
        // Check whether we need to ignore event emulation or not
        //
        if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
        {
            //
            // Handle the VMCALL event in the case of triggering event
            //
            VmxHandleVmcallVmExit(CoreIndex, Regs);
        }

        //
        // Check for the post-event triggering needs
        //
        if (PostEventTriggerReq)
        {
            DebuggerTriggerEvents(CPUID_INSTRUCTION_EXECUTION,
                                  DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                  Regs,
                                  NULL,
                                  NULL);
        }
    }
    else
    {
        //
        // Otherwise and if there is no event, we should handle the VMCALL
        // normally
        //
        VmxHandleVmcallVmExit(CoreIndex, Regs);
    }
}

/**
 * @brief Handling debugger functions related to IO events
 *
 * @param Regs Guest's gp register
 * @param Context Context of triggering the event
 * @return VOID
 */
VOID
DispatchEventIO(PGUEST_REGS Regs, PVOID Context)
{
    VMX_EXIT_QUALIFICATION_IO_INSTRUCTION IoQualification     = {0};
    RFLAGS                                Flags               = {0};
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult  = DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_NO_INITIALIZED;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // Read the I/O Qualification which indicates the I/O instruction
    //
    __vmx_vmread(VMCS_EXIT_QUALIFICATION, &IoQualification);

    //
    // Read Guest's RFLAGS
    //
    __vmx_vmread(VMCS_GUEST_RFLAGS, &Flags);

    //
    // As the context to event trigger, port address
    //
    if (IoQualification.DirectionOfAccess == AccessIn)
    {
        EventTriggerResult = DebuggerTriggerEvents(IN_INSTRUCTION_EXECUTION,
                                                   DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                   Regs,
                                                   IoQualification.PortNumber,
                                                   &PostEventTriggerReq);
    }
    else if (IoQualification.DirectionOfAccess == AccessOut)
    {
        EventTriggerResult = DebuggerTriggerEvents(OUT_INSTRUCTION_EXECUTION,
                                                   DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                   Regs,
                                                   IoQualification.PortNumber,
                                                   &PostEventTriggerReq);
    }

    //
    // Check whether we need to ignore event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Call the I/O Handler
        //
        IoHandleIoVmExits(Regs, IoQualification, Flags);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        if (IoQualification.DirectionOfAccess == AccessIn)
        {
            DebuggerTriggerEvents(IN_INSTRUCTION_EXECUTION,
                                  DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                  Regs,
                                  IoQualification.PortNumber,
                                  NULL);
        }
        else if (IoQualification.DirectionOfAccess == AccessOut)
        {
            DebuggerTriggerEvents(OUT_INSTRUCTION_EXECUTION,
                                  DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                  Regs,
                                  IoQualification.PortNumber,
                                  NULL);
        }
    }
}
