/**
 * @file Kd.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @brief Routines related to kernel debugging
 * @details 
 * @version 0.1
 * @date 2020-12-20
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief initialize kernel debugger
 * @details this function should be called on vmx non-root
 * 
 * @return VOID 
 */
VOID
KdInitializeKernelDebugger()
{
    //
    // Initialize APIC
    //
    ApicInitialize();

    //
    // Allocate DPC routine
    //
    g_DebuggeeDpc = ExAllocatePoolWithTag(NonPagedPool, sizeof(KDPC), POOLTAG);

    if (g_DebuggeeDpc == NULL)
    {
        LogError("err, allocating dpc holder for debuggee");
        return;
    }

    //
    // Register NMI handler for vmx-root
    //
    g_NmiHandlerForKeDeregisterNmiCallback = KeRegisterNmiCallback(&KdNmiCallback, NULL);

    //
    // Broadcast on all core to cause exit for NMIs
    //
    HvEnableNmiExitingAllCores();

    //
    // Enable vm-exit on Hardware debug exceptions and breakpoints
    // so, intercept #DBs and #BP by changing exception bitmap (one core)
    //
    HvEnableDbAndBpExitingAllCores();

    //
    // Reset pause break requests
    //
    RtlZeroMemory(&g_IgnoreBreaksToDebugger, sizeof(DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT));

    //
    // Initialize list of breakpoints and breakpoint id
    //
    InitializeListHead(&g_BreakpointsListHead);
    g_MaximumBreakpointId = 0;

    //
    // Indicate that kernel debugger is active
    //
    g_KernelDebuggerState = TRUE;
}

/**
 * @brief uninitialize kernel debugger
 *  
 * @details this function should be called on vmx non-root
 *
 * @return VOID 
 */
VOID
KdUninitializeKernelDebugger()
{
    if (g_KernelDebuggerState)
    {
        //
        // Indicate that kernel debugger is not active
        //
        g_KernelDebuggerState = FALSE;

        //
        // Reset pause break requests
        //
        RtlZeroMemory(&g_IgnoreBreaksToDebugger, sizeof(DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT));

        //
        // Remove all active breakpoints
        //
        BreakpointRemoveAllBreakpoints();

        //
        // De-register NMI handler
        //
        KeDeregisterNmiCallback(g_NmiHandlerForKeDeregisterNmiCallback);

        //
        // Broadcast on all core to cause not to exit for NMIs
        //
        HvDisableNmiExitingAllCores();

        //
        // Disable vm-exit on Hardware debug exceptions and breakpoints
        // so, not intercept #DBs and #BP by changing exception bitmap (one core)
        //
        HvDisableDbAndBpExitingAllCores();

        //
        // Free DPC holder
        //
        ExFreePoolWithTag(g_DebuggeeDpc, POOLTAG);

        //
        // Uinitialize APIC related function
        //
        ApicUninitialize();
    }
}

/**
 * @brief For debugging purpose, passes the errors to Windbg
 *
 * @return BOOLEAN
 */
VOID
KdPassErrorsToWindbg()
{
    ULONG CoreCount;

    CoreCount = KeQueryActiveProcessorCount(0);

    for (size_t i = 0; i < CoreCount; i++)
    {
        g_GuestState[i].DebuggingState.PassErrorsToWindbg = TRUE;
    }
}

/**
 * @brief Handles NMIs in kernel-mode
 *
 * @param Context
 * @param Handled
 * @return BOOLEAN
 */
BOOLEAN
KdNmiCallback(PVOID Context, BOOLEAN Handled)
{
    UINT32 CurrentCoreIndex;

    CurrentCoreIndex = KeGetCurrentProcessorNumber();

    //
    // This mechanism tries to solve the problem of receiving NMIs
    // when we're already in vmx-root mode, e.g., when we want to
    // inject NMIs to other cores and those cores are already operating
    // in vmx-root mode; however, this is not the approach to solve the
    // problem. In order to solve this problem, we should create our own
    // host IDT in vmx-root mode (Note that we should set HOST_IDTR_BASE
    // and there is no need to LIMIT as it's fixed at 0xffff for VMX
    // operations).
    // Because we want to use the debugging mechanism of the Windows
    // we use the same IDT with the guest (guest and host IDT is the
    // same), but in the future versions we solve this problem by our
    // own ISR NMI handler in vmx-root mode
    //

    //
    // We should check whether the NMI is in vmx-root mode or not
    // if it's not in vmx-root mode then it's not relate to use
    //
    if (!g_GuestState[CurrentCoreIndex].DebuggingState.WaitingForNmi)
    {
        return Handled;
    }

    //
    // If we're here then it related to us
    // We set a flag to indicate that this core should be halted
    //
    g_GuestState[CurrentCoreIndex].DebuggingState.WaitingForNmi = FALSE;

    //
    // This way of handling has a problem, sometimes the guest is not made the guest
    // registers available and in those cases we pass null to the debugger, but in order
    // to avoid complexity we handle it this way
    //
    KdHandleNmi(CurrentCoreIndex, g_GuestState[CurrentCoreIndex].DebuggingState.GuestRegs);

    //
    // Also, return true to show that it's handled
    //
    return TRUE;
}

/**
 * @brief compares the buffer with a string
 *
 * @param CurrentLoopIndex Number of previously read bytes
 * @param Buffer
 * @return BOOLEAN
 */
BOOLEAN
KdCheckForTheEndOfTheBuffer(PUINT32 CurrentLoopIndex, BYTE * Buffer)
{
    UINT32 ActualBufferLength;

    ActualBufferLength = *CurrentLoopIndex;

    //
    // End of buffer is 4 character long
    //
    if (*CurrentLoopIndex <= 3)
    {
        return FALSE;
    }

    if (Buffer[ActualBufferLength] == SERIAL_END_OF_BUFFER_CHAR_4 &&
        Buffer[ActualBufferLength - 1] == SERIAL_END_OF_BUFFER_CHAR_3 &&
        Buffer[ActualBufferLength - 2] == SERIAL_END_OF_BUFFER_CHAR_2 &&
        Buffer[ActualBufferLength - 3] == SERIAL_END_OF_BUFFER_CHAR_1)
    {
        //
        // Clear the end character
        //
        Buffer[ActualBufferLength - 3] = NULL;
        Buffer[ActualBufferLength - 2] = NULL;
        Buffer[ActualBufferLength - 1] = NULL;
        Buffer[ActualBufferLength]     = NULL;

        //
        // Set the new length
        //
        *CurrentLoopIndex = ActualBufferLength - 3;

        return TRUE;
    }
    return FALSE;
}

/**
 * @brief calculate the checksum of recived buffer from debugger
 *
 * @param Buffer
 * @param LengthReceived
 * @return BYTE
 */
BYTE
KdComputeDataChecksum(PVOID Buffer, UINT32 Length)
{
    BYTE CalculatedCheckSum = 0;
    BYTE Temp               = 0;
    while (Length--)
    {
        Temp               = *(BYTE *)Buffer;
        CalculatedCheckSum = CalculatedCheckSum + Temp;
        Buffer             = (PVOID)((UINT64)Buffer + 1);
    }
    return CalculatedCheckSum;
}

/**
 * @brief Sends a HyperDbg response packet to the debugger
 *
 * @param PacketType
 * @param Response
 * @param OptionalBuffer
 * @param OptionalBufferLength
 * @return BOOLEAN
 */
