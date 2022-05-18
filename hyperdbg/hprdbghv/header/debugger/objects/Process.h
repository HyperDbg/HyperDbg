/**
 * @file Process.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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

VOID
ProcessEnableOrDisableThreadChangeMonitor(UINT32  CurrentProcessorIndex,
                                          BOOLEAN Enable,
                                          BOOLEAN CheckByClockInterrupts);

BOOLEAN
ProcessHandleProcessChange(UINT32 ProcessorIndex, PGUEST_REGS GuestState);

BOOLEAN
ProcessInterpretProcess(PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET PidRequest);

BOOLEAN
ProcessCheckIfEprocessIsValid(UINT64 Eprocess, UINT64 ActiveProcessHead, ULONG ActiveProcessLinksOffset);

BOOLEAN
ProcessQueryCount(PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS DebuggerUsermodeProcessOrThreadQueryRequest);

BOOLEAN
ProcessQueryList(PDEBUGGER_QUERY_ACTIVE_PROCESSES_OR_THREADS DebuggerUsermodeProcessOrThreadQueryRequest,
                 PVOID                                       AddressToSaveDetail,
                 UINT32                                      BufferSize);

BOOLEAN
ProcessQueryDetails(PDEBUGGEE_DETAILS_AND_SWITCH_PROCESS_PACKET GetInformationProcessRequest);
