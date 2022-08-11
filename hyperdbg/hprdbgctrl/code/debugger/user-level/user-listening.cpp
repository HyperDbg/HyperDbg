/**
 * @file user-listening.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Listening for user debugger thread events
 * @details
 * @version 0.1
 * @date 2022-01-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;
extern DEBUGGER_SYNCRONIZATION_EVENTS_STATE
    g_UserSyncronizationObjectsHandleTable[DEBUGGER_MAXIMUM_SYNCRONIZATION_USER_DEBUGGER_OBJECTS];

/**
 * @brief Handle pause packets from user debugger
 *
 * @param PausePacket
 *
 * @return VOID
 */
VOID
UdHandleUserDebuggerPausing(PDEBUGGEE_UD_PAUSED_PACKET PausePacket)
{
    //
    // Set the current active debugging process (thread)
    //
    UdSetActiveDebuggingProcess(PausePacket->ProcessDebuggingToken,
                                PausePacket->ProcessId,
                                PausePacket->ThreadId,
                                PausePacket->Is32Bit,
                                TRUE);

    //
    // Perform extra tasks for pausing reasons
    //
    switch (PausePacket->PausingReason)
    {
    case DEBUGGEE_PAUSING_REASON_DEBUGGEE_ENTRY_POINT_REACHED:

        ShowMessages("reached to the entrypoint of the main module\n");

        break;
    case DEBUGGEE_PAUSING_REASON_DEBUGGEE_GENERAL_THREAD_INTERCEPTED:

        ShowMessages("\nthread: %x from process: %x intercepted\n",
                     PausePacket->ThreadId,
                     PausePacket->ProcessId);

        break;

    default:
        break;
    }

    //
    // Check if the instruction is received completely or not
    //
    if (PausePacket->ReadInstructionLen != MAXIMUM_INSTR_SIZE)
    {
        //
        // We check if the disassembled buffer has greater size
        // than what is retrieved
        //
        if (HyperDbgLengthDisassemblerEngine(PausePacket->InstructionBytesOnRip,
                                             MAXIMUM_INSTR_SIZE,
                                             PausePacket->Is32Bit ? FALSE : TRUE) > PausePacket->ReadInstructionLen)
        {
            ShowMessages("oOh, no! there might be a misinterpretation in disassembling the current instruction\n");
        }
    }

    if (!PausePacket->Is32Bit)
    {
        //
        // Show diassembles
        //
        HyperDbgDisassembler64(PausePacket->InstructionBytesOnRip,
                               PausePacket->Rip,
                               MAXIMUM_INSTR_SIZE,
                               1,
                               TRUE,
                               (PRFLAGS)&PausePacket->Rflags);
    }
    else
    {
        //
        // Show diassembles
        //
        HyperDbgDisassembler32(PausePacket->InstructionBytesOnRip,
                               PausePacket->Rip,
                               MAXIMUM_INSTR_SIZE,
                               1,
                               TRUE,
                               (PRFLAGS)&PausePacket->Rflags);
    }

    //
    // Unpause the user debugger to get commands
    //
    if (g_UserSyncronizationObjectsHandleTable
            [DEBUGGER_SYNCRONIZATION_OBJECT_USER_DEBUGGER_IS_DEBUGGER_RUNNING]
                .IsOnWaitingState == TRUE)
    {
        DbgReceivedUserResponse(DEBUGGER_SYNCRONIZATION_OBJECT_USER_DEBUGGER_IS_DEBUGGER_RUNNING);
    }
}