BOOLEAN
KdResponsePacketToDebugger(
    DEBUGGER_REMOTE_PACKET_TYPE             PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION Response,
    CHAR *                                  OptionalBuffer,
    UINT32                                  OptionalBufferLength)
{
    DEBUGGER_REMOTE_PACKET Packet = {0};

    //
    // Make the packet's structure
    //
    Packet.Indicator       = INDICATOR_OF_HYPERDBG_PACKER;
    Packet.TypeOfThePacket = PacketType;

    //
    // Set the requested action
    //
    Packet.RequestedActionOfThePacket = Response;

    //
    // Send the serial packets to the debugger
    //
    if (OptionalBuffer == NULL || OptionalBufferLength == 0)
    {
        Packet.Checksum =
            KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                                  sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

        //
        // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
        // if not we use the windows spinlock
        //
        SpinlockLock(&DebuggerResponseLock);

        SerialConnectionSend((CHAR *)&Packet, sizeof(DEBUGGER_REMOTE_PACKET));

        SpinlockUnlock(&DebuggerResponseLock);
    }
    else
    {
        Packet.Checksum =
            KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                                  sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

        Packet.Checksum += KdComputeDataChecksum((PVOID)OptionalBuffer, OptionalBufferLength);

        //
        // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
        // if not we use the windows spinlock
        //
        SpinlockLock(&DebuggerResponseLock);

        SerialConnectionSendTwoBuffers((CHAR *)&Packet, sizeof(DEBUGGER_REMOTE_PACKET), OptionalBuffer, OptionalBufferLength);

        SpinlockUnlock(&DebuggerResponseLock);
    }

    if (g_IgnoreBreaksToDebugger.PauseBreaksUntilASpecialMessageSent &&
        g_IgnoreBreaksToDebugger.SpeialEventResponse == Response)
    {
        //
        // Set it to false by zeroing it
        //
        RtlZeroMemory(&g_IgnoreBreaksToDebugger, sizeof(DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT));
    }
    return TRUE;
}

/**
 * @brief Sends a HyperDbg logging response packet to the debugger
 *
 * @param OptionalBuffer
 * @param OptionalBufferLength
 * @param OperationCode
 * @return BOOLEAN
 */
BOOLEAN
KdLoggingResponsePacketToDebugger(
    CHAR * OptionalBuffer,
    UINT32 OptionalBufferLength,
    UINT32 OperationCode)
{
    DEBUGGER_REMOTE_PACKET Packet = {0};

    //
    // Make the packet's structure
    //
    Packet.Indicator       = INDICATOR_OF_HYPERDBG_PACKER;
    Packet.TypeOfThePacket = DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER;

    //
    // Set the requested action
    //
    Packet.RequestedActionOfThePacket = DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_LOGGING_MECHANISM;

    //
    // Calculate checksum
    //
    Packet.Checksum =
        KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                              sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

    Packet.Checksum += KdComputeDataChecksum((PVOID)&OperationCode, sizeof(UINT32));
    Packet.Checksum += KdComputeDataChecksum((PVOID)OptionalBuffer, OptionalBufferLength);

    //
    // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
    // if not we use the windows spinlock
    //
    SpinlockLock(&DebuggerResponseLock);

    SerialConnectionSendThreeBuffers((CHAR *)&Packet,
                                     sizeof(DEBUGGER_REMOTE_PACKET),
                                     &OperationCode,
                                     sizeof(UINT32),
                                     OptionalBuffer,
                                     OptionalBufferLength);

    SpinlockUnlock(&DebuggerResponseLock);

    return TRUE;
}

/**
 * @brief Receive packet from the debugger
 *
 * @param BufferToSave
 * @param LengthReceived
 *
 * @return BOOLEAN
 */
BOOLEAN
KdRecvBuffer(CHAR *   BufferToSave,
             UINT32 * LengthReceived)
{
    UINT32 Loop = 0;

    //
    // Read data and store in a buffer
    //
    while (TRUE)
    {
        UCHAR RecvChar = NULL;

        if (!KdHyperDbgRecvByte(&RecvChar))
        {
            continue;
        }

        //
        // We already now that the maximum packet size is MaxSerialPacketSize
        // Check to make sure that we don't pass the boundaries
        //
        if (!(MaxSerialPacketSize > Loop))
        {
            //
            // Invalid buffer (size of buffer exceeds the limitation)
            //
            LogError("err, a buffer received in debuggee which exceeds the buffer limitation");
            return FALSE;
        }

        BufferToSave[Loop] = RecvChar;

        if (KdCheckForTheEndOfTheBuffer(&Loop, (BYTE *)BufferToSave))
        {
            break;
        }

        Loop++;
    }

    //
    // Set the length
    //
    *LengthReceived = Loop;

    return TRUE;
}

/**
 * @brief Handles debug events when kernel-debugger is attached
 * 
 * @param CurrentProcessorIndex
 * @param GuestRegs
 * 
 * @return VOID 
 */
VOID
KdHandleDebugEventsWhenKernelDebuggerIsAttached(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs)
{
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag    = {0};
    RFLAGS                           Rflags           = {0};
    BOOLEAN                          IgnoreDebugEvent = FALSE;
    BOOLEAN                          AvoidUnsetMtf;

    //
    // It's a breakpoint and should be handled by the kernel debugger
    //
    ContextAndTag.Context = g_GuestState[CurrentProcessorIndex].LastVmexitRip;

    if (g_WaitForStepTrap)
    {
        //
        // *** Handle a regular step
        //

        //
        // Unset to show that we're no longer looking for a trap
        //
        g_WaitForStepTrap = FALSE;

        //
        // Check if we should disable RFLAGS.TF in this core or not
        //
        if (g_GuestState[CurrentProcessorIndex].DebuggingState.DisableTrapFlagOnContinue)
        {
            __vmx_vmread(GUEST_RFLAGS, &Rflags);

            Rflags.TrapFlag = FALSE;

            __vmx_vmwrite(GUEST_RFLAGS, Rflags.Value);

            g_GuestState[CurrentProcessorIndex].DebuggingState.DisableTrapFlagOnContinue = FALSE;
        }

        //
        // Check and handle if there is a software defined breakpoint
        //
        if (!BreakpointCheckAndHandleDebuggerDefinedBreakpoints(CurrentProcessorIndex,
                                                                g_GuestState[CurrentProcessorIndex].LastVmexitRip,
                                                                DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
                                                                GuestRegs,
                                                                &AvoidUnsetMtf))
        {
            if (g_HardwareDebugRegisterDetailsForStepOver.Address != NULL)
            {
                //
                // Check if it's caused by a step-over hardware debug breakpoint or not
                //
                if (g_GuestState[CurrentProcessorIndex].LastVmexitRip == g_HardwareDebugRegisterDetailsForStepOver.Address)
                {
                    if (g_HardwareDebugRegisterDetailsForStepOver.ProcessId == PsGetCurrentProcessId() &&
                        g_HardwareDebugRegisterDetailsForStepOver.ThreadId == PsGetCurrentThreadId())
                    {
                        //
                        // It's a step caused by a debug register breakpoint step-over
                        //
                        RtlZeroMemory(&g_HardwareDebugRegisterDetailsForStepOver, sizeof(HARDWARE_DEBUG_REGISTER_DETAILS));
                    }
                    else
                    {
                        //
                        // Should be ignored because it's a hardware debug register that is
                        // likely triggered by other thread
                        //
                        IgnoreDebugEvent = TRUE;

                        //
                        // Also, we should re-apply the hardware debug breakpoint on this thread
                        //
                        SteppingsSetDebugRegister(0,
                                                  BREAK_ON_INSTRUCTION_FETCH,
                                                  FALSE,
                                                  g_HardwareDebugRegisterDetailsForStepOver.Address);
                    }
                }
            }

            if (!IgnoreDebugEvent)
            {
                //
                // Handle a regular step
                //
                ContextAndTag.Context = g_GuestState[CurrentProcessorIndex].LastVmexitRip;
                KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex,
                                                      GuestRegs,
                                                      DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
                                                      &ContextAndTag);
            }
        }
    }
    else
    {
        //
        // It's a regular breakpoint
        //
        KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex,
                                              GuestRegs,
                                              DEBUGGEE_PAUSING_REASON_DEBUGGEE_HARDWARE_DEBUG_REGISTER_HIT,
                                              &ContextAndTag);
    }
}

/**
 * @brief before halting any core, all the tasks will be applied to all
 * cores including the main core
 * @details these tasks will be applied in vmx-root
 * 
 * @param CurrentCore
 * 
 * @return VOID 
 */
VOID
KdApplyTasksPreHaltCore(UINT32 CurrentCore)
{
    //
    // Nothing to do !
    //
}

/**
 * @brief before continue any core, all the tasks will be applied to all
 * cores including the main core
 * @details these tasks will be applied in vmx-root
 * 
 * @param CurrentCore
 * 
 * @return VOID 
 */
