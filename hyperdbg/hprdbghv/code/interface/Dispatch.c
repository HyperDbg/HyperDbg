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
DispatchEventEferSysret(VIRTUAL_MACHINE_STATE * VCpu, PVOID Context)
{
    BOOLEAN                               PostEventTriggerReq = FALSE;
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;

    //
    // We should trigger the event of SYSRET here
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSRET,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           Context,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        SyscallHookEmulateSYSRET(VCpu);
        VmFuncSuppressRipIncrement(VCpu->CoreId);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        g_Callbacks.DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSRET,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          Context,
                                          NULL,
                                          VCpu->Regs);
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
DispatchEventEferSyscall(VIRTUAL_MACHINE_STATE * VCpu, PVOID Context)
{
    BOOLEAN                               PostEventTriggerReq = FALSE;
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;

    //
    // We should trigger the event of SYSCALL here, we send the
    // syscall number in rax
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSCALL,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           VCpu->Regs->rax,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        SyscallHookEmulateSYSCALL(VCpu);
        VmFuncSuppressRipIncrement(VCpu->CoreId);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        g_Callbacks.DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSCALL,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          VCpu->Regs->rax,
                                          NULL,
                                          VCpu->Regs);
    }
}

/**
 * @brief Handling debugger functions related to CPUID events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventCpuid(VIRTUAL_MACHINE_STATE * VCpu)
{
    UINT64                                Context;
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // Check if attaching is for command dispatching in user debugger
    // or a regular CPUID
    //
    if (g_Callbacks.UdCheckForCommand())
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
        Context = VCpu->Regs->rax;

        //
        // Triggering the pre-event
        //
        EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(CPUID_INSTRUCTION_EXECUTION,
                                                               DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                               Context,
                                                               &PostEventTriggerReq,
                                                               VCpu->Regs);

        //
        // Check whether we need to short-circuiting event emulation or not
        //
        if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
        {
            //
            // Handle the CPUID event in the case of triggering event
            //
            HvHandleCpuid(VCpu);
        }

        //
        // Check for the post-event triggering needs
        //
        if (PostEventTriggerReq)
        {
            g_Callbacks.DebuggerTriggerEvents(CPUID_INSTRUCTION_EXECUTION,
                                              DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                              Context,
                                              NULL,
                                              VCpu->Regs);
        }
    }
    else
    {
        //
        // Otherwise and if there is no event, we should handle the CPUID
        // normally
        //
        HvHandleCpuid(VCpu);
    }
}

/**
 * @brief Handling debugger functions related to RDTSC/RDTSCP events
 *
 * @param VCpu The virtual processor's state
 * @param IsRdtscp Is a RDTSCP or RDTSC
 * @return VOID
 */
VOID
DispatchEventTsc(VIRTUAL_MACHINE_STATE * VCpu, BOOLEAN IsRdtscp)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // As the context to event trigger, we send the false which means
    // it's an rdtsc (for rdtscp we set Context to true)
    // Note : Using !tsc command in transparent-mode is not allowed
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           IsRdtscp,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Handle rdtsc (emulate rdtsc/p)
        //
        if (IsRdtscp)
        {
            CounterEmulateRdtscp(VCpu);
        }
        else
        {
            CounterEmulateRdtsc(VCpu);
        }
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        g_Callbacks.DebuggerTriggerEvents(TSC_INSTRUCTION_EXECUTION,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          IsRdtscp,
                                          NULL,
                                          VCpu->Regs);
    }
}

