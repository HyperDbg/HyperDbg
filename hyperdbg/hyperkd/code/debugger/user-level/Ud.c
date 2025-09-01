/**
 * @file Ud.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines related to user mode debugging
 * @details
 * @version 0.1
 * @date 2022-01-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief initialize user debugger
 * @details this function should be called on vmx non-root
 *
 * @return BOOLEAN
 */
BOOLEAN
UdInitializeUserDebugger()
{
    //
    // Check if it's already initialized or not, we'll ignore it if it's
    // previously initialized
    //
    if (g_UserDebuggerState)
    {
        return TRUE;
    }

    //
    // Configure the exec-trap on all processors
    //
    if (!ConfigureInitializeExecTrapOnAllProcessors())
    {
        return FALSE;
    }

    //
    // Check if we have functions we need for attaching mechanism
    //
    if (g_PsGetProcessPeb == NULL || g_PsGetProcessWow64Process == NULL || g_ZwQueryInformationProcess == NULL)
    {
        LogError("Err, unable to find needed functions for user-debugger");
        // return FALSE;
    }

    //
    // Start the seed of user-mode debugging thread
    //
    g_SeedOfUserDebuggingDetails = DebuggerThreadDebuggingTagStartSeed;

    //
    // Initialize the thread debugging details list
    //
    InitializeListHead(&g_ProcessDebuggingDetailsListHead);

    //
    // Enable vm-exit on Hardware debug exceptions and breakpoints
    // so, intercept #DBs and #BP by changing exception bitmap (one core)
    //
    BroadcastEnableDbAndBpExitingAllCores();

    //
    // Request to allocate buffers for thread holder of threads
    //
    ThreadHolderAllocateThreadHoldingBuffers();

    //
    // Initialize command waiting event
    //
    SynchronizationInitializeEvent(&g_UserDebuggerWaitingCommandEvent);

    //
    // Indicate that the user debugger is active
    //
    g_UserDebuggerState = TRUE;

    return TRUE;
}

/**
 * @brief uninitialize user debugger
 * @details this function should be called on vmx non-root
 *
 * @return VOID
 */
VOID
UdUninitializeUserDebugger()
{
    if (g_UserDebuggerState)
    {
        //
        // Indicate that the user debugger is not active
        //
        g_UserDebuggerState = FALSE;

        //
        // Uninitialize the exec-trap on all processors
        //
        ConfigureUninitializeExecTrapOnAllProcessors();

        //
        // Free and deallocate all the buffers (pools) relating to
        // thread debugging details
        //
        AttachingRemoveAndFreeAllProcessDebuggingDetails();
    }
}

/**
 * @brief Handle cases where we instant break is needed on the user debugger
 *
 * @param DbgState The state of the debugger on the current core
 * @param Reason The reason of the pausing
 * @param ProcessDebuggingDetail
 *
 * @return BOOLEAN
 */
BOOLEAN
UdHandleInstantBreak(PROCESSOR_DEBUGGING_STATE *         DbgState,
                     DEBUGGEE_PAUSING_REASON             Reason,
                     PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail)
{
    //
    // Check if the process debugging details is available or not
    //
    if (ProcessDebuggingDetail == NULL)
    {
        //
        // If the process debugging detail is not available, we should
        // find it by the current process id
        //
        ProcessDebuggingDetail = AttachingFindProcessDebuggingDetailsByProcessId(HANDLE_TO_UINT32(PsGetCurrentProcessId()));

        if (ProcessDebuggingDetail == NULL)
        {
            //
            // If we reached here, it means that the process debugging detail is not found
            // so, we should return FALSE to indicate that we couldn't handle the instant break
            //
            return FALSE;
        }
    }

    //
    // Add the process to the watching list to be able to intercept the threads
    //
    if (AttachingConfigureInterceptingThreads(ProcessDebuggingDetail->Token, TRUE))
    {
        //
        // Since the adding it to the watching list will take effect from the next
        // CR3 vm-exit, we should change the state of the core to prevent further execution
        //
        ConfigureExecTrapApplyMbecConfiguratinFromKernelSide(DbgState->CoreId);

        //
        // Handling state through the user-mode debugger
        //
        return UdCheckAndHandleBreakpointsAndDebugBreaks(DbgState,
                                                         Reason,
                                                         NULL);
    }

    //
    // If we reached here, it means that we couldn't add the process to the
    //
    return FALSE;
}

/**
 * @brief Apply hardware debug registers to all cores
 *
 * @param TargetAddress
 *
 * @return VOID
 */
