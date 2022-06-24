/**
 * @file Kd.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author Alee Amini (alee@hyperdbg.org)
 * @brief Routines related to kernel mode debugging
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
    ULONG CoreCount = KeQueryActiveProcessorCount(0);

    //
    // Allocate DPC routine
    //
    // for (size_t i = 0; i < CoreCount; i++)
    // {
    //     g_GuestState[i].KdDpcObject = ExAllocatePoolWithTag(NonPagedPool, sizeof(KDPC), POOLTAG);
    //
    //     if (g_GuestState[i].KdDpcObject == NULL)
    //     {
    //         LogError("Err, allocating dpc holder for debuggee");
    //         return;
    //     }
    // }

    //
    // Enable vm-exit on Hardware debug exceptions and breakpoints
    // so, intercept #DBs and #BP by changing exception bitmap (one core)
    //
    BroadcastEnableDbAndBpExitingAllCores();

    //
    // Reset pause break requests
    //
    RtlZeroMemory(&g_IgnoreBreaksToDebugger, sizeof(DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT));

    //
    // Initialize list of breakpoints and breakpoint id
    //
    g_MaximumBreakpointId = 0;

    InitializeListHead(&g_BreakpointsListHead);

    //
    // Indicate that the kernel debugger is active
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
    ULONG CoreCount;

    if (g_KernelDebuggerState)
    {
        CoreCount = KeQueryActiveProcessorCount(0);

        //
        // Indicate that the kernel debugger is not active
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
        // Disable vm-exit on Hardware debug exceptions and breakpoints
        // so, not intercept #DBs and #BP by changing exception bitmap (one core)
        //
        BroadcastDisableDbAndBpExitingAllCores();
    }
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

    LogInfo("I'm here %llx\n", DeferredContext);
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
KdFireDpc(PVOID Routine, PVOID Paramter)
{
    ULONG                   CurrentCore    = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CurrentCore];

    KeInitializeDpc(CurrentVmState->KdDpcObject, Routine, Paramter);

    KeInsertQueueDpc(CurrentVmState->KdDpcObject, NULL, NULL);
}

/**
 * @brief calculate the checksum of recived buffer from debugger
 *
 * @param Buffer
 * @param LengthReceived
 * @return BYTE
 */
_Use_decl_annotations_
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
_Use_decl_annotations_
BOOLEAN
KdResponsePacketToDebugger(
    DEBUGGER_REMOTE_PACKET_TYPE             PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION Response,
    CHAR *                                  OptionalBuffer,
    UINT32                                  OptionalBufferLength)
{
    DEBUGGER_REMOTE_PACKET Packet = {0};
    BOOLEAN                Result = FALSE;

    //
    // Make the packet's structure
    //
    Packet.Indicator       = INDICATOR_OF_HYPERDBG_PACKET;
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
        Packet.Checksum = KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                                                sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

        //
        // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
        // if not we use the windows spinlock
        //
        ScopedSpinlock(
            DebuggerResponseLock,
            Result = SerialConnectionSend((CHAR *)&Packet,
                                          sizeof(DEBUGGER_REMOTE_PACKET)));
    }
    else
    {
        Packet.Checksum = KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                                                sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

        Packet.Checksum += KdComputeDataChecksum((PVOID)OptionalBuffer, OptionalBufferLength);

        //
        // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
        // if not we use the windows spinlock
        //

        ScopedSpinlock(
            DebuggerResponseLock,
            Result = SerialConnectionSendTwoBuffers((CHAR *)&Packet,
                                                    sizeof(DEBUGGER_REMOTE_PACKET),
                                                    OptionalBuffer,
                                                    OptionalBufferLength));
    }

    if (g_IgnoreBreaksToDebugger.PauseBreaksUntilSpecialMessageSent &&
        g_IgnoreBreaksToDebugger.SpeialEventResponse == Response)
    {
        //
        // Set it to false by zeroing it
        //
        RtlZeroMemory(&g_IgnoreBreaksToDebugger, sizeof(DEBUGGEE_REQUEST_TO_IGNORE_BREAKS_UNTIL_AN_EVENT));
    }

    return Result;
}

/**
 * @brief Sends a HyperDbg logging response packet to the debugger
 *
 * @param OptionalBuffer
 * @param OptionalBufferLength
 * @param OperationCode
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
KdLoggingResponsePacketToDebugger(
    CHAR * OptionalBuffer,
    UINT32 OptionalBufferLength,
    UINT32 OperationCode)
{
    DEBUGGER_REMOTE_PACKET Packet = {0};
    BOOLEAN                Result = FALSE;

    //
    // Make the packet's structure
    //
    Packet.Indicator       = INDICATOR_OF_HYPERDBG_PACKET;
    Packet.TypeOfThePacket = DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER;

    //
    // Set the requested action
    //
    Packet.RequestedActionOfThePacket = DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_LOGGING_MECHANISM;

    //
    // Calculate checksum
    //
    Packet.Checksum = KdComputeDataChecksum((PVOID)((UINT64)&Packet + 1),
                                            sizeof(DEBUGGER_REMOTE_PACKET) - sizeof(BYTE));

    Packet.Checksum += KdComputeDataChecksum((PVOID)&OperationCode, sizeof(UINT32));
    Packet.Checksum += KdComputeDataChecksum((PVOID)OptionalBuffer, OptionalBufferLength);

    //
    // Check if we're in Vmx-root, if it is then we use our customized HIGH_IRQL Spinlock,
    // if not we use the windows spinlock
    //

    ScopedSpinlock(
        DebuggerResponseLock,
        Result = SerialConnectionSendThreeBuffers((CHAR *)&Packet,
                                                  sizeof(DEBUGGER_REMOTE_PACKET),
                                                  &OperationCode,
                                                  sizeof(UINT32),
                                                  OptionalBuffer,
                                                  OptionalBufferLength));

    return Result;
}

/**
 * @brief Handles debug events when kernel-debugger is attached
 *
 * @param CurrentCore
 * @param GuestRegs
 *
 * @return VOID
 */
