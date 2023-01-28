/**
 * @file BreakpointCommands.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Commands for setting breakpoints
 *
 * @version 0.1
 * @date 2021-03-12
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

VOID
BreakpointRemoveAllBreakpoints();

BOOLEAN
BreakpointAddNew(PDEBUGGEE_BP_PACKET BpDescriptorArg);

BOOLEAN
BreakpointListOrModify(PDEBUGGEE_BP_LIST_OR_MODIFY_PACKET ListOrModifyBreakpoints);

BOOLEAN
BreakpointHandleBpTraps(UINT32 CoreId);

BOOLEAN
BreakpointCheckAndHandleDebuggerDefinedBreakpoints(PROCESSOR_DEBUGGING_STATE * DbgState,
                                                   UINT64                      GuestRip,
                                                   DEBUGGEE_PAUSING_REASON     Reason,
                                                   BOOLEAN                     ChangeMtfState);

BOOLEAN
BreakpointCheckAndHandleReApplyingBreakpoint(UINT32 CoreId);

BOOLEAN
BreakpointCheckAndHandleDebugBreakpoint(UINT32 CoreId);

BOOLEAN
SearchAddressWrapper(PUINT64                 AddressToSaveResults,
                     PDEBUGGER_SEARCH_MEMORY SearchMemRequest,
                     UINT64                  StartAddress,
                     UINT64                  EndAddress,
                     BOOLEAN                 IsDebuggeePaused,
                     PUINT32                 CountOfMatchedCases);