VOID
UdApplyHardwareDebugRegister(PVOID TargetAddress)
{
    SetDebugRegisters(DEBUGGER_DEBUG_REGISTER_FOR_STEP_OVER,
                      BREAK_ON_INSTRUCTION_FETCH,
                      FALSE,
                      (UINT64)TargetAddress);
}

/**
 * @brief Send the result of formats command to the user debugger
 * @param Value
 *
 * @return VOID
 */
VOID
UdSendFormatsFunctionResult(UINT64 Value)
{
    g_UserDebuggerFormatsResultPacket.Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    g_UserDebuggerFormatsResultPacket.Value  = Value;
}

/**
 * @brief routines to broadcast setting hardware debug registers on all cores
 * @param TargetAddress
 *
 * @return VOID
 */
VOID
UdBroadcastSetHardwareDebugRegistersAllCores(PVOID TargetAddress)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineSetHardwareDebugRegisters, TargetAddress);
}

/**
 * @brief Regular step-over, step one instruction to the debuggee on user debugger if
 * there is a call then it jumps the call
 *
 * @param LastRip Last RIP register
 * @param IsNextInstructionACall
 * @param CallLength
 *
 * @return VOID
 */
VOID
UdRegularStepOver(UINT64 LastRip, BOOLEAN IsNextInstructionACall, UINT32 CallLength)
{
    UINT64 NextAddressForHardwareDebugBp = 0;

    // LogInfo("Last Rip: %llx, IsNextInstructionACall: %s, Call length: %x",
    //         LastRip,
    //         IsNextInstructionACall ? "true" : "false",
    //         CallLength);

    if (IsNextInstructionACall)
    {
        //
        // It's a call, we should put a hardware debug register breakpoint
        // on the next instruction
        //
        NextAddressForHardwareDebugBp = LastRip + CallLength;

        //
        // Broadcast to apply hardware debug registers to all cores
        //
        UdBroadcastSetHardwareDebugRegistersAllCores((PVOID)NextAddressForHardwareDebugBp);
    }
    else
    {
        //
        // Any instruction other than call (regular step)
        //
        TracingRegularStepInInstruction();
    }
}

/**
 * @brief Handles debug events when user-debugger is attached
 *
 * @param DbgState The state of the debugger on the current core
 * @param TrapSetByDebugger Shows whether a trap set by debugger or not
 *
 * @return BOOLEAN
 */
BOOLEAN
UdHandleDebugEventsWhenUserDebuggerIsAttached(PROCESSOR_DEBUGGING_STATE * DbgState,
                                              BOOLEAN                     TrapSetByDebugger)
{
    UNREFERENCED_PARAMETER(TrapSetByDebugger);

    if (UdHandleInstantBreak(DbgState,
                             DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_DEBUG_BREAK,
                             NULL))
    {
        //
        // Handled by user debugger
        //
        return TRUE;
    }
    else
    {
        //
        // Not handled by user debugger
        //
        return FALSE;
    }
}

/**
 * @brief Perform stepping though the instructions in the target thread
 *
 * @param DbgState The state of the debugger on the current core
 * @param ProcessDebuggingDetail
 * @param ThreadDebuggingDetails
 * @param SteppingType
 * @param IsCurrentInstructionACall
 * @param CallInstructionSize
 *
 * @return VOID
 */
VOID
UdStepInstructions(PROCESSOR_DEBUGGING_STATE *         DbgState,
                   PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail,
                   PUSERMODE_DEBUGGING_THREAD_DETAILS  ThreadDebuggingDetails,
                   DEBUGGER_REMOTE_STEPPING_REQUEST    SteppingType,
                   BOOLEAN                             IsCurrentInstructionACall,
                   UINT32                              CallInstructionSize)
{
    UNREFERENCED_PARAMETER(ThreadDebuggingDetails);

    switch (SteppingType)
    {
    case DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN:

        //
        // Apply step-in (t command)
        //
        TracingRegularStepInInstruction();

        //
        // Continue the debuggee process
        //
        if (AttachingConfigureInterceptingThreads(ProcessDebuggingDetail->Token, FALSE))
        {
            //
            // Unpause the threads of the target process
            //
            ThreadHolderUnpauseAllThreadsInProcess(ProcessDebuggingDetail);
        }

        break;

    case DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER:

        //
        // Apply Step-over (p command)
        //
        UdRegularStepOver(
            VmFuncGetLastVmexitRip(DbgState->CoreId),
            IsCurrentInstructionACall,
            CallInstructionSize);

        //
        // Continue the debuggee process
        //
        if (AttachingConfigureInterceptingThreads(ProcessDebuggingDetail->Token, FALSE))
        {
            //
            // Unpause the threads of the target process
            //
            ThreadHolderUnpauseAllThreadsInProcess(ProcessDebuggingDetail);
        }

        break;

    default:
        break;
    }
}

