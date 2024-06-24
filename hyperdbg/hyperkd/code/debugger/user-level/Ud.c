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
        // Free and deallocate all the buffers (pools) relating to
        // thread debugging details
        //
        AttachingRemoveAndFreeAllProcessDebuggingDetails();
    }
}

/**
 * @brief Restore the thread to the original direction
 *
 * @param ThreadDebuggingDetails
 *
 * @return VOID
 */
VOID
UdRestoreToOriginalDirection(PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails)
{
    //
    // Configure the RIP again
    //
    VmFuncSetRip(ThreadDebuggingDetails->ThreadRip);
}

/**
 * @brief Continue the thread
 *
 * @param ThreadDebuggingDetails
 *
 * @return VOID
 */
VOID
UdContinueThread(PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails)
{
    //
    // Configure the RIP and RSP again
    //
    UdRestoreToOriginalDirection(ThreadDebuggingDetails);

    //
    // Continue the current instruction won't pass it
    //
    VmFuncSuppressRipIncrement(KeGetCurrentProcessorNumberEx(NULL));

    //
    // It's not paused anymore!
    //
    ThreadDebuggingDetails->IsPaused = FALSE;
}

/**
 * @brief Perform stepping though the instructions in target thread
 *
 * @param ThreadDebuggingDetails
 *
 * @return VOID
 */
VOID
UdStepInstructions(PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails,
                   DEBUGGER_REMOTE_STEPPING_REQUEST   SteppingType)
{
    //
    // Configure the RIP
    //
    UdRestoreToOriginalDirection(ThreadDebuggingDetails);

    switch (SteppingType)
    {
    case DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_IN:

        //
        // Set the trap-flag
        //
        VmFuncSetRflagTrapFlag(TRUE);

        //
        // Indicate that we should set the trap flag to the FALSE next time on
        // the same process/thread
        //
        if (!BreakpointRestoreTheTrapFlagOnceTriggered(HANDLE_TO_UINT32(PsGetCurrentProcessId()), HANDLE_TO_UINT32(PsGetCurrentThreadId())))
        {
            LogWarning("Warning, it is currently not possible to add the current process/thread to the list of processes "
                       "where the trap flag should be masked. Please ensure that you manually unset the trap flag");
        }

        break;

    case DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER:

        break;

    default:
        break;
    }

    //
    // Continue the current instruction won't pass it
    //
    VmFuncSuppressRipIncrement(KeGetCurrentProcessorNumberEx(NULL));

    //
    // It's not paused anymore!
    //
    ThreadDebuggingDetails->IsPaused = FALSE;
}

/**
 * @brief Perform the user-mode commands
 *
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
UdPerformCommand(PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails,
                 DEBUGGER_UD_COMMAND_ACTION_TYPE    UserAction,
                 UINT64                             OptionalParam1,
                 UINT64                             OptionalParam2,
                 UINT64                             OptionalParam3,
                 UINT64                             OptionalParam4)
{
    UNREFERENCED_PARAMETER(OptionalParam2);
    UNREFERENCED_PARAMETER(OptionalParam3);
    UNREFERENCED_PARAMETER(OptionalParam4);

    //
    // Perform the command
    //
    switch (UserAction)
    {
    case DEBUGGER_UD_COMMAND_ACTION_TYPE_CONTINUE:

        //
        // Continue the thread normally
        //
        UdContinueThread(ThreadDebuggingDetails);

        break;

    case DEBUGGER_UD_COMMAND_ACTION_TYPE_REGULAR_STEP:

        //
        // Stepping through the instructions
        //
        UdStepInstructions(ThreadDebuggingDetails, (DEBUGGER_REMOTE_STEPPING_REQUEST)OptionalParam1);

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
 * @return BOOLEAN
 */
BOOLEAN
UdCheckForCommand()
{
    PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails;

    //
    // Check if user-debugger is initialized or not
    //
    if (!g_UserDebuggerState)
    {
        return FALSE;
    }

    ThreadDebuggingDetails = ThreadHolderGetProcessThreadDetailsByProcessIdAndThreadId(HANDLE_TO_UINT32(PsGetCurrentProcessId()),
                                                                                       HANDLE_TO_UINT32(PsGetCurrentThreadId()));

    if (!ThreadDebuggingDetails)
    {
        return FALSE;
    }

    //
    // If we reached here, the current thread is in debugger attached mechanism
    // now we check whether it's a regular CPUID or a debugger paused thread CPUID
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
            // Perform the command
            //
            UdPerformCommand(ThreadDebuggingDetails,
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
    // Won't change the registers for cpuid
    //
    return TRUE;
}

/**
 * @brief Dispatch the user-mode commands
 *
 * @param ActionRequest
 * @return BOOLEAN
 */
BOOLEAN
UdDispatchUsermodeCommands(PDEBUGGER_UD_COMMAND_PACKET ActionRequest)
{
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails;

    //
    // Find the thread debugging detail of the thread
    //
    ProcessDebuggingDetails = AttachingFindProcessDebuggingDetailsByToken(ActionRequest->ProcessDebuggingDetailToken);

    if (!ProcessDebuggingDetails)
    {
        //
        // Token not found!
        //
        return FALSE;
    }

    //
    // Based on the documentation, HyperDbg stops intercepting threads
    // when the debugger sent the first command, but if user presses
    // CTRL+C again, all the threads (or new threads) that will enter
    // the user-mode will be intercepted
    //
    if (ProcessDebuggingDetails->IsOnThreadInterceptingPhase)
    {
        AttachingConfigureInterceptingThreads(ProcessDebuggingDetails->Token, FALSE);
    }

    //
    // Apply the command to all threads or just one thread
    //
    return ThreadHolderApplyActionToPausedThreads(ProcessDebuggingDetails, ActionRequest);
}

/**
 * @brief Spin on nop sled in user-mode to halt the debuggee
 *
 * @param ThreadDebuggingDetails
 * @param ProcessDebuggingDetails
 * @return VOID
 */
VOID
UdSpinThreadOnNop(PUSERMODE_DEBUGGING_THREAD_DETAILS  ThreadDebuggingDetails,
                  PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails)
{
    //
    // Save the RIP for future return
    //
    ThreadDebuggingDetails->ThreadRip = VmFuncGetRip();

    //
    // Set the rip to new spinning address
    //
    VmFuncSetRip(ProcessDebuggingDetails->UsermodeReservedBuffer);

    //
    // Indicate that it's spinning
    //
    ThreadDebuggingDetails->IsPaused = TRUE;
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
    // Breaking only supported in vmx-root mode, and if user-debugger is
    // loaded
    //
    if (!g_UserDebuggerState && VmFuncVmxGetCurrentExecutionMode() == FALSE)
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
    // Copy registers to the pause packet
    //
    RtlCopyMemory(&PausePacket.GuestRegs, DbgState->Regs, sizeof(GUEST_REGS));

    //
    // Send the pause packet, along with RIP and an indication
    // to pause to the user debugger
    //
    LogCallbackSendBuffer(OPERATION_NOTIFICATION_FROM_USER_DEBUGGER_PAUSE,
                          &PausePacket,
                          sizeof(DEBUGGEE_UD_PAUSED_PACKET),
                          TRUE);

    //
    // Halt the thread on nop sleds
    //
    UdSpinThreadOnNop(ThreadDebuggingDetails, ProcessDebuggingDetails);

    //
    // Everything was okay
    //
    return TRUE;
}
