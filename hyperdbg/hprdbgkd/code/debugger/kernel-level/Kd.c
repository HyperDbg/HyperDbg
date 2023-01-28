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
    //     g_DbgState[i].KdDpcObject = ExAllocatePoolWithTag(NonPagedPool, sizeof(KDPC), POOLTAG);
    //
    //     if (g_DbgState[i].KdDpcObject == NULL)
    //     {
    //         LogError("Err, allocating dpc holder for debuggee");
    //         return;
    //     }
    // }

    //
    // Request pages for breakpoint detail
    //
    PoolManagerRequestAllocation(sizeof(DEBUGGEE_BP_DESCRIPTOR),
                                 MAXIMUM_BREAKPOINTS_WITHOUT_CONTINUE,
                                 BREAKPOINT_DEFINITION_STRUCTURE);

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
 * @brief Checks whether the immediate messaging mechism is
 * needed or not
 * @param OperationCode
 *
 * @return BOOLEAN
 */
BOOLEAN
KdCheckImmediateMessagingMechanism(UINT32 OperationCode)
{
    return (g_KernelDebuggerState && !(OperationCode & OPERATION_MANDATORY_DEBUGGEE_BIT));
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
    ULONG CurrentCore = KeGetCurrentProcessorNumber();

    KeInitializeDpc(g_DbgState[CurrentCore].KdDpcObject, Routine, Paramter);

    KeInsertQueueDpc(g_DbgState[CurrentCore].KdDpcObject, NULL, NULL);
}

/**
 * @brief Query for process/thread interception status
 * @param CoreId
 * @param TracingType
 *
 * @return BOOLEAN whether it's activated or not
 */