/**
 * @brief Perform reading register(s) in the target thread
 *
 * @param DbgState The state of the debugger on the current core
 * @param RegisterId
 *
 * @return VOID
 */
VOID
UdReadRegisters(PROCESSOR_DEBUGGING_STATE * DbgState,
                UINT32                      RegisterId)
{
    UNREFERENCED_PARAMETER(DbgState);
    UNREFERENCED_PARAMETER(RegisterId);

    PDEBUGGER_UD_COMMAND_PACKET         ActionRequest;
    PDEBUGGEE_REGISTER_READ_DESCRIPTION RegDesc;

    //
    // Recover the action request buffer and optional storage buffer
    //
    ActionRequest = (DEBUGGER_UD_COMMAND_PACKET *)g_UserDebuggerWaitingCommandBuffer;
    RegDesc       = (PDEBUGGEE_REGISTER_READ_DESCRIPTION)((UINT8 *)g_UserDebuggerWaitingCommandBuffer + sizeof(DEBUGGER_UD_COMMAND_PACKET));

    //
    // *** Here, we should read the registers and put them in the optional storage buffer ***
    //
    DebuggerCommandReadRegisters(DbgState->Regs, RegDesc);

    //
    // Set the register request result
    //
    RegDesc->KernelStatus = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    //
    // Set the action request result
    //
    ActionRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    //
    // Set the event to indicate that the command is completed
    //
    SynchronizationSetEvent(&g_UserDebuggerWaitingCommandEvent);
}

/**
 * @brief Perform running script in the target thread
 *
 * @param DbgState The state of the debugger on the current core
 *
 * @return VOID
 */
VOID
UdRunScript(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    PDEBUGGER_UD_COMMAND_PACKET ActionRequest;
    DEBUGGEE_SCRIPT_PACKET *    ScriptPacket;

    //
    // Recover the action request buffer and optional storage buffer
    //
    ActionRequest = (DEBUGGER_UD_COMMAND_PACKET *)g_UserDebuggerWaitingCommandBuffer;
    ScriptPacket  = (DEBUGGEE_SCRIPT_PACKET *)((UINT8 *)g_UserDebuggerWaitingCommandBuffer + sizeof(DEBUGGER_UD_COMMAND_PACKET));

    //
    // *** Here, we should execute script buffer and put them in the optional storage buffer ***
    //

    //
    // Run the script in the target process (thread)
    //
    if (DebuggerPerformRunScript(DbgState,
                                 NULL,
                                 ScriptPacket,
                                 &g_EventTriggerDetail))
    {
        //
        // Check if we need to format the output or not
        //
        if (ScriptPacket->IsFormat)
        {
            //
            // For the format, we need to get the result from the script engine
            // through global variables, we store the result of format into
            // optional parameters
            //
            ScriptPacket->Result      = g_UserDebuggerFormatsResultPacket.Result;
            ScriptPacket->FormatValue = g_UserDebuggerFormatsResultPacket.Value;
        }
        else
        {
            //
            // Set status
            //
            ScriptPacket->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
        }
    }
    else
    {
        //
        // Set status
        //
        ScriptPacket->Result = DEBUGGER_ERROR_PREPARING_DEBUGGEE_TO_RUN_SCRIPT;
    }

    //
    // Set the action request result
    //
    ActionRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;

    //
    // Set the event to indicate that the command is completed
    //
    SynchronizationSetEvent(&g_UserDebuggerWaitingCommandEvent);
}

/**
 * @brief Perform the user-mode commands
 *
 * @param DbgState The state of the debugger on the current core
 * @param ProcessDebuggingDetails
 * @param ThreadDebuggingDetails
 * @param UserAction
 * @param OptionalParam1
 * @param OptionalParam2
 * @param OptionalParam3
 * @param OptionalParam4
 *
 * @return BOOLEAN
 */