VOID
KdApplyTasksPostContinueCore(UINT32 CurrentCore)
{
    //
    // Check to apply hardware debug register breakpoints for step-over
    //
    if (g_GuestState[CurrentCore].DebuggingState.HardwareDebugRegisterForStepping != NULL)
    {
        SteppingsSetDebugRegister(0,
                                  BREAK_ON_INSTRUCTION_FETCH,
                                  FALSE,
                                  g_GuestState[CurrentCore].DebuggingState.HardwareDebugRegisterForStepping);

        g_GuestState[CurrentCore].DebuggingState.HardwareDebugRegisterForStepping = NULL;
    }

    //
    // Check to apply error passing to Windbg (for debugging purpose)
    //
    if (g_GuestState[CurrentCore].DebuggingState.PassErrorsToWindbg)
    {
        //
        // Disable hooking of breakpoint exceptions
        //
        HvUnsetExceptionBitmap(EXCEPTION_VECTOR_BREAKPOINT);
        HvUnsetExceptionBitmap(EXCEPTION_VECTOR_DEBUG_BREAKPOINT);

        g_GuestState[CurrentCore].DebuggingState.PassErrorsToWindbg = FALSE;
    }
}

/**
 * @brief continue the debuggee, this function gurantees that all other cores
 * are continued (except current core)
 * @param CurrentCore
 * @param SpeialEventResponse
 * @param PauseBreaksUntilASpecialMessageSent
 * 
 * @return VOID 
 */
VOID
KdContinueDebuggee(UINT32                                  CurrentCore,
                   BOOLEAN                                 PauseBreaksUntilASpecialMessageSent,
                   DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION SpeialEventResponse)
{
    ULONG CoreCount;

    CoreCount = KeQueryActiveProcessorCount(0);

    if (PauseBreaksUntilASpecialMessageSent)
    {
        g_IgnoreBreaksToDebugger.PauseBreaksUntilASpecialMessageSent = TRUE;
        g_IgnoreBreaksToDebugger.SpeialEventResponse                 = SpeialEventResponse;
    }

    //
    // Check if we should enable interrupts in this core or not,
    // we have another same check in SWITCHING CORES too
    //
    if (g_GuestState[CurrentCore].DebuggingState.EnableExternalInterruptsOnContinue)
    {
        //
        // Check if the debugger has events relating to external-interrupts, if no
        // we completely disable external interrupts
        //
        if (DebuggerEventListCount(&g_Events->ExternalInterruptOccurredEventsHead) == 0)
        {
            //
            // There is no events for external interrupts
            //
            HvSetExternalInterruptExiting(FALSE);
        }

        g_GuestState[CurrentCore].DebuggingState.EnableExternalInterruptsOnContinue = FALSE;
    }

    //
    // Unlock all the cores
    //
    for (size_t i = 0; i < CoreCount; i++)
    {
        SpinlockUnlock(&g_GuestState[i].DebuggingState.Lock);
    }
}

/**
 * @brief continue the debuggee, just the current operating core
 * @param CurrentCore
 * @return VOID 
 */
VOID
KdContinueDebuggeeJustCurrentCore(UINT32 CurrentCore)
{
    //
    // In the case of any halting event, the processor won't send NMIs
    // to other cores if this field is set
    //
    g_GuestState[CurrentCore].DebuggingState.DoNotNmiNotifyOtherCoresByThisCore = TRUE;

    //
    // Unlock the current core
    //
    SpinlockUnlock(&g_GuestState[CurrentCore].DebuggingState.Lock);
}

/**
 * @brief A test function for DPC
 * @param Dpc
 * @param DeferredContext
 * @param SystemArgument1
 * @param SystemArgument2
 * 
 * @return VOID 
 */
VOID
KdDummyDPC(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    LogInfo("I'm here %x\n", DeferredContext);
}

/**
 * @brief Switch to new process
 * @details This function will be called in vmx non-root
 * @param Dpc
 * @param DeferredContext It's the process ID
 * @param SystemArgument1
 * @param SystemArgument2
 * 
 * @return VOID 
 */
VOID
KdSwitchToNewProcessDpc(PKDPC Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    CR3_TYPE         CurrentProcessCr3;
    UINT64           VirtualAddress;
    PHYSICAL_ADDRESS PhysicalAddr;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // Switch to new process's memory layout
    //
    CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayout(DeferredContext);

    //
    // Validate if process id is valid
    //
    if (CurrentProcessCr3.Flags == NULL)
    {
        //
        // Pid is invalid
        //
        LogInfo("err, process id is invalid (unable to switch)");

        //
        // Trigger a breakpoint to be managed by HyperDbg as sign of failure
        //
        DbgBreakPoint();

        return NULL;
    }

    //
    // vm-exit and halt current core with change the process
    //
    AsmVmxVmcall(VMCALL_VM_EXIT_HALT_SYSTEM_AND_CHANGE_CR3, CurrentProcessCr3.Flags, 0, 0);

    //
    // Restore the original process
    //
    RestoreToPreviousProcess(CurrentProcessCr3);
}

/**
 * @brief Add a DPC to dpc queue
 * @param Routine
 * @param Paramter
 * @param ProcessorNumber
 * 
 * @return VOID 
 */
VOID
KdFireDpc(PVOID Routine, PVOID Paramter, UINT32 ProcessorNumber)
{
    KeInitializeDpc(&g_DebuggeeDpc, Routine, Paramter);

    if (ProcessorNumber != DEBUGGER_PROCESSOR_CORE_NOT_IMPORTANT)
    {
        KeSetTargetProcessorDpc(&g_DebuggeeDpc, ProcessorNumber);
    }

    KeInsertQueueDpc(&g_DebuggeeDpc, NULL, NULL);
}

/**
 * @brief change the current process
 * @param PidRequest
 * 
 * @return BOOLEAN 
 */
BOOLEAN
KdSwitchProcess(PDEBUGGEE_CHANGE_PROCESS_PACKET PidRequest)
{
    if (PidRequest->GetRemotePid)
    {
        //
        // Debugger wants to know current pid
        //
        PidRequest->ProcessId = PsGetCurrentProcessId();
    }
    else
    {
        //
        // Debugger wants to switch to new process
        //
        KdFireDpc(KdSwitchToNewProcessDpc,
                  PidRequest->ProcessId,
                  DEBUGGER_PROCESSOR_CORE_NOT_IMPORTANT);
    }

    return TRUE;
}

/**
 * @brief read registers
 * @param Regs
 * @param ReadRegisterRequest
 * 
 * @return BOOLEAN 
 */
BOOLEAN
KdReadRegisters(PGUEST_REGS Regs, PDEBUGGEE_REGISTER_READ_DESCRIPTION ReadRegisterRequest)
{
    GUEST_EXTRA_REGISTERS ERegs = {0};

    if (ReadRegisterRequest->RegisterID == DEBUGGEE_SHOW_ALL_REGISTERS)
    {
        //
        // Add General porpuse registers
        //
        memcpy((void *)((CHAR *)ReadRegisterRequest + sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION)),
               Regs,
               sizeof(GUEST_REGS));

        //
        // Read Extra registers
        //
        ERegs.CS     = DebuggerGetRegValueWrapper(NULL, REGISTER_CS);
        ERegs.SS     = DebuggerGetRegValueWrapper(NULL, REGISTER_SS);
        ERegs.DS     = DebuggerGetRegValueWrapper(NULL, REGISTER_DS);
        ERegs.ES     = DebuggerGetRegValueWrapper(NULL, REGISTER_ES);
        ERegs.FS     = DebuggerGetRegValueWrapper(NULL, REGISTER_FS);
        ERegs.GS     = DebuggerGetRegValueWrapper(NULL, REGISTER_GS);
        ERegs.RFLAGS = DebuggerGetRegValueWrapper(NULL, REGISTER_RFLAGS);
        ERegs.RIP    = DebuggerGetRegValueWrapper(NULL, REGISTER_RIP);

        //
        // copy at the end of ReadRegisterRequest structure
        //
        memcpy((void *)((CHAR *)ReadRegisterRequest + sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION) + sizeof(GUEST_REGS)),
               &ERegs,
               sizeof(GUEST_EXTRA_REGISTERS));
    }
    else
    {
        ReadRegisterRequest->Value = DebuggerGetRegValueWrapper(Regs, ReadRegisterRequest->RegisterID);
    }

    return TRUE;
}