BOOLEAN
KdQueryDebuggerQueryThreadOrProcessTracingDetailsByCoreId(UINT32                          CoreId,
                                                          DEBUGGER_THREAD_PROCESS_TRACING TracingType)
{
    BOOLEAN                     Result   = FALSE;
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[CoreId];

    switch (TracingType)
    {
    case DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_INTERRUPTS_FOR_THREAD_CHANGE:

        Result = DbgState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForThreadChange;

        break;

    case DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_INTERRUPTS_FOR_PROCESS_CHANGE:

        Result = DbgState->ThreadOrProcessTracingDetails.InterceptClockInterruptsForProcessChange;

        break;

    case DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_DEBUG_REGISTER_INTERCEPTION:

        Result = DbgState->ThreadOrProcessTracingDetails.DebugRegisterInterceptionState;

        break;

    case DEBUGGER_THREAD_PROCESS_TRACING_INTERCEPT_CLOCK_WAITING_FOR_MOV_CR3_VM_EXITS:

        Result = DbgState->ThreadOrProcessTracingDetails.IsWatingForMovCr3VmExits;

        break;

    default:

        LogError("Err, debugger encountered an unknown query type for querying process or thread interception details");

        break;
    }

    return Result;
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
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
KdHandleDebugEventsWhenKernelDebuggerIsAttached(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag    = {0};
    RFLAGS                           Rflags           = {0};
    BOOLEAN                          IgnoreDebugEvent = FALSE;
    UINT64                           LastVmexitRip    = VmFuncGetLastVmexitRip(DbgState->CoreId);
    //
    // It's a breakpoint and should be handled by the kernel debugger
    //
    ContextAndTag.Context = LastVmexitRip;

    if (DbgState->WaitForStepTrap)
    {
        //
        // *** Handle a regular step ***
        //

        //
        // Unset to show that we're no longer looking for a trap
        //
        DbgState->WaitForStepTrap = FALSE;

        //
        // Check if we should disable RFLAGS.TF in this core or not
        //
        if (DbgState->DisableTrapFlagOnContinue)
        {
            Rflags.AsUInt = VmFuncGetRflags();

            Rflags.TrapFlag = FALSE;

            VmFuncSetRflags(Rflags.AsUInt);

            DbgState->DisableTrapFlagOnContinue = FALSE;
        }

        //
        // Check and handle if there is a software defined breakpoint
        //
        if (!BreakpointCheckAndHandleDebuggerDefinedBreakpoints(DbgState,
                                                                LastVmexitRip,
                                                                DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
                                                                FALSE))
        {
            if (g_HardwareDebugRegisterDetailsForStepOver.Address != NULL)
            {
                //
                // Check if it's caused by a step-over hardware debug breakpoint or not
                //
                if (LastVmexitRip == g_HardwareDebugRegisterDetailsForStepOver.Address)
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
                        SetDebugRegisters(DEBUGGER_DEBUG_REGISTER_FOR_STEP_OVER,
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
                ContextAndTag.Context = LastVmexitRip;
                KdHandleBreakpointAndDebugBreakpoints(DbgState,
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
        KdHandleBreakpointAndDebugBreakpoints(DbgState,
                                              DEBUGGEE_PAUSING_REASON_DEBUGGEE_HARDWARE_DEBUG_REGISTER_HIT,
                                              &ContextAndTag);
    }
}

/**
 * @brief before halting any core, all the tasks will be applied to all
 * cores including the main core
 * @details these tasks will be applied in vmx-root
 *
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
KdApplyTasksPreHaltCore(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // Check to unset mov to cr3 vm-exits
    //
    if (DbgState->ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent == TRUE)
    {
        //
        // Disable process change detection
        //
        ProcessEnableOrDisableThreadChangeMonitor(DbgState, FALSE);

        //
        // Avoid future sets/unsets
        //
        DbgState->ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent = FALSE;
        DbgState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt   = FALSE;
    }

    //
    // Check to unset change thread alerts
    //
    if (DbgState->ThreadOrProcessTracingDetails.InitialSetThreadChangeEvent == TRUE)
    {
        //
        // Disable thread change alerts
        //
        ThreadEnableOrDisableThreadChangeMonitor(DbgState, FALSE);

        //
        // Avoid future sets/unsets
        //
        DbgState->ThreadOrProcessTracingDetails.InitialSetThreadChangeEvent = FALSE;
        DbgState->ThreadOrProcessTracingDetails.InitialSetByClockInterrupt  = FALSE;
    }
}

/**
 * @brief before continue any core, all the tasks will be applied to all
 * cores including the main core
 * @details these tasks will be applied in vmx-root
 *
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
KdApplyTasksPostContinueCore(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // Check to apply hardware debug register breakpoints for step-over
    //
    if (DbgState->HardwareDebugRegisterForStepping != NULL)
    {
        SetDebugRegisters(DEBUGGER_DEBUG_REGISTER_FOR_STEP_OVER,
                          BREAK_ON_INSTRUCTION_FETCH,
                          FALSE,
                          DbgState->HardwareDebugRegisterForStepping);

        DbgState->HardwareDebugRegisterForStepping = NULL;
    }

    //
    // Check to apply mov to cr3 vm-exits
    //
    if (DbgState->ThreadOrProcessTracingDetails.InitialSetProcessChangeEvent == TRUE)
    {
        //
        // Enable process change detection
        //
        ProcessEnableOrDisableThreadChangeMonitor(DbgState, TRUE);
    }

    //
    // Check to apply thread change alerts
    //
    if (DbgState->ThreadOrProcessTracingDetails.InitialSetThreadChangeEvent == TRUE)
    {
        //
        // Enable alert for thread changes
        //
        ThreadEnableOrDisableThreadChangeMonitor(DbgState, TRUE);
    }
}

/**
 * @brief continue the debuggee, this function gurantees that all other cores
 * are continued (except current core)
 * @param DbgState The state of the debugger on the current core
 * @param SpeialEventResponse
 * @param PauseBreaksUntilSpecialMessageSent
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdContinueDebuggee(PROCESSOR_DEBUGGING_STATE *             DbgState,
                   BOOLEAN                                 PauseBreaksUntilSpecialMessageSent,
                   DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION SpeialEventResponse)
{
    if (PauseBreaksUntilSpecialMessageSent)
    {
        g_IgnoreBreaksToDebugger.PauseBreaksUntilSpecialMessageSent = TRUE;
        g_IgnoreBreaksToDebugger.SpeialEventResponse                = SpeialEventResponse;
    }

    //
    // Check if we should enable interrupts in this core or not,
    // we have another same check in SWITCHING CORES too
    //
    VmFuncCheckAndEnableExternalInterrupts(DbgState->CoreId);

    //
    // Unlock all the cores
    //
    ULONG CoreCount = KeQueryActiveProcessorCount(0);
    for (size_t i = 0; i < CoreCount; i++)
    {
        SpinlockUnlock(&g_DbgState[i].Lock);
    }
}

/**
 * @brief continue the debuggee, just the current operating core
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
KdContinueDebuggeeJustCurrentCore(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // In the case of any halting event, the processor won't send NMIs
    // to other cores if this field is set
    //
    DbgState->DoNotNmiNotifyOtherCoresByThisCore = TRUE;

    //
    // Unlock the current core
    //
    SpinlockUnlock(&DbgState->Lock);
}

/**
 * @brief read registers
 * @param DbgState The state of the debugger on the current core
 * @param ReadRegisterRequest
 *
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
KdReadRegisters(PROCESSOR_DEBUGGING_STATE * DbgState, PDEBUGGEE_REGISTER_READ_DESCRIPTION ReadRegisterRequest)
{
    GUEST_EXTRA_REGISTERS ERegs = {0};

    if (ReadRegisterRequest->RegisterID == DEBUGGEE_SHOW_ALL_REGISTERS)
    {
        //
        // Add General purpose registers
        //
        memcpy((void *)((CHAR *)ReadRegisterRequest + sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION)),
               DbgState->Regs,
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
        ReadRegisterRequest->Value = DebuggerGetRegValueWrapper(DbgState->Regs, ReadRegisterRequest->RegisterID);
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
 * @param DbgState The state of the debugger on the current core
 * @param NewCore
 * @return BOOLEAN
 */
BOOLEAN
KdSwitchCore(PROCESSOR_DEBUGGING_STATE * DbgState, UINT32 NewCore)
{
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
    VmFuncCheckAndEnableExternalInterrupts(DbgState->CoreId);

    //
    // Unset the current operating core (this is not important as if we
    // return from the operating function then the it will be unset
    // automatically but as we want to not have two operating cores
    // at the same time so we unset it here too)
    //
    DbgState->MainDebuggingCore = FALSE;

    //
    // Set new operating core
    //
    g_DbgState[NewCore].MainDebuggingCore = TRUE;

    //
    // Unlock the new core
    // *** We should not unlock the spinlock here as the other core might
    // simultaneously start sending packets and corrupt our packets ***
    //

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
    LogCallbackSendBuffer(OPERATION_COMMAND_FROM_DEBUGGER_CLOSE_AND_UNLOAD_VMM,
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
    LogCallbackSendBuffer(OPERATION_COMMAND_FROM_DEBUGGER_RELOAD_SYMBOL,
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
    LogCallbackSendBuffer(OPERATION_DEBUGGEE_USER_INPUT,
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

    FormatsPacket.Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
 * @param CoreId
 *
 * @return VOID
 */
VOID
KdSendCommandFinishedSignal(UINT32 CoreId)
{
    //
    // Halt other cores again
    //
    KdHandleBreakpointAndDebugBreakpointsCallback(CoreId,
                                                  DEBUGGEE_PAUSING_REASON_DEBUGGEE_COMMAND_EXECUTION_FINISHED,
                                                  NULL);
}

/**
 * @brief Tries to get the lock and won't return until successfully get the lock
 *
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdHandleHaltsWhenNmiReceivedFromVmxRoot(PROCESSOR_DEBUGGING_STATE * DbgState)
{
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
    KdHandleNmi(DbgState);

    //
    // Set the indication to false as we handled it
    //
    DbgState->NmiState.NmiCalledInVmxRootRelatedToHaltDebuggee = FALSE;
}

/**
 * @brief Tries to get the lock and won't return until successfully get the lock
 *
 * @param DbgState The state of the debugger on the current core
 * @param LONG Lock variable
 *
 * @return VOID
 */
VOID
KdCustomDebuggerBreakSpinlockLock(PROCESSOR_DEBUGGING_STATE * DbgState, volatile LONG * Lock)
{
    unsigned wait = 1;

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
        if (DbgState->NmiState.WaitingToBeLocked)
        {
            //
            // We should ignore one MTF as we touched MTF and it's not usable anymore
            //
            VmFuncChangeIgnoreOneMtfState(DbgState->CoreId, TRUE);

            //
            // Handle break of the core
            //
            if (DbgState->NmiState.NmiCalledInVmxRootRelatedToHaltDebuggee)
            {
                //
                // Handle it like an NMI is received from VMX root
                //
                KdHandleHaltsWhenNmiReceivedFromVmxRoot(DbgState);
            }
            else
            {
                //
                // Handle halt of the current core as an NMI
                //
                KdHandleNmi(DbgState);
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
 * @brief Handle broadcast NMIs for halting cores in vmx-root mode
 *
 * @param CoreId
 * @param IsOnVmxNmiHandler
 *
 * @return VOID
 */
VOID
KdHandleNmiBroadcastDebugBreaks(UINT32 CoreId, BOOLEAN IsOnVmxNmiHandler)
{
    //
    // Get the current debugging state
    //
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[CoreId];

    //
    // We use it as a global flag (for both vmx-root and vmx non-root), because
    // generally it doesn't have any use case in vmx-root (IsOnVmxNmiHandler == FALSE)
    // but in some cases, we might set the MTF but another vm-exit receives before
    // MTF and in that place if it tries to trigger and event, then the MTF is not
    // handled and the core is not locked properly, just waits to get the handle
    // of the "DebuggerHandleBreakpointLock", so we check this flag there
    //
    DbgState->NmiState.WaitingToBeLocked = TRUE;

    if (IsOnVmxNmiHandler)
    {
        //
        // Indicate that it's called from NMI handle, and it relates to
        // halting the debuggee
        //
        DbgState->NmiState.NmiCalledInVmxRootRelatedToHaltDebuggee = TRUE;

        //
        // If the core was in the middle of spinning on the spinlock
        // of getting the debug lock, this mechansim is not needed,
        // but if the core is not spinning there or the core is processing
        // a random vm-exit, then we inject an immediate vm-exit after vm-entry
        // or inject a DPC
        // this is used for two reasons.
        //
        //      1. first, we will get the registers (context) to halt the core
        //      2. second, it guarantees that if the NMI arrives within any
        //         instruction in vmx-root mode, then we injected an immediate
        //         vm-exit and we won't miss any cpu cycle in the guest
        //
        // KdFireDpc(KdHaltCoreInTheCaseOfHaltedFromNmiInVmxRoot, NULL);
        VmFuncSetMonitorTrapFlag(TRUE);
    }
    else
    {
        //
        // Handle core break
        //
        KdHandleNmi(DbgState);
    }
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root
 *
 * @param CoreId
 * @param Reason
 * @param EventDetails
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdHandleBreakpointAndDebugBreakpointsCallback(UINT32                            CoreId,
                                              DEBUGGEE_PAUSING_REASON           Reason,
                                              PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails)
{
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[CoreId];

    KdHandleBreakpointAndDebugBreakpoints(DbgState, Reason, EventDetails);
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root
 *
 * @param CoreId
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdHandleRegisteredMtfCallback(UINT32 CoreId)
{
    DEBUGGER_TRIGGERED_EVENT_DETAILS ContextAndTag = {0};
    //
    // Only 16 bit is needed howerver, vmwrite might write on other bits
    // and corrupt other variables, that's why we get 64bit
    //
    UINT64                      CsSel         = NULL;
    PROCESSOR_DEBUGGING_STATE * DbgState      = &g_DbgState[CoreId];
    UINT64                      LastVmexitRip = VmFuncGetLastVmexitRip(CoreId);

    //
    // Check if the cs selector changed or not, which indicates that the
    // execution changed from user-mode to kernel-mode or kernel-mode to
    // user-mode
    //
    CsSel = VmFuncGetCsSelector();

    KdCheckGuestOperatingModeChanges(DbgState->InstrumentationStepInTrace.CsSel,
                                     (UINT16)CsSel);

    //
    //  Unset the MTF flag and previous cs selector
    //
    DbgState->InstrumentationStepInTrace.CsSel = 0;

    //
    // Check and handle if there is a software defined breakpoint
    //
    if (!BreakpointCheckAndHandleDebuggerDefinedBreakpoints(DbgState,
                                                            LastVmexitRip,
                                                            DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
                                                            TRUE))
    {
        //
        // Handle the step
        //
        ContextAndTag.Context = LastVmexitRip;
        KdHandleBreakpointAndDebugBreakpoints(DbgState,
                                              DEBUGGEE_PAUSING_REASON_DEBUGGEE_STEPPED,
                                              &ContextAndTag);
    }
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root
 *
 * @param DbgState The state of the debugger on the current core
 * @param Reason
 * @param EventDetails
 *
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdHandleBreakpointAndDebugBreakpoints(PROCESSOR_DEBUGGING_STATE *       DbgState,
                                      DEBUGGEE_PAUSING_REASON           Reason,
                                      PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails)
{
    //
    // Lock handling breaks
    //
    KdCustomDebuggerBreakSpinlockLock(DbgState, &DebuggerHandleBreakpointLock);

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
    DbgState->MainDebuggingCore = TRUE;

    //
    // Lock current core
    //
    DbgState->NmiState.WaitingToBeLocked = FALSE;
    SpinlockLock(&DbgState->Lock);

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

    if (DbgState->DoNotNmiNotifyOtherCoresByThisCore == TRUE)
    {
        //
        // Unset to avoid future not notifying events
        //
        DbgState->DoNotNmiNotifyOtherCoresByThisCore = FALSE;
    }
    else
    {
        //
        // Make sure, nobody is in the middle of sending anything
        //
        SpinlockLock(&DebuggerResponseLock);

        //
        // Broadcast NMI with the intention of halting cores
        //
        VmFuncNmiBroadcastRequest(DbgState->CoreId);

        //
        // Unlock the sending response lock to perform regular debugging
        //
        SpinlockUnlock(&DebuggerResponseLock);
    }

    //
    // All the cores should go and manage through the following function
    //
    KdManageSystemHaltOnVmxRoot(DbgState, EventDetails);

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
    if (DbgState->MainDebuggingCore)
    {
        DbgState->MainDebuggingCore = FALSE;
        SpinlockUnlock(&DebuggerHandleBreakpointLock);
    }
}

/**
 * @brief Handle NMI vm-exits
 * @param CoreId
 *
 * @details This function should be called in vmx-root mode
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
KdCheckAndHandleNmiCallback(UINT32 CoreId)
{
    BOOLEAN                     Result   = FALSE;
    PROCESSOR_DEBUGGING_STATE * DbgState = &g_DbgState[CoreId];

    if (DbgState->NmiState.WaitingToBeLocked)
    {
        //
        // The NMI wait is handled here
        //
        Result = TRUE;

        //
        // Handle break of the core
        //
        if (DbgState->NmiState.NmiCalledInVmxRootRelatedToHaltDebuggee)
        {
            //
            // Handle it like an NMI is received from VMX root
            //
            KdHandleHaltsWhenNmiReceivedFromVmxRoot(DbgState);
        }
        else
        {
            //
            // Handle halt of the current core as an NMI
            //
            KdHandleNmi(DbgState);
        }
    }

    return Result;
}

/**
 * @brief Handle NMI Vm-exits
 * @param DbgState The state of the debugger on the current core
 *
 * @details This function should be called in vmx-root mode
 * @return VOID
 */
_Use_decl_annotations_
VOID
KdHandleNmi(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // Test
    //

    // LogInfo("NMI Arrived on : %d \n",CurrentProcessorIndex);

    //
    // Not the main debugging core
    //
    DbgState->MainDebuggingCore = FALSE;

    //
    // Lock current core
    //
    DbgState->NmiState.WaitingToBeLocked = FALSE;
    SpinlockLock(&DbgState->Lock);

    //
    // All the cores should go and manage through the following function
    //
    KdManageSystemHaltOnVmxRoot(DbgState, NULL);

    //
    // Unlock handling breaks
    //
    if (DbgState->MainDebuggingCore)
    {
        DbgState->MainDebuggingCore = FALSE;
        SpinlockUnlock(&DebuggerHandleBreakpointLock);
    }
}

/**
 * @brief apply a guaranteed step one instruction to the debuggee
 *
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
KdGuaranteedStepInstruction(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    //
    // Only 16 bit is needed howerver, vmwrite might write on other bits
    // and corrupt other variables, that's why we get 64bit
    //
    UINT64 CsSel = NULL;

    //
    // Read cs to have a trace of the execution mode of running application
    // in the debuggee
    //
    CsSel = VmFuncGetCsSelector();

    DbgState->InstrumentationStepInTrace.CsSel = (UINT16)CsSel;

    //
    // Set an indicator of a break in the case of an MTF
    //
    VmFuncRegisterMtfBreak(DbgState->CoreId);

    //
    // Not unset MTF again
    //
    VmFuncChangeMtfUnsettingState(DbgState->CoreId, TRUE);

    //
    // Disable external interrupts and interrupt Window
    //
    VmFuncDisableExternalInterruptsAndInterruptWindow(DbgState->CoreId);

    //
    // Set the MTF flag
    //
    VmFuncSetMonitorTrapFlag(TRUE);
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
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
KdRegularStepInInstruction(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    UINT32 Interruptibility;
    UINT32 InterruptibilityOld = NULL;
    RFLAGS Rflags              = {0};

    //
    // We're waiting for an step
    //
    DbgState->WaitForStepTrap = TRUE;

    //
    // Change guest trap flag
    //
    if (!DbgState->DisableTrapFlagOnContinue)
    {
        Rflags.AsUInt = VmFuncGetRflags();

        if (Rflags.TrapFlag == FALSE)
        {
            Rflags.TrapFlag = TRUE;

            VmFuncSetRflags(Rflags.AsUInt);

            DbgState->DisableTrapFlagOnContinue = TRUE;
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
    InterruptibilityOld = VmFuncGetInterruptibilityState();

    Interruptibility = InterruptibilityOld;

    Interruptibility = VmFuncClearSteppingBits(Interruptibility);

    if ((Interruptibility != InterruptibilityOld))
    {
        VmFuncSetInterruptibilityState(Interruptibility);
    }
}

/**
 * @brief Regualar step-over | step one instruction to the debuggee if
 * there is a call then it jumps the call
 *
 * @param DbgState The state of the debugger on the current core
 * @param IsNextInstructionACall
 * @param CallLength
 *
 * @return VOID
 */
VOID
KdRegularStepOver(PROCESSOR_DEBUGGING_STATE * DbgState, BOOLEAN IsNextInstructionACall, UINT32 CallLength)
{
    UINT64 NextAddressForHardwareDebugBp = 0;
    ULONG  CoreCount;

    if (IsNextInstructionACall)
    {
        //
        // It's a call, we should put a hardware debug register breakpoint
        // on the next instruction
        //
        DbgState->WaitForStepTrap     = TRUE;
        NextAddressForHardwareDebugBp = VmFuncGetLastVmexitRip(DbgState->CoreId) + CallLength;

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
            DbgState->HardwareDebugRegisterForStepping = NextAddressForHardwareDebugBp;
        }
    }
    else
    {
        //
        // Any instruction other than call (regular step)
        //
        KdRegularStepInInstruction(DbgState);
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
    LogCallbackSendBuffer(OPERATION_DEBUGGEE_REGISTER_EVENT,
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
    LogCallbackSendBuffer(OPERATION_DEBUGGEE_ADD_ACTION_TO_EVENT,
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
        if (g_DbgState[i].Lock)
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
        if (g_DbgState[i].NmiState.NmiCalledInVmxRootRelatedToHaltDebuggee)
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
 * @brief Perform modify the state of short-circuiting
 *
 * @param DbgState The state of the debugger on the current core
 * @param ShortCircuitingEvent
 *
 * @return VOID
 */
VOID
KdPerformSettingTheStateOfShortCircuiting(PROCESSOR_DEBUGGING_STATE * DbgState, PDEBUGGER_SHORT_CIRCUITING_EVENT ShortCircuitingEvent)
{
    //
    // Perform the short-circuiting changes
    //
    if (ShortCircuitingEvent->IsShortCircuiting)
    {
        DbgState->ShortCircuitingEvent = TRUE;
    }
    else
    {
        DbgState->ShortCircuitingEvent = FALSE;
    }

    //
    // The status was okay
    //
    ShortCircuitingEvent->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
            ModifyAndQueryEvent->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
        ModifyAndQueryEvent->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
        ModifyAndQueryEvent->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }
    else if (ModifyAndQueryEvent->TypeOfAction == DEBUGGER_MODIFY_EVENTS_CLEAR)
    {
        //
        // Send one byte buffer and operation codes
        //
        LogCallbackSendBuffer(OPERATION_DEBUGGEE_CLEAR_EVENTS,
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
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
KdDispatchAndPerformCommandsFromDebugger(PROCESSOR_DEBUGGING_STATE * DbgState)
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
    PDEBUGGER_SHORT_CIRCUITING_EVENT                    ShortCircuitingEventPacket;
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
                KdContinueDebuggee(DbgState, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

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
                    KdGuaranteedStepInstruction(DbgState);

                    //
                    // Unlock just on core
                    //
                    KdContinueDebuggeeJustCurrentCore(DbgState);

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
                    KdRegularStepInInstruction(DbgState);

                    //
                    // Unlock other cores
                    //
                    KdContinueDebuggee(DbgState, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

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
                    KdRegularStepOver(DbgState, SteppingPacket->IsCurrentInstructionACall, SteppingPacket->CallLength);

                    //
                    // Unlock other cores
                    //
                    KdContinueDebuggee(DbgState, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

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
                KdContinueDebuggee(DbgState, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

                //
                // No need to wait for new commands
                //
                EscapeFromTheLoop = TRUE;

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_CHANGE_CORE:

                ChangeCorePacket = (DEBUGGEE_CHANGE_CORE_PACKET *)(((CHAR *)TheActualPacket) +
                                                                   sizeof(DEBUGGER_REMOTE_PACKET));

                if (DbgState->CoreId != ChangeCorePacket->NewCore)
                {
                    //
                    // Switch to new core
                    //
                    if (KdSwitchCore(DbgState, ChangeCorePacket->NewCore))
                    {
                        //
                        // No need to wait for new commands
                        //
                        EscapeFromTheLoop = TRUE;

                        //
                        // Unlock the new core
                        //
                        UnlockTheNewCore = TRUE;

                        ChangeCorePacket->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
                    ChangeCorePacket->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
                    SpinlockUnlock(&g_DbgState[ChangeCorePacket->NewCore].Lock);
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
                    CallstackPacket->BaseAddress = DbgState->Regs;
                }

                //
                // Feel the callstack frames the buffers
                //
                if (CallstackWalkthroughStack(CallstackFrameBuffer,
                                              CallstackPacket->BaseAddress,
                                              CallstackPacket->Size,
                                              CallstackPacket->Is32Bit))
                {
                    CallstackPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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

                    TestQueryPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

                    break;

                default:

                    //
                    // Query index not found
                    //
                    TestQueryPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

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
                if (KdReadRegisters(DbgState, ReadRegisterPacket))
                {
                    ReadRegisterPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
                    ReadMemoryPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
                    EditMemoryPacket->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
                if (DebuggerPerformRunScript(DbgState,
                                             OPERATION_LOG_INFO_MESSAGE /* simple print */,
                                             NULL,
                                             ScriptPacket,
                                             g_DebuggeeHaltContext))
                {
                    //
                    // Set status
                    //
                    ScriptPacket->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
                KdContinueDebuggee(DbgState, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);
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
                    SearchPacketResult.Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
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
                KdContinueDebuggee(DbgState, TRUE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_REGISTERING_EVENT);
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
                KdContinueDebuggee(DbgState, TRUE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_ADDING_ACTION_TO_EVENT);
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
                    KdContinueDebuggee(DbgState, TRUE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_QUERY_AND_MODIFY_EVENT);
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

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_SET_SHORT_CIRCUITING_STATE:

                ShortCircuitingEventPacket = (DEBUGGER_SHORT_CIRCUITING_EVENT *)(((CHAR *)TheActualPacket) +
                                                                                 sizeof(DEBUGGER_REMOTE_PACKET));

                //
                // Perform the action
                //
                KdPerformSettingTheStateOfShortCircuiting(DbgState, ShortCircuitingEventPacket);

                //
                // Send the response of short-circuiting event
                //
                KdResponsePacketToDebugger(DEBUGGER_REMOTE_PACKET_TYPE_DEBUGGEE_TO_DEBUGGER,
                                           DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_DEBUGGEE_RESULT_OF_SHORT_CIRCUITING_STATE,
                                           ShortCircuitingEventPacket,
                                           sizeof(DEBUGGER_SHORT_CIRCUITING_EVENT));

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
                KdContinueDebuggee(DbgState, FALSE, DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_NO_ACTION);

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
    CsSel = VmFuncGetCsSelector();

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
 * @details This function should only be called from KdHandleBreakpointAndDebugBreakpoints
 * @param DbgState The state of the debugger on the current core
 * @param EventDetails
 * @param MainCore the core that triggered the event
 *
 * @return VOID
 */
VOID
KdManageSystemHaltOnVmxRoot(PROCESSOR_DEBUGGING_STATE *       DbgState,
                            PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails)
{
    DEBUGGEE_KD_PAUSED_PACKET PausePacket;
    ULONG                     ExitInstructionLength  = 0;
    UINT64                    SizeOfSafeBufferToRead = 0;
    RFLAGS                    Rflags                 = {0};
    UINT64                    LastVmexitRip          = 0;

    //
    // Perform Pre-halt tasks
    //
    KdApplyTasksPreHaltCore(DbgState);

StartAgain:

    //
    // We check for receiving buffer (unhalting) only on the
    // first core and not on every cores
    //
    if (DbgState->MainDebuggingCore)
    {
        //
        // *** Current Operating Core  ***
        //
        RtlZeroMemory(&PausePacket, sizeof(DEBUGGEE_KD_PAUSED_PACKET));

        //
        // Get the last RIP for vm-exit handler
        //
        LastVmexitRip = VmFuncGetLastVmexitRip(DbgState->CoreId);

        //
        // Set the halt reason
        //
        PausePacket.PausingReason = g_DebuggeeHaltReason;

        //
        // Set the current core
        //
        PausePacket.CurrentCore = DbgState->CoreId;

        //
        // Set the RIP and mode of execution
        //
        PausePacket.Rip            = LastVmexitRip;
        PausePacket.Is32BitAddress = KdIsGuestOnUsermode32Bit();

        //
        // Set rflags for finding the results of conditional jumps
        //
        Rflags.AsUInt      = VmFuncGetRflags();
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
        if (DbgState->InstructionLengthHint != 0)
        {
            ExitInstructionLength = DbgState->InstructionLengthHint;
        }
        else
        {
            //
            // Reading instruction length (VMCS_VMEXIT_INSTRUCTION_LENGTH) proved to provide wrong results,
            // so we won't use it anymore
            //

            //
            // Compute the amount of buffer we can read without problem
            //
            SizeOfSafeBufferToRead = LastVmexitRip & 0xfff;
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
        MemoryMapperReadMemorySafeOnTargetProcess(LastVmexitRip,
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
        KdDispatchAndPerformCommandsFromDebugger(DbgState);

        //
        // Check if it's a change core event or not, otherwise finish the execution
        // and continue debuggee
        //
        if (!DbgState->MainDebuggingCore)
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
        DbgState->NmiState.WaitingToBeLocked = FALSE;

        ScopedSpinlock(
            DbgState->Lock,
            //
            // Check if it's a change core event or not
            //
            if (DbgState->MainDebuggingCore) {
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
    KdApplyTasksPostContinueCore(DbgState);
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
    VmFuncVmxVmcall(DEBUGGER_VMCALL_VM_EXIT_HALT_SYSTEM, 0, 0, 0);

    //
    // Set the status
    //
    PausePacket->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
}
