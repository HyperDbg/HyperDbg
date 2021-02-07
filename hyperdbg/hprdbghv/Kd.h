/**
 * @file Kd.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header for routines related to kernel debugging
 * @details 
 * @version 0.1
 * @date 2020-12-20
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				   Functions 	    			//
//////////////////////////////////////////////////

VOID
KdHaltSystem(PDEBUGGER_PAUSE_PACKET_RECEIVED PausePacket);

VOID
KdManageSystemHaltOnVmxRoot(ULONG CurrentCore, PGUEST_REGS GuestRegs, BOOLEAN MainCore);

VOID
KdHandleNmi(UINT32 CurrentProcessorIndex, PGUEST_REGS GuestRegs);

VOID
KdInitializeKernelDebugger();

VOID
KdUninitializeKernelDebugger();

VOID
KdSendFormatsFunctionResult(UINT64 Value);

VOID
KdHandleBreakpointAndDebugBreakpoints(UINT32                  CurrentProcessorIndex,
                                      PGUEST_REGS             GuestRegs,
                                      DEBUGGEE_PAUSING_REASON Reason,
                                      PVOID                   Context);

VOID
KdChangeCr3AndTriggerBreakpointHandler(UINT32                  CurrentProcessorIndex,
                                       PGUEST_REGS             GuestRegs,
                                       DEBUGGEE_PAUSING_REASON Reason,
                                       CR3_TYPE                TargetCr3);

BOOLEAN
KdResponsePacketToDebugger(
    DEBUGGER_REMOTE_PACKET_TYPE             PacketType,
    DEBUGGER_REMOTE_PACKET_REQUESTED_ACTION Response,
    CHAR *                                  OptionalBuffer,
    UINT32                                  OptionalBufferLength);