/**
 * @brief read registers
 * @param Regs
 * @param ReadRegisterRequest
 * 
 * @return BOOLEAN 
 */
BOOLEAN
KdReadMemory(PGUEST_REGS Regs, PDEBUGGEE_REGISTER_READ_DESCRIPTION ReadRegisterRequest)
{
    GUEST_EXTRA_REGISTERS ERegs = {0};

    if (ReadRegisterRequest->RegisterID == DEBUGGEE_SHOW_ALL_REGISTERS)
    {
        //
        // Add General porpuse registers
        //
        memcpy((void *)((CHAR *)ReadRegisterRequest + sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION)),
               Regs,
               sizeof(GUEST_REGS));

        //
        // Read Extra registers
        //
        ERegs.CS     = DebuggerGetRegValueWrapper(NULL, REGISTER_CS);
        ERegs.SS     = DebuggerGetRegValueWrapper(NULL, REGISTER_SS);
        ERegs.DS     = DebuggerGetRegValueWrapper(NULL, REGISTER_DS);
        ERegs.ES     = DebuggerGetRegValueWrapper(NULL, REGISTER_ES);
        ERegs.FS     = DebuggerGetRegValueWrapper(NULL, REGISTER_FS);
        ERegs.GS     = DebuggerGetRegValueWrapper(NULL, REGISTER_GS);
        ERegs.RFLAGS = DebuggerGetRegValueWrapper(NULL, REGISTER_RFLAGS);
        ERegs.RIP    = DebuggerGetRegValueWrapper(NULL, REGISTER_RIP);

        //
        // copy at the end of ReadRegisterRequest structure
        //
        memcpy((void *)((CHAR *)ReadRegisterRequest + sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION) + sizeof(GUEST_REGS)),
               &ERegs,
               sizeof(GUEST_EXTRA_REGISTERS));
    }
    else
    {
        ReadRegisterRequest->Value = DebuggerGetRegValueWrapper(Regs, ReadRegisterRequest->RegisterID);
    }

    return TRUE;
}

/**
 * @brief change the current operating core to new core
 * 
 * @param CurrentCore
 * @param NewCore
 * @return BOOLEAN 
 */
BOOLEAN
KdSwitchCore(UINT32 CurrentCore, UINT32 NewCore)
{
    ULONG CoreCount;

    CoreCount = KeQueryActiveProcessorCount(0);

    //
    // Check if core is valid or not
    //
    if (NewCore >= CoreCount)
    {
        //
        // Invalid core count
        //
        return FALSE;
    }

    //
    // *** Core is valid ***
    //

    //
    // Check if we should enable interrupts in this core or not
    //
    if (g_GuestState[CurrentCore].DebuggingState.EnableExternalInterruptsOnContinue)
    {
        //
        // Check if the debugger has events relating to external-interrupts, if no
        // we completely disable external interrupts
        //
        if (DebuggerEventListCount(&g_Events->ExternalInterruptOccurredEventsHead) == 0)
        {
            //
            // There is no events for external interrupts
            //
            HvSetExternalInterruptExiting(FALSE);
        }

        g_GuestState[CurrentCore].DebuggingState.EnableExternalInterruptsOnContinue = FALSE;
    }

    //
    // Unset the current operating core (this is not important as if we
    // return from the operating function then the it will be unset
    // automatically but as we want to not have two operating cores
    // at the same time so we unset it here too)
    //
    g_GuestState[CurrentCore].DebuggingState.CurrentOperatingCore = FALSE;

    //
    // Set new operating core
    //
    g_GuestState[NewCore].DebuggingState.CurrentOperatingCore = TRUE;

    //
    // Unlock the new core
    // *** We should not unlock the spinlock here as the other core might
    // simultaneously start sending packets and corrupt our packets ***
    //
    // SpinlockUnlock(&g_GuestState[NewCore].DebuggingState.Lock);

    return TRUE;
}

/**
 * @brief Notify user-mode to unload the debuggee and close the connections
 * 
 * @return VOID
 */
VOID
KdCloseConnectionAndUnloadDebuggee()
{
    //
    // Send one byte buffer and operation codes
    //
    LogSendBuffer(OPERATION_COMMAND_FROM_DEBUGGER_CLOSE_AND_UNLOAD_VMM,
                  "$",
                  1);
}

/**
 * @brief Notify user-mode to about new user-input buffer
 * @details  
 * @param Buffer
 * @param Len
 * 
 * @return VOID
 */
VOID
KdNotifyDebuggeeForUserInput(CHAR * Buffer, UINT32 Len)
{
    //
    // Send user-input buffer along with operation code to
    // the user-mode
    //
    LogSendBuffer(OPERATION_DEBUGGEE_USER_INPUT,
                  Buffer,
                  Len);
}

/**
 * @brief Notify user-mode to unload the debuggee and close the connections
 * @param Value
 * 
 * @return VOID
 */
VOID
KdSendFormatsFunctionResult(UINT64 Value)
{
    DEBUGGEE_FORMATS_PACKET FormatsPacket = {0};

    FormatsPacket.Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
    FormatsPacket.Value  = Value;

    //
    // Kernel debugger is active, we should send the bytes over serial
    //
    KdResponsePacketToDebugger(
        DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
        DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_FORMATS,
        &FormatsPacket,
        sizeof(DEBUGGEE_FORMATS_PACKET));
}

/**
 * @brief Notify debugger that the execution of command finished
 * 
 * @return VOID
 */
VOID
KdSendCommandFinishedSignal(UINT32      CurrentProcessorIndex,
                            PGUEST_REGS GuestRegs)
{
    //
    // Halt other cores again
    //
    KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex,
                                          GuestRegs,
                                          DEBUGGEE_PAUSING_REASON_DEBUGGEE_COMMAND_EXECUTION_FINISHED,
                                          NULL);
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root 
 * 
 * @return VOID 
 */
VOID
KdHandleBreakpointAndDebugBreakpoints(UINT32                            CurrentProcessorIndex,
                                      PGUEST_REGS                       GuestRegs,
                                      DEBUGGEE_PAUSING_REASON           Reason,
                                      PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails)
{
    //
    // Lock handling breakpoints
    //
    SpinlockLock(&DebuggerHandleBreakpointLock);

    //
    // Check if we should ignore this break request or not
    //
    if (g_IgnoreBreaksToDebugger.PauseBreaksUntilASpecialMessageSent)
    {
        //
        // Unlock the above core
        //
        SpinlockUnlock(&DebuggerHandleBreakpointLock);

        return;
    }

    //
    // Lock current core
    //
    SpinlockLock(&g_GuestState[CurrentProcessorIndex].DebuggingState.Lock);

    //
    // Set the halting reason
    //
    g_DebuggeeHaltReason = Reason;

    //
    // Set the context and tag
    //
    if (EventDetails != NULL)
    {
        g_DebuggeeHaltContext = EventDetails->Context;
        g_DebuggeeHaltTag     = EventDetails->Tag;
    }

    if (g_GuestState[CurrentProcessorIndex].DebuggingState.DoNotNmiNotifyOtherCoresByThisCore == FALSE)
    {
        //
        // Halt all other Core by interrupting them to nmi
        //

        //
        // make sure, nobody is in the middle of sending anything
        //
        SpinlockLock(&DebuggerResponseLock);

        ApicTriggerGenericNmi(CurrentProcessorIndex);

        SpinlockUnlock(&DebuggerResponseLock);
    }
    else
    {
        //
        // Unset to avoid future not notifying events
        //
        g_GuestState[CurrentProcessorIndex].DebuggingState.DoNotNmiNotifyOtherCoresByThisCore = FALSE;
    }

    //
    // All the cores should go and manage through the following function
    //
    KdManageSystemHaltOnVmxRoot(CurrentProcessorIndex, GuestRegs, EventDetails, TRUE);

    //
    // Clear the halting reason
    //
    g_DebuggeeHaltReason = DEBUGGEE_PAUSING_REASON_NOT_PAUSED;

    //
    // Clear the context and tag
    //
    g_DebuggeeHaltContext = NULL;
    g_DebuggeeHaltTag     = NULL;

    //
    // Unlock handling breakpoints
    //
    SpinlockUnlock(&DebuggerHandleBreakpointLock);
}

