/**
 * @file objects.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for routines related to objects
 * @details
 * @version 0.1
 * @date 2022-05-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////
//				Functions 		     	//
//////////////////////////////////////////

BOOLEAN
ObjectShowProcessesOrThreadDetails(DEBUGGEE_DETAILS_AND_SWITCH_PROCESS_TYPE ActionType,
                                   UINT32                                   NewPid,
                                   UINT64                                   NewProcess,
                                   BOOLEAN                                  SetChangeByClockInterrupt,
                                   PDEBUGGEE_PROCESS_LIST_NEEDED_DETAILS    SymDetailsForProcessList);
