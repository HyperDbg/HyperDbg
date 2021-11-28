/**
 * @file Process.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header for kernel debugger functions for processes
 * @details
 * 
 * @version 0.1
 * @date 2021-11-23
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
ProcessHandleProcessChange(UINT32 ProcessorIndex, PGUEST_REGS GuestState);

BOOLEAN
ProcessInterpretProcess(PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET PidRequest);

BOOLEAN
ProcessCheckIfEprocessIsValid(UINT64 Eprocess, ULONG64 ActiveProcessHead, ULONG ActiveProcessLinksOffset);

VOID
ProcessEnableOrDisableThreadChangeMonitor(UINT32  CurrentProcessorIndex,
                                          BOOLEAN Enable,
                                          BOOLEAN CheckByClockInterrupts);