/**
 * @brief Handle changes to cr3
 * @details This function can be used in vmx-root 
 * @param CurrentProcessorIndex
 * @param GuestRegs
 * @param Reason
 * @param TargetCr3
 * 
 * @return VOID 
 */
VOID
KdChangeCr3AndTriggerBreakpointHandler(UINT32                  CurrentProcessorIndex,
                                       PGUEST_REGS             GuestRegs,
                                       DEBUGGEE_PAUSING_REASON Reason,
                                       CR3_TYPE                TargetCr3)

{
    CR3_TYPE CurrentProcessCr3 = {0};

    //
    // Switch to new process's memory layout, it is because in vmx-root
    // we are in system process layout (PID=4)
    //
    CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayoutByCr3(TargetCr3);

    //
    // Trigger the breakpoint
    //
    KdHandleBreakpointAndDebugBreakpoints(CurrentProcessorIndex, GuestRegs, Reason, NULL);

    //
    // Restore the original process
    //
    RestoreToPreviousProcess(CurrentProcessCr3);
}

/**
 * @brief Handle NMI Vm-exits
 * @param CurrentProcessorIndex
 * @param GuestRegs
 * 
 * @details This function should be called in vmx-root mode
 * @return VOID 
 */
VOID
KdHandleNmi(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs)
{
    /* LogInfo("NMI Arrived on : %d \n", KeGetCurrentProcessorNumber()); */

    //
    // Lock current core
    //
    SpinlockLock(&g_GuestState[CurrentProcessorIndex].DebuggingState.Lock);

    //
    // All the cores should go and manage through the following function
    //
    KdManageSystemHaltOnVmxRoot(CurrentProcessorIndex, GuestRegs, NULL, FALSE);
}

/**
 * @brief apply a guaranteed step one instruction to the debuggee
 * @param CoreId 
 * @return VOID 
 */
VOID
KdGuaranteedStepInstruction(ULONG CoreId)
{
    UINT16 CsSel = 0;

    //
    // Read cs to have a trace of the execution mode of running application
    // in the debuggee
    //
    __vmx_vmread(GUEST_CS_SELECTOR, &CsSel);
    g_GuestState[CoreId].DebuggingState.InstrumentInTrace.CsSel = CsSel;

    //
    // Set an indicator of wait for MTF
    //
    g_GuestState[CoreId].DebuggingState.InstrumentInTrace.WaitForStepOnMtf = TRUE;

    //
    // Not unset again
    //
    g_GuestState[CoreId].IgnoreMtfUnset = TRUE;

    //
    // Change guest interrupt-state
    //
    HvSetExternalInterruptExiting(TRUE);
    g_GuestState[CoreId].DebuggingState.EnableExternalInterruptsOnContinue = TRUE;

    //
    // Set the MTF flag
    //
    HvSetMonitorTrapFlag(TRUE);
}

/**
 * @brief Check if the execution mode (kernel-mode to user-mode or user-mode
 * to kernel-mode) changed
 * 
 * @param PreviousCsSelector 
 * @param CurrentCsSelector
 *
 * @return BOOLEAN 
 */
BOOLEAN
KdCheckGuestOperatingModeChanges(UINT16 PreviousCsSelector, UINT16 CurrentCsSelector)
{
    PreviousCsSelector = PreviousCsSelector & ~3;
    CurrentCsSelector  = CurrentCsSelector & ~3;

    //
    // Check if the execution modes are the same or not
    //
    if (PreviousCsSelector == CurrentCsSelector)
    {
        //
        // Execution modes are not changed
        //
        return FALSE;
    }

    if ((PreviousCsSelector == KGDT64_R3_CODE || PreviousCsSelector == KGDT64_R3_CMCODE) && CurrentCsSelector == KGDT64_R0_CODE)
    {
        //
        // User-mode -> Kernel-mode
        //
        LogInfo("User-mode -> Kernel-mode");
    }
    else if ((CurrentCsSelector == KGDT64_R3_CODE || CurrentCsSelector == KGDT64_R3_CMCODE) && PreviousCsSelector == KGDT64_R0_CODE)
    {
        //
        // Kernel-mode to user-mode
        //
        LogInfo("Kernel-mode -> User-mode");

        //
        // Nothing to do !
        //
    }
    else if ((CurrentCsSelector == KGDT64_R3_CODE && PreviousCsSelector == KGDT64_R3_CMCODE) ||
             (PreviousCsSelector == KGDT64_R3_CODE && CurrentCsSelector == KGDT64_R3_CMCODE))
    {
        //
        // Probably a heaven's gate
        //
        LogInfo("Heaven's gate");

        //
        // Nothing to do !
        //
    }
    else
    {
        LogError("Unknwn changes in cs selectro during the instrument step-in");
    }

    //
    // Execution modes are changed
    //
    return TRUE;
}

/**
 * @brief Regualar step-in | step one instruction to the debuggee
 * @param CoreId 
 * 
 * @return VOID 
 */
VOID
KdRegularStepInInstruction(UINT32 CoreId)
{
    RFLAGS Rflags = {0};

    //
    // We're waiting for an step
    //
    g_WaitForStepTrap = TRUE;

    //
    // Change guest trap flag
    //
    if (!g_GuestState[CoreId].DebuggingState.DisableTrapFlagOnContinue)
    {
        __vmx_vmread(GUEST_RFLAGS, &Rflags);

        if (Rflags.TrapFlag == FALSE)
        {
            Rflags.TrapFlag = TRUE;

            __vmx_vmwrite(GUEST_RFLAGS, Rflags.Value);

            g_GuestState[CoreId].DebuggingState.DisableTrapFlagOnContinue = TRUE;
        }
    }
}

/**
 * @brief Send event registeration buffer to user-mode to register the event
 * @param EventDetailHeader 
 * 
 * @return VOID 
 */
VOID
KdPerformRegisterEvent(PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET EventDetailHeader)
{
    LogSendBuffer(OPERATION_DEBUGGEE_REGISTER_EVENT,
                  ((CHAR *)EventDetailHeader + sizeof(DEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET)),
                  EventDetailHeader->Length);
}

/**
 * @brief Send action buffer to user-mode to be added to the event
 * @param ActionDetailHeader 
 * 
 * @return VOID 
 */
VOID
KdPerformAddActionToEvent(PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET ActionDetailHeader)
{
    LogSendBuffer(OPERATION_DEBUGGEE_ADD_ACTION_TO_EVENT,
                  ((CHAR *)ActionDetailHeader + sizeof(DEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET)),
                  ActionDetailHeader->Length);
}

/**
 * @brief Perform modify and query events
 * @param ModifyAndQueryEvent 
 * 
 * @return VOID 
 */
VOID
KdPerformEventQueryAndModification(PDEBUGGER_MODIFY_EVENTS ModifyAndQueryEvent)
{
    BOOLEAN IsForAllEvents = FALSE;

    //
    // Check if the tag is valid or not
    //
    if (ModifyAndQueryEvent->Tag == DEBUGGER_MODIFY_EVENTS_APPLY_TO_ALL_TAG)
    {
        IsForAllEvents = TRUE;
    }
    else if (!DebuggerIsTagValid(ModifyAndQueryEvent->Tag))
    {
        //
        // Tag is invalid
        //
        ModifyAndQueryEvent->KernelStatus = DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TAG;
        return;
    }

    //
    // ***************************************************************************
    //

    //
    // Check if it's a query state command
    //
    if (ModifyAndQueryEvent->TypeOfAction == DEBUGGER_MODIFY_EVENTS_QUERY_STATE)
    {
        //
        // check if tag is valid or not
        //
        if (!DebuggerIsTagValid(ModifyAndQueryEvent->Tag))
        {
            ModifyAndQueryEvent->KernelStatus = DEBUGEER_ERROR_TAG_NOT_EXISTS;
        }
        else
        {
            //
            // Set event state
            //
            if (DebuggerQueryStateEvent(ModifyAndQueryEvent->Tag))
            {
                ModifyAndQueryEvent->IsEnabled = TRUE;
            }
            else
            {
                ModifyAndQueryEvent->IsEnabled = FALSE;
            }

            //
            // The function was successful
            //
            ModifyAndQueryEvent->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
        }
    }
    else if (ModifyAndQueryEvent->TypeOfAction == DEBUGGER_MODIFY_EVENTS_ENABLE)
    {
        if (IsForAllEvents)
        {
            //
            // Enable all events
            //
            DebuggerEnableOrDisableAllEvents(TRUE);
        }
        else
        {
            //
            // Enable just one event
            //
            DebuggerEnableEvent(ModifyAndQueryEvent->Tag);
        }

        //
        // The function was successful
        //
        ModifyAndQueryEvent->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
    }
    else if (ModifyAndQueryEvent->TypeOfAction == DEBUGGER_MODIFY_EVENTS_DISABLE)
    {
        if (IsForAllEvents)
        {
            //
            // Disable all events
            //
            DebuggerEnableOrDisableAllEvents(FALSE);
        }
        else
        {
            //
            // Disable just one event
            //
            DebuggerDisableEvent(ModifyAndQueryEvent->Tag);
        }

        //
        // The function was successful
        //
        ModifyAndQueryEvent->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
    }
    else if (ModifyAndQueryEvent->TypeOfAction == DEBUGGER_MODIFY_EVENTS_CLEAR)
    {
        //
        // Send one byte buffer and operation codes
        //
        LogSendBuffer(OPERATION_DEBUGGEE_CLEAR_EVENTS,
                      ModifyAndQueryEvent,
                      sizeof(DEBUGGER_MODIFY_EVENTS));
    }
    else
    {
        //
        // Invalid parameter specifed in Action
        //
        ModifyAndQueryEvent->KernelStatus = DEBUGGER_ERROR_MODIFY_EVENTS_INVALID_TYPE_OF_ACTION;
    }
}