/**
 * @brief Handling debugger functions related to VMCALL events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventVmcall(VIRTUAL_MACHINE_STATE * VCpu)
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
        EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(VMCALL_INSTRUCTION_EXECUTION,
                                                               DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                               NULL,
                                                               &PostEventTriggerReq,
                                                               VCpu->Regs);

        //
        // Check whether we need to short-circuiting event emulation or not
        //
        if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
        {
            //
            // Handle the VMCALL event in the case of triggering event
            //
            VmxHandleVmcallVmExit(VCpu);
        }

        //
        // Check for the post-event triggering needs
        //
        if (PostEventTriggerReq)
        {
            g_Callbacks.DebuggerTriggerEvents(CPUID_INSTRUCTION_EXECUTION,
                                              DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                              NULL,
                                              NULL,
                                              VCpu->Regs);
        }
    }
    else
    {
        //
        // Otherwise and if there is no event, we should handle the VMCALL
        // normally
        //
        VmxHandleVmcallVmExit(VCpu);
    }
}

/**
 * @brief Handling debugger functions related to IO events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventIO(VIRTUAL_MACHINE_STATE * VCpu)
{
    VMX_EXIT_QUALIFICATION_IO_INSTRUCTION IoQualification     = {.AsUInt = VCpu->ExitQualification};
    RFLAGS                                Flags               = {0};
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult  = DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_NO_INITIALIZED;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // Read Guest's RFLAGS
    //
    __vmx_vmread(VMCS_GUEST_RFLAGS, &Flags);

    //
    // As the context to event trigger, port address
    //
    if (IoQualification.DirectionOfAccess == AccessIn)
    {
        EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(IN_INSTRUCTION_EXECUTION,
                                                               DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                               IoQualification.PortNumber,
                                                               &PostEventTriggerReq,
                                                               VCpu->Regs);
    }
    else if (IoQualification.DirectionOfAccess == AccessOut)
    {
        EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(OUT_INSTRUCTION_EXECUTION,
                                                               DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                               IoQualification.PortNumber,
                                                               &PostEventTriggerReq,
                                                               VCpu->Regs);
    }

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Call the I/O Handler
        //
        IoHandleIoVmExits(VCpu, IoQualification, Flags);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        if (IoQualification.DirectionOfAccess == AccessIn)
        {
            g_Callbacks.DebuggerTriggerEvents(IN_INSTRUCTION_EXECUTION,
                                              DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                              IoQualification.PortNumber,
                                              NULL,
                                              VCpu->Regs);
        }
        else if (IoQualification.DirectionOfAccess == AccessOut)
        {
            g_Callbacks.DebuggerTriggerEvents(OUT_INSTRUCTION_EXECUTION,
                                              DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                              IoQualification.PortNumber,
                                              NULL,
                                              VCpu->Regs);
        }
    }
}

/**
 * @brief Handling debugger functions related to RDMSR events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventRdmsr(VIRTUAL_MACHINE_STATE * VCpu)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // Triggering the pre-event
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(RDMSR_INSTRUCTION_EXECUTION,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           VCpu->Regs->rcx & 0xffffffff,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Handle vm-exit and perform changes
        //
        MsrHandleRdmsrVmexit(VCpu->Regs);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        g_Callbacks.DebuggerTriggerEvents(RDMSR_INSTRUCTION_EXECUTION,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          VCpu->Regs->rcx & 0xffffffff,
                                          NULL,
                                          VCpu->Regs);
    }
}

/**
 * @brief Handling debugger functions related to WRMSR events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventWrmsr(VIRTUAL_MACHINE_STATE * VCpu)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // Triggering the pre-event
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(WRMSR_INSTRUCTION_EXECUTION,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           VCpu->Regs->rcx & 0xffffffff,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Handle vm-exit and perform changes
        //
        MsrHandleWrmsrVmexit(VCpu->Regs);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        g_Callbacks.DebuggerTriggerEvents(WRMSR_INSTRUCTION_EXECUTION,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          VCpu->Regs->rcx & 0xffffffff,
                                          NULL,
                                          VCpu->Regs);
    }
}

/**
 * @brief Handling debugger functions related to RDPMC events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventRdpmc(VIRTUAL_MACHINE_STATE * VCpu)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // Triggering the pre-event
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(PMC_INSTRUCTION_EXECUTION,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           NULL,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Handle RDPMC (emulate RDPMC)
        //
        CounterEmulateRdpmc(VCpu);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        g_Callbacks.DebuggerTriggerEvents(PMC_INSTRUCTION_EXECUTION,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          NULL,
                                          NULL,
                                          VCpu->Regs);
    }
}

/**
 * @brief Handling debugger functions related to MOV 2 DR events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventMov2DebugRegs(VIRTUAL_MACHINE_STATE * VCpu)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // Handle access to debug registers, if we should not ignore it, it is
    // because on detecting thread scheduling we ignore the hardware debug
    // registers modifications
    //
    if (g_Callbacks.KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId(VCpu->CoreId,
                                                                              DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_DEBUG_REGISTER_INTERCEPTION))
    {
        return;
    }

    //
    // Triggering the pre-event
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(DEBUG_REGISTERS_ACCESSED,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           NULL,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Handle RDPMC (emulate MOV 2 Debug Registers)
        //
        HvHandleMovDebugRegister(VCpu);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        g_Callbacks.DebuggerTriggerEvents(DEBUG_REGISTERS_ACCESSED,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          NULL,
                                          NULL,
                                          VCpu->Regs);
    }
}

/**
 * @brief Handling debugger functions related to mov to/from CR events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventMovToFromControlRegisters(VIRTUAL_MACHINE_STATE * VCpu)
{
    BOOLEAN                               ModifyReg;
    VMX_EXIT_QUALIFICATION_MOV_CR *       CrExitQualification;
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;
    ULONG                                 ExitQualification   = 0;

    //
    // Read the exit qualification
    //
    __vmx_vmread(VMCS_EXIT_QUALIFICATION, &ExitQualification);

    CrExitQualification = (VMX_EXIT_QUALIFICATION_MOV_CR *)&ExitQualification;

    if (CrExitQualification->AccessType == VMX_EXIT_QUALIFICATION_ACCESS_MOV_TO_CR)
    {
        ModifyReg = TRUE;
    }
    else
    {
        ModifyReg = FALSE;
    }

    //
    // Triggering the pre-event
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(ModifyReg ? CONTROL_REGISTER_MODIFIED : CONTROL_REGISTER_READ,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           CrExitQualification->ControlRegister,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Handle mov to/from control registers (emulate CR access)
        //
        HvHandleControlRegisterAccess(VCpu, CrExitQualification);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        g_Callbacks.DebuggerTriggerEvents(ModifyReg ? CONTROL_REGISTER_MODIFIED : CONTROL_REGISTER_READ,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          CrExitQualification->ControlRegister,
                                          NULL,
                                          VCpu->Regs);
    }
}

/**
 * @brief Handling debugger functions related to EXCEPTION events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventException(VIRTUAL_MACHINE_STATE * VCpu)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;
    VMEXIT_INTERRUPT_INFORMATION          InterruptExit       = {0};

    //
    // read the exit interruption information
    //
    __vmx_vmread(VMCS_VMEXIT_INTERRUPTION_INFORMATION, &InterruptExit);

    //
    // This type of vm-exit, can be either because of an !exception event,
    // or it might be because we triggered APIC or X2APIC to generate an
    // NMI, we want to halt the debuggee. We perform the checks here to
    // avoid triggering an event for NMIs when the debuggee requested it
    //
    if (InterruptExit.InterruptionType == INTERRUPT_TYPE_NMI &&
        InterruptExit.Vector == EXCEPTION_VECTOR_NMI)
    {
        //
        // Check if we're waiting for an NMI on this core and if the guest is NOT in
        // a instrument step-in ('i' command) routine
        //
        if (!VCpu->RegisterBreakOnMtf &&
            VmxBroadcastNmiHandler(VCpu, FALSE))
        {
            return;
        }
    }

    //
    // Also, avoid exception when we're running on a register form MTF (instrumentation step-in)
    //
    if (VCpu->RegisterBreakOnMtf)
    {
        //
        // We ignore it because an MTF should handle it as it's an instrumentation step-in
        //
        return;
    }

    //
    // *** When we reached here it means that this is not a NMI cause by guest,
    // probably an event ***
    //

    //
    // Triggering the pre-event
    // As the context to event trigger, we send the vector or IDT Index
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(EXCEPTION_OCCURRED,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           InterruptExit.Vector,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Now, we check if the guest enabled MTF for debugging (instrumentation stepping)
    // This is because based on Intel SDM :
    // If the "monitor trap flag" VM-execution control is 1 and VM entry is
    // injecting a vectored event, an MTF VM exit is pending on the instruction
    // boundary before the first instruction following the VM entry
    // and,
    // If VM entry is injecting a pending MTF VM exit, an MTF VM exit is pending on the
    // instruction boundary before the first instruction following the VM entry
    // This is the case even if the "monitor trap flag" VM-execution control is 0
    //
    // So, we'll ignore the injection of Exception in this case
    //
    if (VCpu->RegisterBreakOnMtf)
    {
        return;
    }

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Handle exception (emulate or inject the event)
        //
        IdtEmulationHandleExceptionAndNmi(VCpu, InterruptExit);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        g_Callbacks.DebuggerTriggerEvents(EXCEPTION_OCCURRED,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          InterruptExit.Vector,
                                          NULL,
                                          VCpu->Regs);
    }
}

/**
 * @brief Handling debugger functions related to external-interrupt events
 *
 * @param VCpu The virtual processor's state
 * @return VOID
 */
