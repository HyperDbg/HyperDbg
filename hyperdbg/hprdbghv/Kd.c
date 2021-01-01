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
    HvEnableDbandBpExitingAllCores();

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
    HvDisableDbandBpExitingAllCores();

    //
    // Uinitialize APIC related function
    //
    ApicUninitialize();
}

/**
 * @brief Handle #DBs and #BPs for kernel debugger
 * @details This function can be used in vmx-root 
 * 
 * @return VOID 
 */
VOID
KdHandleBreakpointAndDebugBreakpoints(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs)
{
    //
    // Halt all other Core by interrupting them to nmi
    //
    ApicTriggerGenericNmi();

    //
    // All the cores should go and manage through the following function
    //
    KdManageSystemHaltOnVmxRoot(CurrentProcessorIndex, GuestRegs);
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
    BYTE InstructionBytesOnRip[16 * 2] = {0};

    DbgBreakPoint();

    //
    // Read the buffer at that place
    //
    MemoryMapperReadMemorySafe(g_GuestState[CoreId].LastVmexitRip, InstructionBytesOnRip, 16 * 2);

    //
    // Set vm-exit on hardware debug breakpoint exceptions
    //

    //
    // Set hardware breakpoint on next instruction
    //

    //
    // Send the handshake to show that it Stepped
    //
    SerialConnectionSend("Stepped", 7);
}

/**
 * @brief manage system halt on vmx-root mode 
 * @CurrentCore  
 * 
 * @return VOID 
 */
VOID
KdManageSystemHaltOnVmxRoot(ULONG CurrentCore, PGUEST_REGS GuestRegs)
{
    BYTE InstructionBytesOnRip[MAXIMUM_INSTR_SIZE * 2] = {0};

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
        // Send the handshake to show that it paused
        //
        SerialConnectionSend("Paused", 6);

        //
        // Find the current instruction
        //
        MemoryMapperReadMemorySafe(g_GuestState[CurrentCore].LastVmexitRip, InstructionBytesOnRip, MAXIMUM_INSTR_SIZE * 2);

        //
        // Send it to the debugger
        //
        SerialConnectionSend(InstructionBytesOnRip, MAXIMUM_INSTR_SIZE * 2);

        while (TRUE)
        {
            UCHAR Test = NULL;
            KdHyperDbgRecvByte(&Test);
            if (Test == 'G')
            {
                //
                // Unlock other cores
                //
                SpinlockUnlock(&SystemHaltLock);
                break;
            }
            if (Test == 'S')
            {
                KdStepInstruction(CurrentCore);
            }
        }
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
    // Lock the system halt spinlock
    //
    SpinlockLock(&SystemHaltLock);

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
