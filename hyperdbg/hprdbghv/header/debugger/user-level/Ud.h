/**
 * @file Ud.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for routines related to user mode debugging
 * @details 
 * @version 0.1
 * @date 2022-01-06
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				      Functions     			//
//////////////////////////////////////////////////

BOOLEAN
UdInitializeUserDebugger();

VOID
UdUninitializeUserDebugger();

BOOLEAN
UdCheckAndHandleBreakpointsAndDebugBreaks(UINT32                            CurrentCore,
                                          PGUEST_REGS                       GuestRegs,
                                          DEBUGGEE_PAUSING_REASON           Reason,
                                          PDEBUGGER_TRIGGERED_EVENT_DETAILS EventDetails);

BOOLEAN
UdDispatchUsermodeCommands(PDEBUGGER_UD_COMMAND_PACKET ActionRequest);

BOOLEAN
UdCheckForCommand();