BOOLEAN
UdPerformCommand(PROCESSOR_DEBUGGING_STATE *         DbgState,
                 PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail,
                 PUSERMODE_DEBUGGING_THREAD_DETAILS  ThreadDebuggingDetails,
                 DEBUGGER_UD_COMMAND_ACTION_TYPE     UserAction,
                 UINT64                              OptionalParam1,
                 UINT64                              OptionalParam2,
                 UINT64                              OptionalParam3,
                 UINT64                              OptionalParam4)
{
    UNREFERENCED_PARAMETER(OptionalParam4);

    //
    // Perform the command
    //
    switch (UserAction)
    {
    case DEBUGGER_UD_COMMAND_ACTION_TYPE_REGULAR_STEP:

        //
        // Stepping through the instructions
        //
        UdStepInstructions(DbgState,
                           ProcessDebuggingDetail,
                           ThreadDebuggingDetails,
                           (DEBUGGER_REMOTE_STEPPING_REQUEST)OptionalParam1,
                           (BOOLEAN)OptionalParam2,
                           (UINT32)OptionalParam3);

        break;

    case DEBUGGER_UD_COMMAND_ACTION_TYPE_READ_REGISTERS:

        //
        // Read the registers
        //
        UdReadRegisters(DbgState,
                        (UINT32)OptionalParam1);

        break;

    case DEBUGGER_UD_COMMAND_ACTION_TYPE_EXECUTE_SCRIPT_BUFFER:

        //
        // Execute the script buffer
        //
        UdRunScript(DbgState);

        break;

    default:

        //
        // Invalid user action
        //
        return FALSE;

        break;
    }

    return TRUE;
}

/**
 * @brief Check for the user-mode commands
 *
 * @param DbgState The state of the debugger on the current core
 * @param ProcessDebuggingDetail
 *
 * @return BOOLEAN
 */
BOOLEAN
UdCheckForCommand(PROCESSOR_DEBUGGING_STATE *         DbgState,
                  PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetail)
{
    PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails;
    BOOLEAN                            CommandFound = FALSE;

    ThreadDebuggingDetails = ThreadHolderGetProcessThreadDetailsByProcessIdAndThreadId(HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                                       HANDLE_TO_UINT32(PsGetCurrentThreadId()));

    if (!ThreadDebuggingDetails)
    {
        return FALSE;
    }

    //
    // If we reached here, the current thread is in debugger attached mechanism
    // now we check whether it's paused or not
    //
    if (!ThreadDebuggingDetails->IsPaused)
    {
        return FALSE;
    }

    //
    // Here, we're sure that this thread is looking for command, let
    // see if we find anything
    //
    for (size_t i = 0; i < MAX_USER_ACTIONS_FOR_THREADS; i++)
    {
        if (ThreadDebuggingDetails->UdAction[i].ActionType != DEBUGGER_UD_COMMAND_ACTION_TYPE_NONE)
        {
            //
            // We found a command for this thread
            //
            CommandFound = TRUE;

            //
            // Perform the command
            //
            UdPerformCommand(DbgState,
                             ProcessDebuggingDetail,
                             ThreadDebuggingDetails,
                             ThreadDebuggingDetails->UdAction[i].ActionType,
                             ThreadDebuggingDetails->UdAction[i].OptionalParam1,
                             ThreadDebuggingDetails->UdAction[i].OptionalParam2,
                             ThreadDebuggingDetails->UdAction[i].OptionalParam3,
                             ThreadDebuggingDetails->UdAction[i].OptionalParam4);

            //
            // Remove the command
            //
            ThreadDebuggingDetails->UdAction[i].OptionalParam1 = (UINT64)NULL;
            ThreadDebuggingDetails->UdAction[i].OptionalParam2 = (UINT64)NULL;
            ThreadDebuggingDetails->UdAction[i].OptionalParam3 = (UINT64)NULL;
            ThreadDebuggingDetails->UdAction[i].OptionalParam4 = (UINT64)NULL;

            //
            // At last disable it
            //
            ThreadDebuggingDetails->UdAction[i].ActionType = DEBUGGER_UD_COMMAND_ACTION_TYPE_NONE;

            //
            // only one command at a time
            //
            break;
        }
    }

    //
    // Return whether we found a command or not
    //
    return CommandFound;
}

/**
 * @brief Dispatch the user-mode commands
 *
 * @param ActionRequest
 * @param ActionRequestInputLength
 * @param ActionRequestOutputLength
 *
 * @return BOOLEAN
 */
