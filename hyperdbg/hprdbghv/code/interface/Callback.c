/**
 * @file Callback.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief VMM callback interface routines
 * @details
 *
 * @version 0.2
 * @date 2023-01-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief routines callback to trigger events
 * @param EventType
 * @param CallingStage
 * @param Context
 * @param PostEventRequired
 * @param Regs
 *
 * @return VOID
 */
VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE
VmmCallbackTriggerEvents(VMM_EVENT_TYPE_ENUM                   EventType,
                         VMM_CALLBACK_EVENT_CALLING_STAGE_TYPE CallingStage,
                         PVOID                                 Context,
                         BOOLEAN *                             PostEventRequired,
                         GUEST_REGS *                          Regs)
{
    if (g_Callbacks.VmmCallbackTriggerEvents == NULL)
    {
        return VMM_CALLBACK_TRIGGERING_EVENT_STATUS_SUCCESSFUL_NO_INITIALIZED;
    }

    return g_Callbacks.VmmCallbackTriggerEvents(EventType, CallingStage, Context, PostEventRequired, Regs);
}
