/**
 * @file Ud.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Routines related to user mode debugging
 * @details 
 * @version 0.1
 * @date 2022-01-06
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "..\hprdbghv\pch.h"

/**
 * @brief initialize user debugger
 * @details this function should be called on vmx non-root
 * 
 * @return VOID 
 */
VOID
UdInitializeUserDebugger()
{
    //
    // Check if it's already initialized or not, we'll ignore it if it's
    // previously initialized
    //
    if (g_UserDebuggerState)
    {
        return;
    }

    //
    // Initialize attaching mechanism
    //
    if (!AttachingInitialize())
    {
        return FALSE;
    }

    //
    // Start the seed of user-mode debugging thread
    //
    g_SeedOfUserDebuggingDetails = DebuggerThreadDebuggingTagStartSeed;

    //
    // Initialize the thread debugging details list
    //
    InitializeListHead(&g_ThreadDebuggingDetailsListHead);

    //
    // Enable vm-exit on Hardware debug exceptions and breakpoints
    // so, intercept #DBs and #BP by changing exception bitmap (one core)
    //
    BroadcastEnableDbAndBpExitingAllCores();

    //
    // Indicate that the user debugger is active
    //
    g_UserDebuggerState = TRUE;
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
        AttachingRemoveAndFreeAllThreadDebuggingDetails();
    }
}

VOID
UdSpinThreadOnNop(UINT64 ThreadDebuggingToken)
{
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root 
 * 
 * @param CurrentCore
 * @param ThreadDebuggingToken
 * @param GuestRegs
 * @param Reason
 * @param EventDetails
 * @return VOID 
 */
VOID
UdHandleBreakpointAndDebugBreakpoints(UINT32                            CurrentCore,
                                      UINT64                            ThreadDebuggingToken,
                                      PGUEST_REGS                       GuestRegs,
                                      DEBUGGEE_PAUSING_REASON           Reason,
                                      PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails)
{
    DEBUGGEE_UD_PAUSED_PACKET PausePacket;
    ULONG                     ExitInstructionLength  = 0;
    UINT64                    SizeOfSafeBufferToRead = 0;
    RFLAGS                    Rflags                 = {0};

    //
    // *** Fill the pausing structure ***
    //

    RtlZeroMemory(&PausePacket, sizeof(DEBUGGEE_UD_PAUSED_PACKET));

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
        //
        // Reading instruction length proved to provide wrong results,
        // so we won't use it anymore
        //
        // __vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &ExitInstructionLength);
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
    UdSpinThreadOnNop(ThreadDebuggingToken);
}
