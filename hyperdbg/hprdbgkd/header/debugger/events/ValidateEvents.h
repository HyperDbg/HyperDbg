/**
 * @file ValidateEvents.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers of debugger functions for validating events
 * @details
 *
 * @version 0.7
 * @date 2023-10-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

BOOLEAN
ValidateEventMonitor(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                     PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                     BOOLEAN                           InputFromVmxRoot);

BOOLEAN
ValidateEventException(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                       PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                       BOOLEAN                           InputFromVmxRoot);

BOOLEAN
ValidateEventInterrupt(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                       PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                       BOOLEAN                           InputFromVmxRoot);

BOOLEAN
ValidateEventTrapExec(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                      PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                      BOOLEAN                           InputFromVmxRoot);

BOOLEAN
ValidateEventEptHookHiddenBreakpointAndInlineHooks(PDEBUGGER_GENERAL_EVENT_DETAIL    EventDetails,
                                                   PDEBUGGER_EVENT_AND_ACTION_RESULT ResultsToReturn,
                                                   BOOLEAN                           InputFromVmxRoot);