BOOLEAN
UdDispatchUsermodeCommands(PDEBUGGER_UD_COMMAND_PACKET ActionRequest,
                           UINT32                      ActionRequestInputLength,
                           UINT32                      ActionRequestOutputLength)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails;
    BOOLEAN                             Result;

    //
    // Find the thread debugging detail of the thread
    //
    ProcessDebuggingDetails = AttachingFindProcessDebuggingDetailsByToken(ActionRequest->ProcessDebuggingDetailToken);

    if (!ProcessDebuggingDetails)
    {
        //
        // Token not found!
        //
        ActionRequest->Result = DEBUGGER_ERROR_INVALID_THREAD_DEBUGGING_TOKEN;

        return FALSE;
    }

    //
    // Check if this command needs the action request to be waited for completion or not
    //
    if (ActionRequest->WaitForEventCompletion)
    {
        //
        // Set the command event buffer
        //
        g_UserDebuggerWaitingCommandBuffer = (PVOID)ActionRequest;

        //
        // Set the input command buffer length
        //
        g_UserDebuggerWaitingCommandInputBufferLength = ActionRequestInputLength;

        //
        // Set the output command buffer length
        //
        g_UserDebuggerWaitingCommandOutputBufferLength = ActionRequestOutputLength;
    }

    //
    // Apply the command to all threads or just one thread
    //
    Result = ThreadHolderApplyActionToPausedThreads(ProcessDebuggingDetails, ActionRequest);

    //
    // Check if we need to wait for the command event completion or not
    //
    if (Result && ActionRequest->WaitForEventCompletion)
    {
        //
        // Wait for the command to be completed
        //
        SynchronizationWaitForEvent(&g_UserDebuggerWaitingCommandEvent);
    }

    //
    // Since the command is applied successfully, we can set the result
    // Note that if the command contains another layer of optional buffers
    // the result of that optional buffer might be different than this result
    // this one is just for applying the command itself
    //
    if (Result)
    {
        //
        // If we are not waiting for the event completion, we should set the
        // result of the action request here as we successfully applied the command
        //
        ActionRequest->Result = DEBUGGER_OPERATION_WAS_SUCCESSFUL;
    }
    else
    {
        ActionRequest->Result = DEBUGGER_ERROR_UNABLE_TO_APPLY_COMMAND_TO_THE_TARGET_THREAD;
    }

    return Result;
}

/**
 * @brief Set the thread pausing state
 *
 * @param ThreadDebuggingDetails
 * @param ProcessDebuggingDetails
 * @param InstructionBytesBuffer
 * @param SizeOfInstruction
 *
 * @return VOID
 */
VOID
UdSetThreadPausingState(PUSERMODE_DEBUGGING_THREAD_DETAILS  ThreadDebuggingDetails,
                        PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails,
                        PVOID                               InstructionBytesBuffer,
                        UINT32                              SizeOfInstruction)
{
    UNREFERENCED_PARAMETER(ProcessDebuggingDetails);

    //
    // Save the RIP for future return
    //
    ThreadDebuggingDetails->ThreadRip = VmFuncGetRip();

    //
    // Set the number of context switches to one (reset it if already set)
    //
    ThreadDebuggingDetails->NumberOfBlockedContextSwitches = 1;

    //
    // Indicate that it's spinning
    //
    ThreadDebuggingDetails->IsPaused = TRUE;

    //
    // Set size of instruction
    //
    ThreadDebuggingDetails->SizeOfInstruction = SizeOfInstruction;

    //
    // Copy the current running instruction
    //
    memcpy(&ThreadDebuggingDetails->InstructionBytesOnRip[0], InstructionBytesBuffer, SizeOfInstruction);
}

/**
 * @brief Handle special reasons pre-pausings
 * @details This function can be used in vmx-root
 *
 * @param DbgState The state of the debugger on the current core
 * @param ThreadDebuggingDetails
 * @param Reason
 * @param EventDetails
 * @return VOID
 */
VOID
UdPrePausingReasons(PROCESSOR_DEBUGGING_STATE *        DbgState,
                    PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails,
                    DEBUGGEE_PAUSING_REASON            Reason,
                    PDEBUGGER_TRIGGERED_EVENT_DETAILS  EventDetails)