VOID
KdHandleDebugEventsWhenKernelDebuggerIsAttached(UINT32 CurrentCore, PGUEST_REGS GuestRegs)
{
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag    = {0};
    RFLAGS                           Rflags           = {0};
    BOOLEAN                          IgnoreDebugEvent = FALSE;
    BOOLEAN                          AvoidUnsetMtf;

    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;

    //
    // It's a breakpoint and should be handled by the kernel debugger
    //
    ContextAndTag.Context = CurrentVmState->LastVmexitRip;

    if (CurrentVmState->DebuggingState.WaitForStepTrap)
    {
        //
        // *** Handle a regular step ***
        //

        //
        // Unset to show that we're no longer looking for a trap
        //
        CurrentDebuggingState->WaitForStepTrap = FALSE;

        //
        // Check if we should disable RFLAGS.TF in this core or not
        //
        if (CurrentDebuggingState->DisableTrapFlagOnContinue)
        {
            __vmx_vmread(VMCS_GUEST_RFLAGS, &Rflags);

            Rflags.TrapFlag = FALSE;

            __vmx_vmwrite(VMCS_GUEST_RFLAGS, Rflags.AsUInt);

            CurrentDebuggingState->DisableTrapFlagOnContinue = FALSE;
        }

        //
        // Check and handle if there is a software defined breakpoint
        //
        if (!BreakpointCheckAndHandleDebuggerDefinedBreakpoints(CurrentCore,
                                                                CurrentVmState->LastVmexitRip,
                                                                DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
                                                                GuestRegs,
                                                                &AvoidUnsetMtf))
        {
            if (g_HardwareDebugRegisterDetailsForStepOver.Address != NULL)
            {
                //
                // Check if it's caused by a step-over hardware debug breakpoint or not
                //
                if (CurrentVmState->LastVmexitRip == g_HardwareDebugRegisterDetailsForStepOver.Address)
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
                        DebugRegistersSet(DEBUGGER_DEBUG_REGISTER_FOR_STEP_OVER,
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
                ContextAndTag.Context = CurrentVmState->LastVmexitRip;
                KdHandleBreakpointAndDebugBreakpoints(CurrentCore,
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
        KdHandleBreakpointAndDebugBreakpoints(CurrentCore,
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
    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;
    //
    // Check to unset mov to cr3 vm-exits
    //
    if (CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent == TRUE)
    {
        //
        // Disable process change detection
        //
        ProcessEnableOrDisableThreadChangeMonitor(CurrentCore,
                                                  FALSE,
                                                  CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt);

        //
        // Avoid future sets/unsets
        //
        CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent = FALSE;
        CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt   = FALSE;
    }

    //
    // Check to unset change thread alerts
    //
    if (CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetThreadChangeEvent == TRUE)
    {
        //
        // Disable thread change alerts
        //
        ThreadEnableOrDisableThreadChangeMonitor(CurrentCore,
                                                 FALSE,
                                                 CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt);

        //
        // Avoid future sets/unsets
        //
        CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetThreadChangeEvent = FALSE;
        CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt  = FALSE;
    }
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
    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;
    //
    // Check to apply hardware debug register breakpoints for step-over
    //
    if (CurrentDebuggingState->HardwareDebugRegisterForStepping != NULL)
    {
        DebugRegistersSet(DEBUGGER_DEBUG_REGISTER_FOR_STEP_OVER,
                          BREAK_ON_INSTRUCTION_FETCH,
                          FALSE,
                          CurrentDebuggingState->HardwareDebugRegisterForStepping);

        CurrentDebuggingState->HardwareDebugRegisterForStepping = NULL;
    }

    //
    // Check to apply mov to cr3 vm-exits
    //
    if (CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent == TRUE)
    {
        //
        // Enable process change detection
        //
        ProcessEnableOrDisableThreadChangeMonitor(CurrentCore,
                                                  TRUE,
                                                  CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt);
    }

    //
    // Check to apply thread change alerts
    //
    if (CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetThreadChangeEvent == TRUE)
    {
        //
        // Enable alert for thread changes
        //
        ThreadEnableOrDisableThreadChangeMonitor(CurrentCore,
                                                 TRUE,
                                                 CurrentDebuggingState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt);
    }
}

/**
 * @brief continue the debuggee, this function gurantees that all other cores
 * are continued (except current core)
 * @param CurrentCore
 * @param SpeialEventResponse
 * @param PauseBreaksUntilSpecialMessageSent
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdContinueDebuggee(UINT32                                  CurrentCore,
                   BOOLEAN                                 PauseBreaksUntilSpecialMessageSent,
                   DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION SpeialEventResponse)
{
    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;

    if (PauseBreaksUntilSpecialMessageSent)
    {
        g_IgnoreBreaksToDebugger.PauseBreaksUntilSpecialMessageSent = TRUE;
        g_IgnoreBreaksToDebugger.SpeialEventResponse                = SpeialEventResponse;
    }

    //
    // Check if we should enable interrupts in this core or not,
    // we have another same check in SWITCHING CORES too
    //
    if (CurrentDebuggingState->EnableExternalInterruptsOnContinue)
    {
        //
        // Enable normal interrupt
        //
        HvSetExternalInterruptExiting(FALSE);

        //
        // Check if there is at least an interrupt that needs to be delivered
        //
        if (CurrentVmState->PendingExternalInterrupts[0] != NULL)
        {
            //
            // Enable Interrupt-window exiting.
            //
            HvSetInterruptWindowExiting(TRUE);
        }

        CurrentDebuggingState->EnableExternalInterruptsOnContinue = FALSE;
    }

    //
    // Unlock all the cores
    //
    ULONG CoreCount = KeQueryActiveProcessorCount(0);
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
    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;

    //
    // In the case of any halting event, the processor won't send NMIs
    // to other cores if this field is set
    //
    CurrentDebuggingState->DoNotNmiNotifyOtherCoresByThisCore = TRUE;

    //
    // Unlock the current core
    //
    SpinlockUnlock(&CurrentDebuggingState->Lock);
}

/**
 * @brief read registers
 * @param Regs
 * @param ReadRegisterRequest
 *
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
KdReadRegisters(PGUEST_REGS Regs, PDEBUGGEE_REGISTER_READ_DESCRIPTION ReadRegisterRequest)
{
    GUEST_EXTRA_REGISTERS ERegs = {0};

    if (ReadRegisterRequest->RegisterID == DEBUGGEE_SHOW_ALL_REGISTERS)
    {
        //
        // Add General purpose registers
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
_Use_decl_annotations_
BOOLEAN
KdReadMemory(PGUEST_REGS Regs, PDEBUGGEE_REGISTER_READ_DESCRIPTION ReadRegisterRequest)
{
    GUEST_EXTRA_REGISTERS ERegs = {0};

    if (ReadRegisterRequest->RegisterID == DEBUGGEE_SHOW_ALL_REGISTERS)
    {
        //
        // Add General purpose registers
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
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CurrentCore];

    ULONG CoreCount = KeQueryActiveProcessorCount(0);
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
    if (CurrentVmState->DebuggingState.EnableExternalInterruptsOnContinue)
    {
        //
        // Enable normal interrupts
        //
        HvSetExternalInterruptExiting(FALSE);

        //
        // Check if there is at least an interrupt that needs to be delivered
        //
        if (CurrentVmState->PendingExternalInterrupts[0] != NULL)
        {
            //
            // Enable Interrupt-window exiting.
            //
            HvSetInterruptWindowExiting(TRUE);
        }

        CurrentVmState->DebuggingState.EnableExternalInterruptsOnContinue = FALSE;
    }

    //
    // Unset the current operating core (this is not important as if we
    // return from the operating function then the it will be unset
    // automatically but as we want to not have two operating cores
    // at the same time so we unset it here too)
    //
    CurrentVmState->DebuggingState.MainDebuggingCore = FALSE;

    //
    // Set new operating core
    //
    g_GuestState[NewCore].DebuggingState.MainDebuggingCore = TRUE;

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
                  1,
                  TRUE);
}

/**
 * @brief Notify user-mode to re-send (reload) the symbol packets
 * @param SymPacket
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdReloadSymbolDetailsInDebuggee(PDEBUGGEE_SYMBOL_REQUEST_PACKET SymPacket)
{
    //
    // Send one byte buffer and operation codes
    //
    LogSendBuffer(OPERATION_COMMAND_FROM_DEBUGGER_RELOAD_SYMBOL,
                  SymPacket,
                  sizeof(DEBUGGEE_SYMBOL_REQUEST_PACKET),
                  TRUE);
}

/**
 * @brief Notify user-mode to about new user-input buffer
 * @param Descriptor
 * @param Len
 *
 * @return VOID
 */

VOID
KdNotifyDebuggeeForUserInput(DEBUGGEE_USER_INPUT_PACKET * Descriptor, UINT32 Len)
{
    //
    // Send user-input buffer along with operation code to
    // the user-mode
    //
    LogSendBuffer(OPERATION_DEBUGGEE_USER_INPUT,
                  Descriptor,
                  Len,
                  TRUE);
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

    FormatsPacket.Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
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
KdSendCommandFinishedSignal(UINT32      CurrentCore,
                            PGUEST_REGS GuestRegs)
{
    //
    // Halt other cores again
    //
    KdHandleBreakpointAndDebugBreakpoints(CurrentCore,
                                          GuestRegs,
                                          DEBUGGEE_PAUSING_REASON_DEBUGGEE_COMMAND_EXECUTION_FINISHED,
                                          NULL);
}

/**
 * @brief Tries to get the lock and won't return until successfully get the lock
 *
 * @param UINT32 CurrentCore
 * @param GuestRegs Guest registers
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdHandleHaltsWhenNmiReceivedFromVmxRoot(UINT32 CurrentCore, PGUEST_REGS GuestRegs)
{
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CurrentCore];

    //
    // During the debugging of HyperDbg, we realized that whenever an
    // event is set, one core might (and will) get the lock of the
    // debugging and other cores that are triggering the same event
    // go to the wait for the debugging lock
    // As we send NMIs to all cores to notify them and halt them,
    // a core might be in VMX-root and receive the NMI
    // Handling the NMI and halting from the NMI handlers is not
    // possible as the stack of the Windows' NMI handler routine
    // is not big enough to handle HyperDbg's command dispatching
    // routines, so if the user switches to the cores that are halted
    // from an NMI handler then we ran out of stack and debugger crashes
    // It is also not possible to change the stack to a bigger stack because
    // we're not really interested in allocating other memories for the stack
    // and also the target stack will not be a valid Windows stack as it's
    // supposed to run with NMI handling routines.
    //
    // By the way, I conclude to let the NMI handler finish its normal
    // execution and then we check for the possible pausing reasons.
    //
    // The pausing scenario should be checked two cases,
    //      1. If the current core is stucked in spinlock of getting
    //          the debug lock
    //      2. If the current core is making it self ready for the vm-entry
    //
    // In these two cases we should check for the possible halting of the core
    //

    //
    // Handle halt of the current core as an NMI
    //
    KdHandleNmi(CurrentCore, GuestRegs);

    //
    // Set the indication to false as we handled it
    //
    CurrentVmState->DebuggingState.NmiCalledInVmxRootRelatedToHaltDebuggee = FALSE;
}

/**
 * @brief Tries to get the lock and won't return until successfully get the lock
 *
 * @param UINT32 CurrentCore
 * @param LONG Lock variable
 * @param GuestRegs Guest registers
 *
 * @return VOID
 */
VOID
KdCustomDebuggerBreakSpinlockLock(UINT32 CurrentCore, volatile LONG * Lock, PGUEST_REGS GuestRegs)
{
    unsigned wait = 1;

    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;
    //
    // *** Lock handling breaks ***
    //

    while (!SpinlockTryLock(Lock))
    {
        for (unsigned i = 0; i < wait; ++i)
        {
            _mm_pause();
        }

        //
        // check if the core needs to be locked
        //
        if (CurrentDebuggingState->WaitingToBeLocked)
        {
            //
            // We should ignore one MTF as we touched MTF and it's not usable anymore
            //
            CurrentDebuggingState->IgnoreOneMtf = TRUE;

            //
            // Handle break of the core
            //
            if (CurrentDebuggingState->NmiCalledInVmxRootRelatedToHaltDebuggee)
            {
                //
                // Handle it like an NMI is received from VMX root
                //
                KdHandleHaltsWhenNmiReceivedFromVmxRoot(CurrentCore, GuestRegs);
            }
            else
            {
                //
                // Handle halt of the current core as an NMI
                //
                KdHandleNmi(CurrentCore, GuestRegs);
            }
        }

        //
        // Don't call "pause" too many times. If the wait becomes too big,
        // clamp it to the MaxWait.
        //

        if (wait * 2 > 65536)
        {
            wait = 65536;
        }
        else
        {
            wait = wait * 2;
        }
    }
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdHandleBreakpointAndDebugBreakpoints(UINT32                            CurrentCore,
                                      PGUEST_REGS                       GuestRegs,
                                      DEBUGGEE_PAUSING_REASON           Reason,
                                      PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails)
{
    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;

    //
    // Lock handling breaks
    //
    KdCustomDebuggerBreakSpinlockLock(CurrentCore, &DebuggerHandleBreakpointLock, GuestRegs);

    //
    // Check if we should ignore this break request or not
    //
    if (g_IgnoreBreaksToDebugger.PauseBreaksUntilSpecialMessageSent)
    {
        //
        // Unlock the above core
        //
        SpinlockUnlock(&DebuggerHandleBreakpointLock);

        //
        // Not continue anymore as the break should be ignored
        //
        return;
    }

    //
    // Set it as the main core
    //
    CurrentDebuggingState->MainDebuggingCore = TRUE;

    //
    // Lock current core
    //
    CurrentDebuggingState->WaitingToBeLocked = FALSE;
    SpinlockLock(&CurrentDebuggingState->Lock);

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

    if (CurrentDebuggingState->DoNotNmiNotifyOtherCoresByThisCore == TRUE)
    {
        //
        // Unset to avoid future not notifying events
        //
        CurrentDebuggingState->DoNotNmiNotifyOtherCoresByThisCore = FALSE;
    }
    else
    {
        //
        // Broadcast NMI with the intention of halting cores
        //
        VmxBroadcastNmi(CurrentCore, NMI_BROADCAST_ACTION_KD_HALT_CORE);
    }

    //
    // All the cores should go and manage through the following function
    //
    KdManageSystemHaltOnVmxRoot(CurrentCore, GuestRegs, EventDetails);

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
    // Unlock handling breaks
    //
    if (CurrentDebuggingState->MainDebuggingCore)
    {
        CurrentDebuggingState->MainDebuggingCore = FALSE;
        SpinlockUnlock(&DebuggerHandleBreakpointLock);
    }
}

/**
 * @brief Handle NMI Vm-exits
 * @param CurrentCore
 * @param GuestRegs
 *
 * @details This function should be called in vmx-root mode
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdHandleNmi(UINT32 CurrentCore, PGUEST_REGS GuestRegs)
{
    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;

    //
    // Test
    //

    // LogInfo("NMI Arrived on : %d \n",CurrentProcessorIndex);

    //
    // Not the main debugging core
    //
    CurrentDebuggingState->MainDebuggingCore = FALSE;

    //
    // Lock current core
    //
    CurrentDebuggingState->WaitingToBeLocked = FALSE;
    SpinlockLock(&CurrentDebuggingState->Lock);

    //
    // All the cores should go and manage through the following function
    //
    KdManageSystemHaltOnVmxRoot(CurrentCore, GuestRegs, NULL);

    //
    // Unlock handling breaks
    //
    if (CurrentDebuggingState->MainDebuggingCore)
    {
        CurrentDebuggingState->MainDebuggingCore = FALSE;
        SpinlockUnlock(&DebuggerHandleBreakpointLock);
    }
}

/**
 * @brief apply a guaranteed step one instruction to the debuggee
 * @param CurrentCore
 * @return VOID
 */
VOID
KdGuaranteedStepInstruction(UINT32 CurrentCore)
{
    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;

    //
    // Only 16 bit is needed howerver, vmwrite might write on other bits
    // and corrupt other variables, that's why we get 64bit
    //
    UINT64 CsSel = NULL;

    //
    // Read cs to have a trace of the execution mode of running application
    // in the debuggee
    //
    __vmx_vmread(VMCS_GUEST_CS_SELECTOR, &CsSel);
    CurrentDebuggingState->InstrumentationStepInTrace.CsSel = (UINT16)CsSel;

    //
    // Set an indicator of wait for MTF
    //
    CurrentDebuggingState->InstrumentationStepInTrace.WaitForInstrumentationStepInMtf = TRUE;

    //
    // Not unset again
    //
    CurrentVmState->IgnoreMtfUnset = TRUE;

    //
    // Change guest interrupt-state
    //
    HvSetExternalInterruptExiting(TRUE);

    //
    // Do not vm-exit on interrupt windows
    //
    HvSetInterruptWindowExiting(FALSE);
    CurrentDebuggingState->EnableExternalInterruptsOnContinue = TRUE;

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
        LogInfo("User-mode -> Kernel-mode\n");
    }
    else if ((CurrentCsSelector == KGDT64_R3_CODE || CurrentCsSelector == KGDT64_R3_CMCODE) && PreviousCsSelector == KGDT64_R0_CODE)
    {
        //
        // Kernel-mode to user-mode
        //
        LogInfo("Kernel-mode -> User-mode\n");
    }
    else if (CurrentCsSelector == KGDT64_R3_CODE && PreviousCsSelector == KGDT64_R3_CMCODE)
    {
        //
        // A heaven's gate (User-mode 32-bit code -> User-mode 64-bit code)
        //
        LogInfo("32-bit User-mode -> 64-bit User-mode (Heaven's gate)\n");
    }
    else if (PreviousCsSelector == KGDT64_R3_CODE && CurrentCsSelector == KGDT64_R3_CMCODE)
    {
        //
        // A heaven's gate (User-mode 64-bit code -> User-mode 32-bit code)
        //
        LogInfo("64-bit User-mode -> 32-bit User-mode (Return from Heaven's gate)\n");
    }
    else
    {
        LogError("Err, unknown changes in cs selector during the instrumentation step-in\n");
    }

    //
    // Execution modes are changed
    //
    return TRUE;
}

/**
 * @brief Regualar step-in | step one instruction to the debuggee
 * @param CurrentCore
 *
 * @return VOID
 */
VOID
KdRegularStepInInstruction(UINT32 CurrentCore)
{
    UINT32 Interruptibility;
    UINT32 InterruptibilityOld = NULL;
    RFLAGS Rflags              = {0};

    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;

    //
    // We're waiting for an step
    //
    CurrentDebuggingState->WaitForStepTrap = TRUE;

    //
    // Change guest trap flag
    //
    if (!CurrentDebuggingState->DisableTrapFlagOnContinue)
    {
        __vmx_vmread(VMCS_GUEST_RFLAGS, &Rflags);

        if (Rflags.TrapFlag == FALSE)
        {
            Rflags.TrapFlag = TRUE;

            __vmx_vmwrite(VMCS_GUEST_RFLAGS, Rflags.AsUInt);

            CurrentDebuggingState->DisableTrapFlagOnContinue = TRUE;
        }
    }

    //
    // During testing single-step, we realized that after single-stepping
    // on 'STI' instruction, after one instruction, the guest (target core)
    // starts Invalid Guest State (0x21) vm-exits, after some searches we
    // realized that KVM developer's encountered the same error; so, in order
    // to solve the problem of stepping on 'STI' and 'MOV SS', we check the
    // interruptibility state, here is a comment from KVM :
    //
    // When single stepping over STI and MOV SS, we must clear the
    // corresponding interruptibility bits in the guest state
    // Otherwise vmentry fails as it then expects bit 14 (BS)
    // in pending debug exceptions being set, but that's not
    // correct for the guest debugging case
    //
    __vmx_vmread(VMCS_GUEST_INTERRUPTIBILITY_STATE, &InterruptibilityOld);

    Interruptibility = InterruptibilityOld;

    Interruptibility &= ~(GUEST_INTR_STATE_STI | GUEST_INTR_STATE_MOV_SS);

    if ((Interruptibility != InterruptibilityOld))
    {
        __vmx_vmwrite(VMCS_GUEST_INTERRUPTIBILITY_STATE, Interruptibility);
    }
}

/**
 * @brief Regualar step-over | step one instruction to the debuggee if
 * there is a call then it jumps the call
 *
 * @param IsNextInstructionACall
 * @param CurrentCore
 *
 * @return VOID
 */
VOID
KdRegularStepOver(BOOLEAN IsNextInstructionACall, UINT32 CallLength, UINT32 CurrentCore)
{
    UINT64 NextAddressForHardwareDebugBp = 0;
    ULONG  CoreCount;

    VIRTUAL_MACHINE_STATE *     CurrentVmState        = &g_GuestState[CurrentCore];
    PROCESSOR_DEBUGGING_STATE * CurrentDebuggingState = &CurrentVmState->DebuggingState;

    if (IsNextInstructionACall)
    {
        //
        // It's a call, we should put a hardware debug register breakpoint
        // on the next instruction
        //
        CurrentDebuggingState->WaitForStepTrap = TRUE;
        NextAddressForHardwareDebugBp          = CurrentVmState->LastVmexitRip + CallLength;

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
            CurrentDebuggingState->HardwareDebugRegisterForStepping = NextAddressForHardwareDebugBp;
        }
    }
    else
    {
        //
        // Any instruction other than call (regular step)
        //
        KdRegularStepInInstruction(CurrentCore);
    }
}

/**
 * @brief Send event registration buffer to user-mode to register the event
 * @param EventDetailHeader
 *
 * @return VOID
 */
VOID
KdPerformRegisterEvent(PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET EventDetailHeader)
{
    LogSendBuffer(OPERATION_DEBUGGEE_REGISTER_EVENT,
                  ((CHAR *)EventDetailHeader + sizeof(DEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET)),
                  EventDetailHeader->Length,
                  TRUE);
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
                  ActionDetailHeader->Length,
                  TRUE);
}

/**
 * @brief Query state of the system
 *
 * @return VOID
 */
VOID
KdQuerySystemState()
{
    ULONG CoreCount;

    CoreCount = KeQueryActiveProcessorCount(0);

    //
    // Query core debugging Lock info
    //
    Log("================================================ Debugging Lock Info ================================================\n");

    for (size_t i = 0; i < CoreCount; i++)
    {
        if (g_GuestState[i].DebuggingState.Lock)
        {
            LogInfo("Core : %d is locked", i);
        }
        else
        {
            LogInfo("Core : %d isn't locked", i);
        }
    }

    //
    // Query if the core is halted (or NMI is received) when the debuggee
    // was in the vmx-root mode
    //
    Log("\n================================================ NMI Receiver State =======+=========================================\n");

    for (size_t i = 0; i < CoreCount; i++)
    {
        if (g_GuestState[i].DebuggingState.NmiCalledInVmxRootRelatedToHaltDebuggee)
        {
            LogInfo("Core : %d - called from an NMI that is called in VMX-root mode", i);
        }
        else
        {
            LogInfo("Core : %d - not called from an NMI handler (through the immediate VM-exit mechanism)", i);
        }
    }
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
            ModifyAndQueryEvent->KernelStatus = DEBUGGER_ERROR_TAG_NOT_EXISTS;
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
            ModifyAndQueryEvent->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
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
        ModifyAndQueryEvent->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
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
        ModifyAndQueryEvent->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
    }
    else if (ModifyAndQueryEvent->TypeOfAction == DEBUGGER_MODIFY_EVENTS_CLEAR)
    {
        //
        // Send one byte buffer and operation codes
        //
        LogSendBuffer(OPERATION_DEBUGGEE_CLEAR_EVENTS,
                      ModifyAndQueryEvent,
                      sizeof(DEBUGGER_MODIFY_EVENTS),
                      TRUE);
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
    PDEBUGGER_CALLSTACK_REQUEST                         CallstackPacket;
    PDEBUGGER_SINGLE_CALLSTACK_FRAME                    CallstackFrameBuffer;
    PDEBUGGER_DEBUGGER_TEST_QUERY_BUFFER                TestQueryPacket;
    PDEBUGGEE_REGISTER_READ_DESCRIPTION                 ReadRegisterPacket;
    PDEBUGGER_READ_MEMORY                               ReadMemoryPacket;
    PDEBUGGER_EDIT_MEMORY                               EditMemoryPacket;
    PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET         ChangeProcessPacket;
    PDEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET          ChangeThreadPacket;
    PDEBUGGEE_SCRIPT_PACKET                             ScriptPacket;
    PDEBUGGEE_USER_INPUT_PACKET                         UserInputPacket;
    PDEBUGGER_SEARCH_MEMORY                             SearchQueryPacket;
    PDEBUGGEE_BP_PACKET                                 BpPacket;
    PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS           PtePacket;
    PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS                  Va2paPa2vaPacket;
    PDEBUGGEE_BP_LIST_OR_MODIFY_PACKET                  BpListOrModifyPacket;
    PDEBUGGEE_SYMBOL_REQUEST_PACKET                     SymReloadPacket;
    PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET EventRegPacket;
    PDEBUGGEE_EVENT_AND_ACTION_HEADER_FOR_REMOTE_PACKET AddActionPacket;
    PDEBUGGER_MODIFY_EVENTS                             QueryAndModifyEventPacket;
    UINT32                                              SizeToSend         = 0;
    BOOLEAN                                             UnlockTheNewCore   = FALSE;
    size_t                                              ReturnSize         = 0;
    DEBUGGEE_RESULT_OF_SEARCH_PACKET                    SearchPacketResult = {0};

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
        if (!SerialConnectionRecvBuffer(&RecvBuffer, &RecvBufferLength))
        {
            //
            // Invalid buffer
            //
            continue;
        }

        if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKET)
        {
            //
            // Check checksum
            //
            if (KdComputeDataChecksum((PVOID)&TheActualPacket->Indicator,
                                      RecvBufferLength - sizeof(BYTE)) !=
                TheActualPacket->Checksum)
            {
                LogError("Err, checksum is invalid");
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
                LogError("Err, unknown packet received from the debugger\n");
                continue;
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

                if (SteppingPacket->StepType == DEBUGGER_REMOTE_STEPPING_REQUEST_INSTRUMENTATION_STEP_IN)
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
                    // Step-over (p command)
                    //
                    KdRegularStepOver(SteppingPacket->IsCurrentInstructionACall, SteppingPacket->CallLength, CurrentCore);

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

                        ChangeCorePacket->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
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
                    ChangeCorePacket->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
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

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CALLSTACK:

                CallstackPacket = (DEBUGGER_CALLSTACK_REQUEST *)(((CHAR *)TheActualPacket) +
                                                                 sizeof(DEBUGGER_REMOTE_PACKET));

                CallstackFrameBuffer = (DEBUGGER_SINGLE_CALLSTACK_FRAME *)(((CHAR *)TheActualPacket) +
                                                                           sizeof(DEBUGGER_REMOTE_PACKET) +
                                                                           sizeof(DEBUGGER_CALLSTACK_REQUEST));

                //
                // If the address is null, we use the current RSP register
                //
                if (CallstackPacket->BaseAddress == NULL)
                {
                    CallstackPacket->BaseAddress = GuestRegs->rsp;
                }

                //
                // Feel the callstack frames the buffers
                //
                if (CallstackWalkthroughStack(CallstackFrameBuffer,
                                              CallstackPacket->BaseAddress,
                                              CallstackPacket->Size,
                                              CallstackPacket->Is32Bit))
                {
                    CallstackPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
                }
                else
                {
                    CallstackPacket->KernelStatus = DEBUGGER_ERROR_UNABLE_TO_GET_CALLSTACK;
                }

                //
                // Send the result of flushing back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CALLSTACK,
                                           CallstackPacket,
                                           CallstackPacket->BufferSize);

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_TEST_QUERY:

                TestQueryPacket = (DEBUGGER_DEBUGGER_TEST_QUERY_BUFFER *)(((CHAR *)TheActualPacket) +
                                                                          sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Dispatch the request
                //

                switch (TestQueryPacket->RequestIndex)
                {
                case TEST_QUERY_HALTING_CORE_STATUS:

                    //
                    // Query the state of the system
                    //
                    KdQuerySystemState();

                    TestQueryPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

                    break;

                default:

                    //
                    // Query index not found
                    //
                    TestQueryPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;

                    break;
                }

                //
                // Send the result of query system state to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_TEST_QUERY,
                                           TestQueryPacket,
                                           sizeof(DEBUGGER_DEBUGGER_TEST_QUERY_BUFFER));

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_READ_REGISTERS:

                ReadRegisterPacket = (DEBUGGEE_REGISTER_READ_DESCRIPTION *)(((CHAR *)TheActualPacket) +
                                                                            sizeof(DEBUGGER_REMOTE_PACKET));
                //
                // Read registers
                //
                if (KdReadRegisters(GuestRegs, ReadRegisterPacket))
                {
                    ReadRegisterPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
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
                    ReadMemoryPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
                }
                else
                {
                    ReadMemoryPacket->KernelStatus = DEBUGGER_ERROR_INVALID_ADDRESS;
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
                    EditMemoryPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
                }
                else
                {
                    EditMemoryPacket->KernelStatus = DEBUGGER_ERROR_INVALID_ADDRESS;
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

                ChangeProcessPacket = (DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET *)(((CHAR *)TheActualPacket) +
                                                                                     sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Interpret the process packet
                //
                ProcessInterpretProcess(ChangeProcessPacket);

                //
                // Send the result of switching process back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_PROCESS,
                                           ChangeProcessPacket,
                                           sizeof(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET));

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_THREAD:

                ChangeThreadPacket = (DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET *)(((CHAR *)TheActualPacket) +
                                                                                   sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Interpret the thread packet
                //
                ThreadInterpretThread(ChangeThreadPacket);

                //
                // Send the result of switching thread back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_CHANGING_THREAD,
                                           ChangeThreadPacket,
                                           sizeof(DEBUGGEE_DETAILS_AND_SWITCH_THREAD_PACKET));

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
                    ScriptPacket->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
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
                KdNotifyDebuggeeForUserInput(((CHAR *)UserInputPacket),
                                             sizeof(DEBUGGEE_USER_INPUT_PACKET) + UserInputPacket->CommandLen);

                //
                // Continue Debuggee
                //
                KdContinueDebuggee(CurrentCore, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);
                EscapeFromTheLoop = TRUE;

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SEARCH_QUERY:

                SearchQueryPacket = (DEBUGGER_SEARCH_MEMORY *)(((CHAR *)TheActualPacket) +
                                                               sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Perfom the search in debuggee debuggee
                // Call the search wrapper
                //

                if (SearchAddressWrapper(NULL,
                                         SearchQueryPacket,
                                         SearchQueryPacket->Address,
                                         SearchQueryPacket->Address + SearchQueryPacket->Length,
                                         TRUE,
                                         &SearchPacketResult.CountOfResults))
                {
                    //
                    // The search was successful
                    //
                    SearchPacketResult.Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
                }
                else
                {
                    //
                    // There was an error, probably the address was not valid
                    //
                    SearchPacketResult.Result = DEBUGGER_ERROR_INVALID_ADDRESS;
                }

                //
                // Send the result of 's*' back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RELOAD_SEARCH_QUERY,
                                           &SearchPacketResult,
                                           sizeof(DEBUGGEE_RESULT_OF_SEARCH_PACKET));

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

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SYMBOL_QUERY_PTE:

                PtePacket = (DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS *)(((CHAR *)TheActualPacket) +
                                                                         sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Get the page table details (it's in vmx-root)
                //
                ExtensionCommandPte(PtePacket, TRUE);

                //
                // Send the result of '!pte' back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_PTE,
                                           PtePacket,
                                           sizeof(DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS));

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_QUERY_PA2VA_AND_VA2PA:

                Va2paPa2vaPacket = (DEBUGGER_VA2PA_AND_PA2VA_COMMANDS *)(((CHAR *)TheActualPacket) +
                                                                         sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Perform the virtual to physical or physical to virtual address
                // conversion (it's on vmx-root mode)
                //
                ExtensionCommandVa2paAndPa2va(Va2paPa2vaPacket, TRUE);

                //
                // Send the result of '!va2pa' or '!pa2va' back to the debuggee
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_VA2PA_AND_PA2VA,
                                           Va2paPa2vaPacket,
                                           sizeof(DEBUGGER_VA2PA_AND_PA2VA_COMMANDS));

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

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SYMBOL_RELOAD:

                SymReloadPacket = (DEBUGGEE_SYMBOL_REQUEST_PACKET *)(((CHAR *)TheActualPacket) +
                                                                     sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Send the reload symbol request buffer
                //
                KdReloadSymbolDetailsInDebuggee(SymReloadPacket);

                //
                // Unlock other cores
                //
                KdContinueDebuggee(CurrentCore, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

                //
                // No need to wait for new commands
                //
                EscapeFromTheLoop = TRUE;

                break;

            default:
                LogError("Err, unknown packet action received from the debugger\n");
                break;
            }
        }
        else
        {
            //
            // It's not a HyperDbg packet, the packet is probably deformed
            //
            LogError("Err, it's not a HyperDbg packet, the packet is probably deformed\n");
            continue;
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
    //
    // Only 16 bit is needed howerver, vmwrite might write on other bits
    // and corrupt other variables, that's why we get 64bit
    //
    UINT64 CsSel = NULL;

    //
    // Read guest's cs selector
    //
    __vmx_vmread(VMCS_GUEST_CS_SELECTOR, &CsSel);

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
        LogError("Err, unknown value for cs, cannot determine wow64 mode");
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
                            PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails)
{
    DEBUGGEE_KD_PAUSED_PACKET PausePacket;
    ULONG                     ExitInstructionLength  = 0;
    UINT64                    SizeOfSafeBufferToRead = 0;
    RFLAGS                    Rflags                 = {0};
    VIRTUAL_MACHINE_STATE *   CurrentVmState         = &g_GuestState[CurrentCore];

    //
    // Perform Pre-halt tasks
    //
    KdApplyTasksPreHaltCore(CurrentCore);

StartAgain:

    //
    // We check for receiving buffer (unhalting) only on the
    // first core and not on every cores
    //
    if (CurrentVmState->DebuggingState.MainDebuggingCore)
    {
        //
        // *** Current Operating Core  ***
        //
        RtlZeroMemory(&PausePacket, sizeof(DEBUGGEE_KD_PAUSED_PACKET));

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
        PausePacket.Rip            = CurrentVmState->LastVmexitRip;
        PausePacket.Is32BitAddress = KdIsGuestOnUsermode32Bit();

        //
        // Set rflags for finding the results of conditional jumps
        //
        __vmx_vmread(VMCS_GUEST_RFLAGS, &Rflags);
        PausePacket.Rflags = Rflags.AsUInt;

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
        if (CurrentVmState->DebuggingState.InstructionLengthHint != 0)
        {
            ExitInstructionLength = CurrentVmState->DebuggingState.InstructionLengthHint;
        }
        else
        {
            //
            // Reading instruction length proved to provide wrong results,
            // so we won't use it anymore
            //
            // __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &ExitInstructionLength);
            //

            //
            // Compute the amount of buffer we can read without problem
            //
            SizeOfSafeBufferToRead = CurrentVmState->LastVmexitRip & 0xfff;
            SizeOfSafeBufferToRead += MAXIMUM_INSTR_SIZE;

            if (SizeOfSafeBufferToRead >= PAGE_SIZE)
            {
                SizeOfSafeBufferToRead = SizeOfSafeBufferToRead - PAGE_SIZE;
                SizeOfSafeBufferToRead = MAXIMUM_INSTR_SIZE - SizeOfSafeBufferToRead;
            }
            else
            {
                SizeOfSafeBufferToRead = MAXIMUM_INSTR_SIZE;
            }

            //
            // Set the length to notify debuggee
            //
            ExitInstructionLength = SizeOfSafeBufferToRead;
        }

        //
        // Set the reading length of bytes (for instruction disassembling)
        //
        PausePacket.ReadInstructionLen = ExitInstructionLength;

        //
        // Find the current instruction
        //
        MemoryMapperReadMemorySafeOnTargetProcess(CurrentVmState->LastVmexitRip,
                                                  &PausePacket.InstructionBytesOnRip,
                                                  ExitInstructionLength);

        //
        // Send the pause packet, along with RIP and an indication
        // to pause to the debugger
        //
        KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                   DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_PAUSED_AND_CURRENT_INSTRUCTION,
                                   &PausePacket,
                                   sizeof(DEBUGGEE_KD_PAUSED_PACKET));

        //
        // Perform Commands from the debugger
        //
        KdDispatchAndPerformCommandsFromDebugger(CurrentCore, GuestRegs);

        //
        // Check if it's a change core event or not, otherwise finish the execution
        // and continue debuggee
        //
        if (!CurrentVmState->DebuggingState.MainDebuggingCore)
        {
            //
            // It's a core switch, start again
            //
            goto StartAgain;
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
        CurrentVmState->DebuggingState.WaitingToBeLocked = FALSE;

        ScopedSpinlock(
            CurrentVmState->DebuggingState.Lock,
            //
            // Check if it's a change core event or not
            //
            if (CurrentVmState->DebuggingState.MainDebuggingCore) {
                //
                // It's a core change event
                //
                g_DebuggeeHaltReason = DEBUGGEE_PAUSING_REASON_DEBUGGEE_CORE_SWITCHED;

                goto StartAgain;
            }

        );
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
    KeGenericCallDpc(DpcRoutineVmExitAndHaltSystemAllCores, NULL);
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
    // KdBroadcastHaltOnAllCores();
    //

    //
    // vm-exit and halt current core
    //
    AsmVmxVmcall(VMCALL_VM_EXIT_HALT_SYSTEM, 0, 0, 0);

    //
    // Set the status
    //
    PausePacket->Result = DEBUGGER_OPERATION_WAS_SUCCESSFULL;
}
