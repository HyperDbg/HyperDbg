/**
 * @file MetaDispatch.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implementation of meta dispatching functions
 * @details
 *
 * @version 0.7
 * @date 2023-11-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Handling debugger functions related to instrumentation trace events
 *
 * @param DbgState The state of the debugger on the current core
 * @return VOID
 */
VOID
MetaDispatchEventInstrumentationTrace(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE EventTriggerResult;
    BOOLEAN                                   PostEventTriggerReq = FALSE;

    //
    // Triggering the pre-event
    //
    EventTriggerResult = DebuggerTriggerEvents(TRAP_EXECUTION_INSTRUCTION_TRACE,
                                               VMM_CALLBACK_CALLING_STAGE_PRE_EVENT_EMULATION,
                                               (PVOID)DEBUGGER_EVENT_TRACE_TYPE_STEP_IN,
                                               &PostEventTriggerReq,
                                               DbgState->Regs);

    //
    // *** short-circuiting doesn't make sense for this kind of event! ***
    //

    //
    // *** Post-event doesn't make sense for this kind of event! ***
    //
}
