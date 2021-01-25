/**
 * @file Kd.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
    // Broadcast on all core to cause exit for NMIs
    //
    HvEnableNmiExitingAllCores();

    //
    // Enable vm-exit on Hardware debug exceptions and breakpoints
    // so, intercept #DBs and #BP by changing exception bitmap (one core)
    //
    HvEnableDbAndBpExitingAllCores();

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
    //
    // Indicate that kernel debugger is not active
    //
    g_KernelDebuggerState = FALSE;

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
    // Uinitialize APIC related function
    //
    ApicUninitialize();
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
 * @brief Sends a HyperDbg response packet to the debugger
 *
 * @param PacketType
 * @param Response
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
        SerialConnectionSend((CHAR *)&Packet, sizeof(DEBUGGER_REMOTE_PACKET));
    }
    else
    {
        SerialConnectionSendTwoBuffers((CHAR *)&Packet, sizeof(DEBUGGER_REMOTE_PACKET), OptionalBuffer, OptionalBufferLength);
    }

    return TRUE;
}

/**
 * @brief Receive packet from the debugger
 *
 * @param BufferToSave
 * @param LengthReceived
 *
 * @return VOID
 */
VOID
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

        BufferToSave[Loop] = RecvChar;

        if (KdCheckForTheEndOfTheBuffer(&Loop, (BYTE *)BufferToSave))
        {
            break;
        }

        ++Loop;
    }

    //
    // Set the length
    //
    *LengthReceived = Loop;

    return TRUE;
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root 
 * 
 * @return VOID 
 */
VOID
KdHandleBreakpointAndDebugBreakpoints(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs, DEBUGGEE_PAUSING_REASON Reason)
{
    //
    // Lock the system halt spinlock
    //
    SpinlockLock(&SystemHaltLock);

    //
    // Set the halting reason
    //
    g_DebuggeeHaltReason = Reason;

    //
    // Halt all other Core by interrupting them to nmi
    //
    ApicTriggerGenericNmi();

    //
    // All the cores should go and manage through the following function
    //
    KdManageSystemHaltOnVmxRoot(CurrentProcessorIndex, GuestRegs);

    //
    // Clear the halting reason
    //
    g_DebuggeeHaltReason = DEBUGGEE_PAUSING_REASON_NOT_PAUSED;
}

/**
 * @brief Handle NMI Vm-exits
 * @details This function should be called in vmx-root mode
 * @return VOID 
 */
VOID
KdHandleNmi(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs)
{
    LogInfo("NMI Arrived on : %d \n", KeGetCurrentProcessorNumber());

    //
    // All the cores should go and manage through the following function
    //
    KdManageSystemHaltOnVmxRoot(CurrentProcessorIndex, GuestRegs);
}

/**
 * @brief apply step one instruction to the debuggee
 * @param CoreId 
 * @return VOID 
 */
VOID
KdStepInstruction(ULONG CoreId)
{
    //
    // It is because maximum instruction length in x64 is 16 bytes
    //
    g_GuestState[CoreId].DebuggingState.WaitForStepOnMtf = TRUE;

    //
    // Not unset again
    //
    g_GuestState[CoreId].IgnoreMtfUnset = TRUE;

    //
    // Set the MTF flag
    //
    HvSetMonitorTrapFlag(TRUE);
}

/**
 * @brief This function applies commands from the debugger to the debuggee
 * @details when we reach here, we are on the first core
 * @param GuestRegs  
 * 
 * @return VOID 
 */
VOID
KdDispatchAndPerformCommandsFromDebugger(PGUEST_REGS GuestRegs)
{
    while (TRUE)
    {
        BOOLEAN                 EscapeFromTheLoop = FALSE;
        CHAR *                  RecvBuffer[64]    = {0};
        UINT32                  RecvBufferLength  = 0;
        PDEBUGGER_REMOTE_PACKET TheActualPacket =
            (PDEBUGGER_REMOTE_PACKET)RecvBuffer;

        //
        // Receive the buffer in polling mode
        //
        KdRecvBuffer(&RecvBuffer, &RecvBufferLength);

        if (TheActualPacket->Indicator == INDICATOR_OF_HYPERDBG_PACKER)
        {
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
                LogError("err, unknown packet received from the debugger.\n");
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
                SpinlockUnlock(&SystemHaltLock);

                //
                // No need to wait for new commands
                //
                EscapeFromTheLoop = TRUE;

                break;

            case DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION_ON_VMX_ROOT_MODE_STEP:

                //
                // Indicate a step
                //
                KdStepInstruction(0);

                //
                // Unlock other cores
                //
                SpinlockUnlock(&SystemHaltLock);

                //
                // No need to wait for new commands
                //
                EscapeFromTheLoop = TRUE;

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
 * @brief manage system halt on vmx-root mode 
 * @details Thuis function should only be called from KdHandleBreakpointAndDebugBreakpoints
 * @param CurrentCore  
 * @param GuestRegs  
 * 
 * @return VOID 
 */
VOID
KdManageSystemHaltOnVmxRoot(ULONG CurrentCore, PGUEST_REGS GuestRegs)
{
    DEBUGGEE_PAUSED_PACKET PausePacket = {0};

    //
    // We check for receiving buffer (unhalting) only on the
    // first core and not on every cores
    //
    if (CurrentCore == 0)
    {
        //
        // *** First Core ***
        //

        //
        // Set the halt reason
        //
        PausePacket.PausingReason = g_DebuggeeHaltReason;

        //
        // Set the RIP
        //
        PausePacket.Rip = g_GuestState[CurrentCore].LastVmexitRip;

        //
        // Find the current instruction
        //
        MemoryMapperReadMemorySafe(g_GuestState[CurrentCore].LastVmexitRip,
                                   &PausePacket.InstructionBytesOnRip,
                                   MAXIMUM_INSTR_SIZE);

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
        KdDispatchAndPerformCommandsFromDebugger(GuestRegs);
    }
    else
    {
        //
        // All cores except first core
        //

        //
        // Lock and unlock the lock so all core can get the lock
        // and continue their normal execution
        //
        SpinlockLock(&SystemHaltLock);
        SpinlockUnlock(&SystemHaltLock);
    }
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