/**
 * @brief This function applies commands from the debugger to the debuggee
 * @details when we reach here, we are on the first core
 * @param CurrentCore  
 * @param GuestRegs  
 * 
 * @return VOID 
 */
VOID
KdDispatchAndPerformCommandsFromDebugger(ULONG CurrentCore, PGUEST_REGS GuestRegs)
{
    PDEBUGGEE_CHANGE_CORE_PACKET                        ChangeCorePacket;
    PDEBUGGEE_STEP_PACKET                               SteppingPacket;
    PDEBUGGER_FLUSH_LOGGING_BUFFERS                     FlushPacket;
    PDEBUGGEE_REGISTER_READ_DESCRIPTION                 ReadRegisterPacket;
    PDEBUGGER_READ_MEMORY                               ReadMemoryPacket;
    PDEBUGGER_EDIT_MEMORY                               EditMemoryPacket;
    PDEBUGGEE_CHANGE_PROCESS_PACKET                     ChangeProcessPacket;
    PDEBUGGEE_SCRIPT_PACKET                             ScriptPacket;
    PDEBUGGEE_USER_INPUT_PACKET                         UserInputPacket;
    PDEBUGGEE_BP_PACKET                                 BpPacket;
    PDEBUGGEE_BP_LIST_OR_MODIFY_PACKET                  BpListOrModifyPacket;
    PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET EventRegPacket;
    PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET AddActionPacket;
    PDEBUGGER_MODIFY_EVENTS                             QueryAndModifyEventPacket;
    UINT64                                              NextAddressForHardwareDebugBp = 0;
    UINT32                                              SizeToSend                    = 0;
    ULONG                                               CoreCount;
    BOOLEAN                                             UnlockTheNewCore = FALSE;
    size_t                                              ReturnSize       = 0;

    while (TRUE)
    {
        BOOLEAN                 EscapeFromTheLoop               = FALSE;
        CHAR *                  RecvBuffer[MaxSerialPacketSize] = {0};
        UINT32                  RecvBufferLength                = 0;
        PDEBUGGER_REMOTE_PACKET TheActualPacket =
            (PDEBUGGER_REMOTE_PACKET)RecvBuffer;

        //
        // Receive the buffer in polling mode
        //
        if (!KdRecvBuffer(&RecvBuffer, &RecvBufferLength))
        {
            //
            // Invalid buffer
            //
            continue;
        }

        if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKER)
        {
            //
            // Check checksum
            //
            if (KdComputeDataChecksum((PVOID)&TheActualPacket->Indicator,
                                      RecvBufferLength - sizeof(BYTE)) !=
                TheActualPacket->Checksum)
            {
                LogError("err, checksum is invalid");
                continue;
            }

            //
            // Check if the packet type is correct
            //
            if (TheActualPacket->TypeOfThePacket !=
                DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGER_TO_DEBUGGEE_EXECUTE_ON_VMX_ROOT)
            {
                //
                // sth wrong happened, the packet is not belonging to use
                // nothing to do, just wait again
                //
                LogError("err, unknown packet received from the debugger\n");
            }

            //
            // It's a HyperDbg packet
            //
            switch (TheActualPacket->RequestedActionOfThePacket)
            {
            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CONTINUE:

                //
                // Unlock other cores
                //
                KdContinueDebuggee(CurrentCore, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

                //
                // No need to wait for new commands
                //
                EscapeFromTheLoop = TRUE;

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_STEP:

                SteppingPacket = (DEBUGGEE_STEP_PACKET *)(((CHAR *)TheActualPacket) +
                                                          sizeof(DEBUGGER_REMOTE_PACKET));

                if (SteppingPacket->StepType == DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN_INSTRUMENT)
                {
                    //
                    // Guaranteed step in (i command)
                    //

                    //
                    // Indicate a step
                    //
                    KdGuaranteedStepInstruction(CurrentCore);

                    //
                    // Unlock just on core
                    //
                    KdContinueDebuggeeJustCurrentCore(CurrentCore);

                    //
                    // No need to wait for new commands
                    //
                    EscapeFromTheLoop = TRUE;
                }
                else if (SteppingPacket->StepType == DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN)
                {
                    //
                    // Step in (t command)
                    //

                    //
                    // Indicate a step
                    //
                    KdRegularStepInInstruction(CurrentCore);

                    //
                    // Unlock other cores
                    //
                    KdContinueDebuggee(CurrentCore, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

                    //
                    // Continue to the debuggee
                    //
                    EscapeFromTheLoop = TRUE;
                }
                else if (SteppingPacket->StepType == DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER)
                {
                    //
                    // Step over (p command)
                    //
                    if (SteppingPacket->IsCurrentInstructionACall)
                    {
                        //
                        // It's a call, we should put a hardware debug register breakpoint
                        // on the next instruction
                        //
                        g_WaitForStepTrap             = TRUE;
                        NextAddressForHardwareDebugBp = g_GuestState[CurrentCore].LastVmexitRip + SteppingPacket->CallLength;

                        CoreCount = KeQueryActiveProcessorCount(0);

                        //
                        // Store the detail of the hardware debug register to avoid trigger
                        // in other processes
                        //
                        g_HardwareDebugRegisterDetailsForStepOver.Address   = NextAddressForHardwareDebugBp;
                        g_HardwareDebugRegisterDetailsForStepOver.ProcessId = PsGetCurrentProcessId();
                        g_HardwareDebugRegisterDetailsForStepOver.ThreadId  = PsGetCurrentThreadId();

                        //
                        // Add hardware debug breakpoints on all core on vm-entry
                        //
                        for (size_t i = 0; i < CoreCount; i++)
                        {
                            g_GuestState[CurrentCore].DebuggingState.HardwareDebugRegisterForStepping = NextAddressForHardwareDebugBp;
                        }
                    }
                    else
                    {
                        //
                        // Any instruction other than call (regular step)
                        //
                        KdRegularStepInInstruction(CurrentCore);
                    }

                    //
                    // Unlock other cores
                    //
                    KdContinueDebuggee(CurrentCore, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

                    //
                    // Continue to the debuggee
                    //
                    EscapeFromTheLoop = TRUE;
                }

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CLOSE_AND_UNLOAD_DEBUGGEE:

                //
                // Send the close buffer
                //
                KdCloseConnectionAndUnloadDebuggee();

                //
                // Unlock other cores
                //
                KdContinueDebuggee(CurrentCore, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

                //
                // No need to wait for new commands
                //
                EscapeFromTheLoop = TRUE;

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_CORE:

                ChangeCorePacket = (DEBUGGEE_CHANGE_CORE_PACKET *)(((CHAR *)TheActualPacket) +
                                                                   sizeof(DEBUGGER_REMOTE_PACKET));

                if (CurrentCore != ChangeCorePacket->NewCore)
                {
                    //
                    // Switch to new core
                    //
                    if (KdSwitchCore(CurrentCore, ChangeCorePacket->NewCore))
                    {
                        //
                        // No need to wait for new commands
                        //
                        EscapeFromTheLoop = TRUE;

                        //
                        // Unlock the new core
                        //
                        UnlockTheNewCore = TRUE;

                        ChangeCorePacket->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
                    }
                    else
                    {
                        ChangeCorePacket->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_INVALID_CORE_IN_REMOTE_DEBUGGE;
                    }
                }
                else
                {
                    //
                    // The operating core and the target core is the same, no need for further action
                    //
                    ChangeCorePacket->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
                }

                //
                // Send the result of switching core back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_CORE,
                                           ChangeCorePacket,
                                           sizeof(DEBUGGEE_CHANGE_CORE_PACKET));

                //
                // Because we don't want two cores to send the same packets simultaneously
                //
                if (UnlockTheNewCore)
                {
                    UnlockTheNewCore = FALSE;
                    SpinlockUnlock(&g_GuestState[ChangeCorePacket->NewCore].DebuggingState.Lock);
                }

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_FLUSH_BUFFERS:

                FlushPacket = (DEBUGGER_FLUSH_LOGGING_BUFFERS *)(((CHAR *)TheActualPacket) +
                                                                 sizeof(DEBUGGER_REMOTE_PACKET));
                //
                // Flush the buffers
                //
                DebuggerCommandFlush(FlushPacket);

                //
                // Send the result of flushing back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_FLUSH,
                                           FlushPacket,
                                           sizeof(DEBUGGER_FLUSH_LOGGING_BUFFERS));

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_READ_REGISTERS:

                ReadRegisterPacket = (DEBUGGEE_REGISTER_READ_DESCRIPTION *)(((CHAR *)TheActualPacket) +
                                                                            sizeof(DEBUGGER_REMOTE_PACKET));
                //
                // Read registers
                //
                if (KdReadRegisters(GuestRegs, ReadRegisterPacket))
                {
                    ReadRegisterPacket->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
                }
                else
                {
                    ReadRegisterPacket->KernelStatus = DEBUGGER_ERROR_INVALID_REGISTER_NUMBER;
                }

                if (ReadRegisterPacket->RegisterID == DEBUGGEE_SHOW_ALL_REGISTERS)
                {
                    SizeToSend = sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION) + sizeof(GUEST_REGS) + sizeof(GUEST_EXTRA_REGISTERS);
                }
                else
                {
                    SizeToSend = sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION);
                }
                //
                // Send the result of reading registers back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_READING_REGISTERS,
                                           ReadRegisterPacket,
                                           SizeToSend);

                break;
            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_READ_MEMORY:

                ReadMemoryPacket = (DEBUGGER_READ_MEMORY *)(((CHAR *)TheActualPacket) +
                                                            sizeof(DEBUGGER_REMOTE_PACKET));
                //
                // Read memory
                //
                if (DebuggerCommandReadMemoryVmxRoot(ReadMemoryPacket,
                                                     (PVOID)((UINT64)ReadMemoryPacket + sizeof(DEBUGGER_READ_MEMORY)),
                                                     &ReturnSize))
                {
                    ReadMemoryPacket->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
                }
                else
                {
                    ReadMemoryPacket->KernelStatus = DEBUGEER_ERROR_INVALID_ADDRESS;
                }

                ReadMemoryPacket->ReturnLength = ReturnSize;

                //
                // Send the result of reading memory back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_READING_MEMORY,
                                           (unsigned char *)ReadMemoryPacket,
                                           sizeof(DEBUGGER_READ_MEMORY) + ReturnSize);

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_EDIT_MEMORY:

                EditMemoryPacket = (PDEBUGGER_EDIT_MEMORY)(((CHAR *)TheActualPacket) +
                                                           sizeof(DEBUGGER_REMOTE_PACKET));
                //
                // Edit memory
                //
                if (DebuggerCommandEditMemoryVmxRoot(EditMemoryPacket))
                {
                    EditMemoryPacket->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
                }
                else
                {
                    EditMemoryPacket->KernelStatus = DEBUGEER_ERROR_INVALID_ADDRESS;
                }

                //
                // Send the result of reading memory back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_EDITING_MEMORY,
                                           (unsigned char *)EditMemoryPacket,
                                           sizeof(DEBUGGER_EDIT_MEMORY));

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_PROCESS:

                ChangeProcessPacket = (DEBUGGEE_CHANGE_PROCESS_PACKET *)(((CHAR *)TheActualPacket) +
                                                                         sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Switch to new process
                //
                if (KdSwitchProcess(ChangeProcessPacket))
                {
                    ChangeProcessPacket->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
                }
                else
                {
                    ChangeProcessPacket->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_UNABLE_TO_SWITCH_TO_NEW_PROCESS;
                }

                //
                // Send the result of switching process back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_PROCESS,
                                           ChangeProcessPacket,
                                           sizeof(DEBUGGEE_CHANGE_PROCESS_PACKET));

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_RUN_SCRIPT:

                ScriptPacket = (DEBUGGEE_SCRIPT_PACKET *)(((CHAR *)TheActualPacket) +
                                                          sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Run the script in debuggee
                //
                if (DebuggerPerformRunScript(OPERATION_LOG_INFO_MESSAGE /* simple print */,
                                             NULL,
                                             ScriptPacket,
                                             GuestRegs,
                                             g_DebuggeeHaltContext))
                {
                    //
                    // Set status
                    //
                    ScriptPacket->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
                }
                else
                {
                    //
                    // Set status
                    //
                    ScriptPacket->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_TO_RUN_SCRIPT;
                }

                //
                // Send the result of running script back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_RUNNING_SCRIPT,
                                           ScriptPacket,
                                           sizeof(DEBUGGEE_SCRIPT_PACKET));

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_USER_INPUT_BUFFER:

                UserInputPacket = (DEBUGGEE_USER_INPUT_PACKET *)(((CHAR *)TheActualPacket) +
                                                                 sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Send the user-input to user-mode debuggee
                //
                KdNotifyDebuggeeForUserInput(((CHAR *)UserInputPacket + sizeof(DEBUGGEE_USER_INPUT_PACKET)),
                                             UserInputPacket->CommandLen);

                //
                // Continue Debuggee
                //
                KdContinueDebuggee(CurrentCore, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);
                EscapeFromTheLoop = TRUE;

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_REGISTER_EVENT:

                EventRegPacket = (DEBUGGER_GENERAL_EVENT_DETAIL *)(((CHAR *)TheActualPacket) +
                                                                   sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Send the event buffer to user-mode debuggee
                //
                KdPerformRegisterEvent(EventRegPacket);

                //
                // Continue Debuggee
                //
                KdContinueDebuggee(CurrentCore, TRUE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_REGISTERING_EVENT);
                EscapeFromTheLoop = TRUE;

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_ADD_ACTION_TO_EVENT:

                AddActionPacket = (DEBUGGER_GENERAL_ACTION *)(((CHAR *)TheActualPacket) +
                                                              sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Send the action buffer to user-mode debuggee
                //
                KdPerformAddActionToEvent(AddActionPacket);

                //
                // Continue Debuggee
                //
                KdContinueDebuggee(CurrentCore, TRUE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_ADDING_ACTION_TO_EVENT);
                EscapeFromTheLoop = TRUE;

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_QUERY_AND_MODIFY_EVENT:

                QueryAndModifyEventPacket = (DEBUGGER_MODIFY_EVENTS *)(((CHAR *)TheActualPacket) +
                                                                       sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Perform the action
                //
                KdPerformEventQueryAndModification(QueryAndModifyEventPacket);

                //
                // Only continue debuggee if it's a clear event action
                //
                if (QueryAndModifyEventPacket->TypeOfAction == DEBUGGER_MODIFY_EVENTS_CLEAR)
                {
                    //
                    // Continue Debuggee
                    //
                    KdContinueDebuggee(CurrentCore, TRUE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_QUERY_AND_MODIFY_EVENT);
                    EscapeFromTheLoop = TRUE;
                }
                else
                {
                    //
                    // Send the response of event query and modification (anything other than clear)
                    //
                    KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                               DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_QUERY_AND_MODIFY_EVENT,
                                               QueryAndModifyEventPacket,
                                               sizeof(DEBUGGER_MODIFY_EVENTS));
                }

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_BP:

                BpPacket = (DEBUGGEE_BP_PACKET *)(((CHAR *)TheActualPacket) +
                                                  sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Perform the action
                //
                BreakpointAddNew(BpPacket);

                //
                // Send the result of 'bp' back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_BP,
                                           BpPacket,
                                           sizeof(DEBUGGEE_BP_PACKET));

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_LIST_OR_MODIFY_BREAKPOINTS:

                BpListOrModifyPacket = (DEBUGGEE_BP_LIST_OR_MODIFY_PACKET *)(((CHAR *)TheActualPacket) +
                                                                             sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Perform the action
                //
                BreakpointListOrModify(BpListOrModifyPacket);

                //
                // Send the result of modify or list breakpoints to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_LIST_OR_MODIFY_BREAKPOINTS,
                                           BpListOrModifyPacket,
                                           sizeof(DEBUGGEE_BP_LIST_OR_MODIFY_PACKET));

                break;

            default:
                LogError("err, unknown packet action received from the debugger.\n");
                break;
            }
        }
        else
        {
            //
            // It's not a HyperDbg packet, it's probably a GDB packet
            //
            DbgBreakPoint();
        }

        //
        // If we have to leave the loop, we apply it here
        //
        if (EscapeFromTheLoop)
        {
            break;
        }
    }
}