VOID
DispatchEventExternalInterrupts(VIRTUAL_MACHINE_STATE * VCpu)
{
    VMEXIT_INTERRUPT_INFORMATION          InterruptExit;
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq = FALSE;

    //
    // read the exit interruption information
    //
    __vmx_vmread(VMCS_VMEXIT_INTERRUPTION_INFORMATION, &InterruptExit);

    //
    // Check for immediate vm-exit mechanism
    //
    if (VCpu->WaitForImmediateVmexit &&
        InterruptExit.Vector == IMMEDIATE_VMEXIT_MECHANISM_VECTOR_FOR_SELF_IPI)
    {
        //
        // Disable vm-exit on external interrupts
        //
        HvSetExternalInterruptExiting(VCpu, FALSE);

        //
        // Not increase the RIP
        //
        VmFuncSuppressRipIncrement(VCpu->CoreId);

        //
        // Hanlde immediate vm-exit mechanism
        //
        VmxMechanismHandleImmediateVmexit(VCpu);

        //
        // No need to continue, it's a HyperDbg mechanism
        //
        return;
    }

    //
    // Check process or thread change detections
    // we cannot ignore injecting the interrupt to the guest if the target interrupt
    // and process or thread proved to cause a system halt. it halts the system as
    // we Windows expects to switch the thread while we're forcing it to not do it
    // Windows fires a clk interrupt on core 0 and fires IPI on other cores
    // to change a thread
    //
    if ((VCpu->CoreId == 0 && InterruptExit.Vector == CLOCK_INTERRUPT) ||
        (VCpu->CoreId != 0 && InterruptExit.Vector == IPI_INTERRUPT))
    {
        g_Callbacks.DebuggerCheckProcessOrThreadChange(VCpu->CoreId);
    }

    //
    // Triggering the pre-event
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(EXTERNAL_INTERRUPT_OCCURRED,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           InterruptExit.Vector,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    //
    // Check whether we need to short-circuiting event emulation or not
    //
    if (EventTriggerResult != DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        //
        // Handle vm-exit and perform changes
        //
        IdtEmulationHandleExternalInterrupt(VCpu, InterruptExit);
    }

    //
    // Check for the post-event triggering needs
    //
    if (PostEventTriggerReq)
    {
        //
        // Trigger the event
        //
        // As the context to event trigger, we send the vector index
        //
        // Keep in mind that interrupt might be inseted in pending list
        // because the guest is not in a interruptible state and will
        // be re-injected when the guest is ready for interrupts
        //
        g_Callbacks.DebuggerTriggerEvents(EXTERNAL_INTERRUPT_OCCURRED,
                                          DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                          InterruptExit.Vector,
                                          NULL,
                                          VCpu->Regs);
    }
}

/**
 * @brief Handling debugger functions related to hidden hook exec
 * CC events
 *
 * @param VCpu The virtual processor's state
 * @param Context The context of the caller
 * @return VOID
 */
VOID
DispatchEventHiddenHookExecCc(VIRTUAL_MACHINE_STATE * VCpu, PVOID Context)
{
    BOOLEAN PostEventTriggerReq = FALSE;

    //
    // Triggering the pre-event (This command only support the
    // pre-event, the post-event doesn't make sense in this command)
    //
    g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_EXEC_CC,
                                      DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                      Context,
                                      &PostEventTriggerReq,
                                      VCpu->Regs); // it will crash if we pass it NULL
}

