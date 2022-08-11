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
    __vmx_vmwrite(VMCS_GUEST_RIP, ThreadDebuggingDetails->ThreadRip);
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
    g_GuestState[KeGetCurrentProcessorNumber()].IncrementRip = FALSE;

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
    RFLAGS Rflags = {0};

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
        __vmx_vmread(VMCS_GUEST_RFLAGS, &Rflags);

        Rflags.TrapFlag = TRUE;

        __vmx_vmwrite(VMCS_GUEST_RFLAGS, Rflags.AsUInt);

        //
        // Rflags' trap flag is set
        //
        ThreadDebuggingDetails->IsRflagsTrapFlagsSet = TRUE;

        break;

    case DEBUGGER_REMOTE_STEPPING_REQUEST_STEP_OVER:

        break;

    default:
        break;
    }

    //
    // Continue the current instruction won't pass it
    //
    g_GuestState[KeGetCurrentProcessorNumber()].IncrementRip = FALSE;

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

    ThreadDebuggingDetails =
        ThreadHolderGetProcessThreadDetailsByProcessIdAndThreadId(PsGetCurrentProcessId(),
                                                                  PsGetCurrentThreadId());

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
            ThreadDebuggingDetails->UdAction[i].OptionalParam1 = NULL;
            ThreadDebuggingDetails->UdAction[i].OptionalParam2 = NULL;
            ThreadDebuggingDetails->UdAction[i].OptionalParam3 = NULL;
            ThreadDebuggingDetails->UdAction[i].OptionalParam4 = NULL;

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
    __vmx_vmread(VMCS_GUEST_RIP, &ThreadDebuggingDetails->ThreadRip);

    //
    // Set the rip to new spinning address
    //
    __vmx_vmwrite(VMCS_GUEST_RIP, ProcessDebuggingDetails->UsermodeReservedBuffer);

    //
    // Indicate that it's spinning
    //
    ThreadDebuggingDetails->IsPaused = TRUE;
}

/**
 * @brief Handle after we hit the stepping
 * @details This function can be used in vmx-root
 *
 * @param CurrentCore
 * @param ThreadDebuggingDetails
 * @return VOID
 */
VOID
UdHandleAfterSteppingReason(UINT32                             CurrentCore,
                            PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails)
{
    RFLAGS Rflags = {0};

    //
    // Unset the trap-flag
    //
    __vmx_vmread(VMCS_GUEST_RFLAGS, &Rflags);

    Rflags.TrapFlag = FALSE;

    __vmx_vmwrite(VMCS_GUEST_RFLAGS, Rflags.AsUInt);

    //
    // Rflags' trap flag is not set anymore
    //
    ThreadDebuggingDetails->IsRflagsTrapFlagsSet = FALSE;
}

/**
 * @brief Handle special reasons pre-pausings
 * @details This function can be used in vmx-root
 *
 * @param CurrentCore
 * @param ThreadDebuggingDetails
 * @param GuestRegs
 * @param Reason
 * @param EventDetails
 * @return VOID
 */
VOID
UdPrePausingReasons(UINT32                             CurrentCore,
                    PUSERMODE_DEBUGGING_THREAD_DETAILS ThreadDebuggingDetails,
                    PGUEST_REGS                        GuestRegs,
                    DEBUGGEE_PAUSING_REASON            Reason,
                    PDEBUGGER_TRIGGERED_EVENT_DETAILS  EventDetails)

{
    //
    // *** Handle events before pausing ***
    //
    switch (Reason)
    {
    case DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_DEBUG_BREAK:

        if (ThreadDebuggingDetails->IsRflagsTrapFlagsSet)
        {
            UdHandleAfterSteppingReason(CurrentCore, ThreadDebuggingDetails);
        }

        break;

    default:
        break;
    }
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root
 *
 * @param CurrentCore
 * @param GuestRegs
 * @param Reason
 * @param EventDetails
 * @return BOOLEAN
 */
BOOLEAN
UdCheckAndHandleBreakpointsAndDebugBreaks(UINT32                            CurrentCore,
                                          PGUEST_REGS                       GuestRegs,
                                          DEBUGGEE_PAUSING_REASON           Reason,
                                          PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails)
{
    DEBUGGEE_UD_PAUSED_PACKET           PausePacket;
    ULONG                               ExitInstructionLength   = 0;
    UINT64                              SizeOfSafeBufferToRead  = 0;
    RFLAGS                              Rflags                  = {0};
    PUSERMODE_DEBUGGING_PROCESS_DETAILS ProcessDebuggingDetails = NULL;
    PUSERMODE_DEBUGGING_THREAD_DETAILS  ThreadDebuggingDetails  = NULL;

    //
    // Breaking only supported in vmx-root mode, and if user-debugger is
    // loaded
    //
    if (!g_UserDebuggerState && !g_GuestState[CurrentCore].IsOnVmxRootMode)
    {
        return FALSE;
    }

    //
    // Check entry of paused thread
    //
    ProcessDebuggingDetails = AttachingFindProcessDebuggingDetailsByProcessId(PsGetCurrentProcessId());

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
    ThreadDebuggingDetails = ThreadHolderFindOrCreateThreadDebuggingDetail(PsGetCurrentThreadId(), ProcessDebuggingDetails);

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
    UdPrePausingReasons(CurrentCore, ThreadDebuggingDetails, GuestRegs, Reason, EventDetails);

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
    PausePacket.ProcessId             = PsGetCurrentProcessId();
    PausePacket.ThreadId              = PsGetCurrentThreadId();
    PausePacket.ProcessDebuggingToken = ProcessDebuggingDetails->Token;

    //
    // Set the RIP and mode of execution
    //
    PausePacket.Rip     = g_GuestState[CurrentCore].LastVmexitRip;
    PausePacket.Is32Bit = KdIsGuestOnUsermode32Bit();

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
    if (g_GuestState[CurrentCore].DebuggingState.InstructionLengthHint != 0)
    {
        ExitInstructionLength = g_GuestState[CurrentCore].DebuggingState.InstructionLengthHint;
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
        SizeOfSafeBufferToRead = g_GuestState[CurrentCore].LastVmexitRip & 0xfff;
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
    MemoryMapperReadMemorySafeOnTargetProcess(g_GuestState[CurrentCore].LastVmexitRip,
                                              &PausePacket.InstructionBytesOnRip,
                                              ExitInstructionLength);

    //
    // Copy registers to the pause packet
    //
    RtlCopyMemory(&PausePacket.GuestRegs, GuestRegs, sizeof(GUEST_REGS));

    //
    // Send the pause packet, along with RIP and an indication
    // to pause to the user debugger
    //
    LogSendBuffer(OPERATION_NOTIFICATION_FROM_USER_DEBUGGER_PAUSE,
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