/**
 * @brief determines if the guest was in 32-bit user-mode or 64-bit (long mode) 
 * @details this function should be called from vmx-root
 * 
 * @return BOOLEAN 
 */
BOOLEAN
KdIsGuestOnUsermode32Bit()
{
    UINT16 CsSel;

    //
    // Read guest's cs selector
    //
    __vmx_vmread(GUEST_CS_SELECTOR, &CsSel);

    if (CsSel == KGDT64_R0_CODE)
    {
        //
        // 64-bit kernel-mode
        //
        return FALSE;
    }
    else if ((CsSel & ~3) == KGDT64_R3_CODE)
    {
        //
        // 64-bit user-mode
        //
        return FALSE;
    }
    else if ((CsSel & ~3) == KGDT64_R3_CMCODE)
    {
        //
        // 32-bit user-mode
        //
        return TRUE;
    }
    else
    {
        LogError("unknown value for cs, cannot determine wow64 mode.");
    }

    //
    // By default, 64-bit
    //
    return FALSE;
}

/**
 * @brief manage system halt on vmx-root mode 
 * @details Thuis function should only be called from KdHandleBreakpointAndDebugBreakpoints
 * @param CurrentCore  
 * @param GuestRegs  
 * @param EventDetails  
 * @param MainCore the core that triggered the event  
 * 
 * @return VOID 
 */