/**
 * @brief Handling debugger functions related to hidden hook exec
 * detours events
 *
 * @param VCpu The virtual processor's state
 * @param Context The context of the caller
 * @return VOID
 */
VOID
DispatchEventHiddenHookExecDetours(VIRTUAL_MACHINE_STATE * VCpu, PVOID Context)
{
    BOOLEAN PostEventTriggerReq = FALSE;

    //
    // Triggering the pre-event (This command only support the
    // pre-event, the post-event doesn't make sense in this command)
    //
    g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_EXEC_DETOURS,
                                      DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                      Context,
                                      &PostEventTriggerReq,
                                      VCpu->Regs); // it will crash if we pass it NULL
}

/**
 * @brief Handling debugger functions related to read & write, write events (pre)
 *
 * @param VCpu The virtual processor's state
 * @param Context The context of the caller
 * @param IsTriggeringPostEventAllowed Is the caller required to trigger post event
 * @return BOOLEAN
 */
BOOLEAN
DispatchEventHiddenHookPageReadWriteWritePreEvent(VIRTUAL_MACHINE_STATE * VCpu, PVOID Context, BOOLEAN * IsTriggeringPostEventAllowed)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq  = FALSE;
    BOOLEAN                               ShortCircuitingEvent = FALSE;

    //
    // Triggering the pre-event (for the write hooks)
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_WRITE,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           Context,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    if (EventTriggerResult == DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        ShortCircuitingEvent = TRUE;
    }

    if (PostEventTriggerReq)
    {
        *IsTriggeringPostEventAllowed = TRUE;
    }

    //
    // Triggering the pre-event (for the read & write hooks)
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_READ_AND_WRITE,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           Context,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    if (EventTriggerResult == DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        ShortCircuitingEvent = TRUE;
    }

    if (PostEventTriggerReq)
    {
        *IsTriggeringPostEventAllowed = TRUE;
    }

    return ShortCircuitingEvent;
}