{
    UNREFERENCED_PARAMETER(DbgState);
    UNREFERENCED_PARAMETER(ThreadDebuggingDetails);
    UNREFERENCED_PARAMETER(EventDetails);

    //
    // *** Handle events before pausing ***
    //
    switch (Reason)
    {
    case DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_DEBUG_BREAK:

        break;

    default:
        break;
    }
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root
 *
 * @param DbgState The state of the debugger on the current core
 * @param Reason
 * @param EventDetails
 * @return BOOLEAN
 */
BOOLEAN
UdCheckAndHandleBreakpointsAndDebugBreaks(PROCESSOR_DEBUGGING_STATE *       DbgState,
                                          DEBUGGEE_PAUSING_REASON           Reason,
                                          PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails)
{
    DEBUGGEE_UD_PAUSED_PACKET           PausePacket;
    ULONG                               ExitInstructionLength   = 0;
    UINT32                              SizeOfSafeBufferToRead  = 0;
    RFLAGS                              Rflags                  = {0};
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails = NULL;
    PUSERMODE_DEBUGGING_THREAD_DETAILS  ThreadDebuggingDetails  = NULL;
    UINT64                              LastVmexitRip           = VmFuncGetLastVmexitRip(DbgState->CoreId);

    //
    // Breaking only supported  if user-debugger is loaded
    //
    if (!g_UserDebuggerState)
    {
        return FALSE;
    }

    //
    // Check entry of paused thread
    //
    ProcessDebuggingDetails = AttachingFindProcessDebuggingDetailsByProcessId(HANDLE_TO_UINT32(PsGetCurrentProcessId()));

    if (!ProcessDebuggingDetails)
    {
        //
        // Token not found!
        //
        return FALSE;
    }

    //
    // Find the thread entry and if not found, create one for it
    //
    ThreadDebuggingDetails = ThreadHolderFindOrCreateThreadDebuggingDetail(HANDLE_TO_UINT32(PsGetCurrentThreadId()), ProcessDebuggingDetails);

    if (!ThreadDebuggingDetails)
    {
        //
        // Sth went wrong!
        //
        return FALSE;
    }

    //
    // Check if the thread is already paused or not
    //
    if (ThreadDebuggingDetails->IsPaused)
    {
        //
        // Increase the number of context switches
        //
        ThreadDebuggingDetails->NumberOfBlockedContextSwitches++;

        //
        // The thread is already paused, so we don't need to pause it again
        //
        return TRUE;
    }

    //
    // Set it as active thread debugging
    //
    ProcessDebuggingDetails->ActiveThreadId = ThreadDebuggingDetails->ThreadId;

    //
    // Perform the pre-pausing tasks
    //
    UdPrePausingReasons(DbgState, ThreadDebuggingDetails, Reason, EventDetails);

    //
    // *** Fill the pausing structure ***
    //

    RtlZeroMemory(&PausePacket, sizeof(DEBUGGEE_UD_PAUSED_PACKET));

    //
    // Set the pausing reason
    //
    PausePacket.PausingReason = Reason;

    //
    // Set process debugging information
    //
    PausePacket.ProcessId             = HANDLE_TO_UINT32(PsGetCurrentProcessId());
    PausePacket.ThreadId              = HANDLE_TO_UINT32(PsGetCurrentThreadId());
    PausePacket.ProcessDebuggingToken = ProcessDebuggingDetails->Token;

    //
    // Set the RIP and mode of execution
    //
    PausePacket.Rip     = LastVmexitRip;
    PausePacket.Is32Bit = KdIsGuestOnUsermode32Bit();

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
        PausePacket.EventTag          = EventDetails->Tag;
        PausePacket.EventCallingStage = EventDetails->Stage;
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
        // Reading instruction length (VMCS_VMEXIT_INSTRUCTION_LENGTH) proved to
        // provide wrong results, so we won't use it
        //

        //
        // Compute the amount of buffer we can read without problem
        //
        SizeOfSafeBufferToRead = (UINT32)(LastVmexitRip & 0xfff);
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
    PausePacket.ReadInstructionLen = (UINT16)ExitInstructionLength;

    //
    // Find the current instruction
    //
    MemoryMapperReadMemorySafeOnTargetProcess(LastVmexitRip,
                                              &PausePacket.InstructionBytesOnRip,
                                              ExitInstructionLength);

    //
    // Send the pause packet, along with RIP and an indication
    // to pause to the user debugger
    //
    LogCallbackSendBuffer(OPERATION_NOTIFICATION_FROM_USER_DEBUGGER_PAUSE,
                          &PausePacket,
                          sizeof(DEBUGGEE_UD_PAUSED_PACKET),
                          TRUE);

    //
    // Set the thread debugging details
    //
    UdSetThreadPausingState(ThreadDebuggingDetails,
                            ProcessDebuggingDetails,
                            &PausePacket.InstructionBytesOnRip,
                            ExitInstructionLength);

    //
    // Everything was okay
    //
    return TRUE;
}