VOID
KdManageSystemHaltOnVmxRoot(ULONG                             CurrentCore,
                            PGUEST_REGS                       GuestRegs,
                            PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails,
                            BOOLEAN                           MainCore)
{
    DEBUGGEE_PAUSED_PACKET PausePacket;
    ULONG                  ExitInstructionLength = 0;
    RFLAGS                 Rflags                = {0};

    //
    // Perform Pre-halt tasks
    //
    KdApplyTasksPreHaltCore(CurrentCore);

StartAgain:

    //
    // We check for receiving buffer (unhalting) only on the
    // first core and not on every cores
    //
    if (MainCore)
    {
        //
        // *** Current Operating Core  ***
        //
        RtlZeroMemory(&PausePacket, sizeof(DEBUGGEE_PAUSED_PACKET));

        //
        // Set as current operating core
        //
        g_GuestState[CurrentCore].DebuggingState.CurrentOperatingCore = TRUE;

        //
        // Set the halt reason
        //
        PausePacket.PausingReason = g_DebuggeeHaltReason;

        //
        // Set the current core
        //
        PausePacket.CurrentCore = CurrentCore;

        //
        // Set the RIP and mode of execution
        //
        PausePacket.Rip            = g_GuestState[CurrentCore].LastVmexitRip;
        PausePacket.Is32BitAddress = KdIsGuestOnUsermode32Bit();

        //
        // Set rflags for finding the results of conditional jumps
        //
        __vmx_vmread(GUEST_RFLAGS, &Rflags);
        PausePacket.Rflags.Value = Rflags.Value;

        //
        // Set the event tag (if it's an event)
        //
        if (EventDetails != NULL)
        {
            PausePacket.EventTag = EventDetails->Tag;
        }

        //
        // Read the instruction len
        //
        if (g_GuestState[CurrentCore].DebuggingState.InstructionLengthHint != 0)
        {
            ExitInstructionLength = g_GuestState[CurrentCore].DebuggingState.InstructionLengthHint;
        }
        else
        {
            __vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstructionLength);
        }

        //
        // Sometime instruction length is 0, e.g., in Trap #DB therefore we
        // use maximum length for an instruction
        //
        if (ExitInstructionLength == 0)
        {
            if (CheckMemoryAccessSafety(g_GuestState[CurrentCore].LastVmexitRip, MAXIMUM_INSTR_SIZE))
            {
                ExitInstructionLength = MAXIMUM_INSTR_SIZE;
            }
        }

        //
        // Find the current instruction
        //
        MemoryMapperReadMemorySafeOnTargetProcess(g_GuestState[CurrentCore].LastVmexitRip,
                                                  &PausePacket.InstructionBytesOnRip,
                                                  ExitInstructionLength);

        //
        // Send the pause packet, along with RIP and an
        // indication to pause to the debugger to the debugger
        //
        KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                   DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_PAUSED_AND_CURRENT_INSTRUCTION,
                                   &PausePacket,
                                   sizeof(DEBUGGEE_PAUSED_PACKET));

        //
        // Perform Commands from the debugger
        //
        KdDispatchAndPerformCommandsFromDebugger(CurrentCore, GuestRegs);

        //
        // Check if it's a change core event or not
        //
        if (!g_GuestState[CurrentCore].DebuggingState.CurrentOperatingCore)
        {
            //
            // Set main core to FALSE
            //
            MainCore = FALSE;
            goto StartAgain;
        }
        else
        {
            //
            // Unset the current operating core
            //
            g_GuestState[CurrentCore].DebuggingState.CurrentOperatingCore = FALSE;
        }
    }
    else
    {
        //
        // All cores except operating core
        //

        //
        // Lock and unlock the lock so all core can get the lock
        // and continue their normal execution
        //
        SpinlockLock(&g_GuestState[CurrentCore].DebuggingState.Lock);

        //
        // Check if it's a change core event or not
        //
        if (g_GuestState[CurrentCore].DebuggingState.CurrentOperatingCore)
        {
            //
            // It's a core change event
            //
            MainCore             = TRUE;
            g_DebuggeeHaltReason = DEBUGGEE_PAUSING_REASON_DEBUGGEE_CORE_SWITCHED;

            goto StartAgain;
        }

        SpinlockUnlock(&g_GuestState[CurrentCore].DebuggingState.Lock);
    }

    //
    // Apply the basic task for the core before continue
    //
    KdApplyTasksPostContinueCore(CurrentCore);
}

/**
 * @brief routines for broadcast system halt 
 * @return VOID 
 */
VOID
KdBroadcastHaltOnAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(BroadcastDpcVmExitAndHaltSystemAllCores, NULL);
}

/**
 * @brief Halt the system
 * @param PausePacket 
 * 
 * @return VOID 
 */
VOID
KdHaltSystem(PDEBUGGER_PAUSE_PACKET_RECEIVED PausePacket)
{
    //
    // Broadcast to halt everything
    // Instead of broadcasting we will just send one vmcall and
    // from that point, we halt all the other cores by NMIs, this
    // way we are sure that we get all the other cores at the middle
    // of their execution codes and not on HyperDbg routines
    //
    /*
    KdBroadcastHaltOnAllCores();
    */

    //
    // vm-exit and halt current core
    //
    AsmVmxVmcall(VMCALL_VM_EXIT_HALT_SYSTEM, 0, 0, 0);

    //
    // Set the status
    //
    PausePacket->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
}