/**
 * @brief Handling debugger functions related to read & write, read events (pre)
 *
 * @param VCpu The virtual processor's state
 * @param Context The context of the caller
 * @param IsTriggeringPostEventAllowed
 * @return BOOLEAN
 */
BOOLEAN
DispatchEventHiddenHookPageReadWriteReadPreEvent(VIRTUAL_MACHINE_STATE * VCpu, PVOID Context, BOOLEAN * IsTriggeringPostEventAllowed)
{
    DEBUGGER_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                               PostEventTriggerReq  = FALSE;
    BOOLEAN                               ShortCircuitingEvent = FALSE;

    //
    // Triggering the pre-event (for the read hooks)
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_READ,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           Context,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    if (EventTriggerResult == DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        ShortCircuitingEvent = TRUE;
    }

    if (PostEventTriggerReq)
    {
        *IsTriggeringPostEventAllowed = TRUE;
    }

    //
    // Triggering the pre-event (for the read & write hooks)
    //
    EventTriggerResult = g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_READ_AND_WRITE,
                                                           DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                                           Context,
                                                           &PostEventTriggerReq,
                                                           VCpu->Regs);

    if (EventTriggerResult == DEBUGGER_TRIGGERING_EVENT_STATUS_SUCCESSFUL_IGNORE_EVENT)
    {
        ShortCircuitingEvent = TRUE;
    }

    if (PostEventTriggerReq)
    {
        *IsTriggeringPostEventAllowed = TRUE;
    }

    return ShortCircuitingEvent;
}

/**
 * @brief Handling debugger functions related to read & write, write events (post)
 *
 * @param VCpu The virtual processor's state
 * @param Context The context of the caller
 * @return VOID
 */
VOID
DispatchEventHiddenHookPageReadWriteWritePostEvent(VIRTUAL_MACHINE_STATE * VCpu, PVOID Context)
{
    //
    // Triggering the post-event (for the write hooks)
    //
    g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_WRITE,
                                      DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                      Context,
                                      NULL,
                                      VCpu->Regs);

    //
    // Triggering the post-event (for the read & write hooks)
    //
    g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_READ_AND_WRITE,
                                      DEBUGGER_CALLING_STAGE_POST_EVENT_EMULATION,
                                      Context,
                                      NULL,
                                      VCpu->Regs);
}

/**
 * @brief Handling debugger functions related to read & write, read events (post)
 *
 * @param VCpu The virtual processor's state
 * @param Context The context of the caller
 * @return VOID
 */
VOID
DispatchEventHiddenHookPageReadWriteReadPostEvent(VIRTUAL_MACHINE_STATE * VCpu, PVOID Context)
{
    //
    // Triggering the post-event (for the read hooks)
    //
    g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_READ,
                                      DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                      Context,
                                      NULL,
                                      VCpu->Regs);

    //
    // Triggering the post-event (for the read & write hooks)
    //
    g_Callbacks.DebuggerTriggerEvents(HIDDEN_HOOK_READ_AND_WRITE,
                                      DEBUGGER_CALLING_STAGE_PRE_EVENT_EMULATION,
                                      Context,
                                      NULL,
                                      VCpu->Regs);
}