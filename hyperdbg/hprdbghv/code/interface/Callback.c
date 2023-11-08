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
 * @brief routines callback for preparing and sending message to queue
 *
 * @param OperationCode
 * @param IsImmediateMessage
 * @param ShowCurrentSystemTime
 * @param Priority
 * @param Fmt
 * @param ...
 *
 * @return BOOLEAN
 */
BOOLEAN
LogCallbackPrepareAndSendMessageToQueue(UINT32       OperationCode,
                                        BOOLEAN      IsImmediateMessage,
                                        BOOLEAN      ShowCurrentSystemTime,
                                        BOOLEAN      Priority,
                                        const char * Fmt,
                                        ...)
{
    BOOLEAN Result;
    va_list ArgList;

    if (g_Callbacks.LogCallbackPrepareAndSendMessageToQueueWrapper == NULL)
    {
        //
        // Ignore sending message to queue
        //
        return FALSE;
    }

    va_start(ArgList, Fmt);

    Result = g_Callbacks.LogCallbackPrepareAndSendMessageToQueueWrapper(OperationCode,
                                                                        IsImmediateMessage,
                                                                        ShowCurrentSystemTime,
                                                                        Priority,
                                                                        Fmt,
                                                                        ArgList);
    va_end(ArgList);

    return Result;
}

/**
 * @brief routines callback for sending message to queue
 *
 * @param OperationCode
 * @param IsImmediateMessage
 * @param LogMessage
 * @param BufferLen
 * @param Priority
 *
 * @return BOOLEAN
 */
BOOLEAN
LogCallbackSendMessageToQueue(UINT32  OperationCode,
                              BOOLEAN IsImmediateMessage,
                              CHAR *  LogMessage,
                              UINT32  BufferLen,
                              BOOLEAN Priority)
{
    if (g_Callbacks.LogCallbackSendMessageToQueue == NULL)
    {
        //
        // Ignore sending message to queue
        //
        return FALSE;
    }

    return g_Callbacks.LogCallbackSendMessageToQueue(OperationCode,
                                                     IsImmediateMessage,
                                                     LogMessage,
                                                     BufferLen,
                                                     Priority);
}

/**
 * @brief routines callback for checking if buffer is full
 *
 * @param Priority
 *
 * @return BOOLEAN
 */
BOOLEAN
LogCallbackCheckIfBufferIsFull(BOOLEAN Priority)
{
    if (g_Callbacks.LogCallbackCheckIfBufferIsFull == NULL)
    {
        //
        // Ignore sending message to queue
        //
        return FALSE;
    }

    return g_Callbacks.LogCallbackCheckIfBufferIsFull(Priority);
}

/**
 * @brief routines callback for sending buffer
 * @param OperationCode
 * @param Buffer
 * @param BufferLength
 * @param Priority
 *
 * @return BOOLEAN
 */
BOOLEAN
LogCallbackSendBuffer(_In_ UINT32                          OperationCode,
                      _In_reads_bytes_(BufferLength) PVOID Buffer,
                      _In_ UINT32                          BufferLength,
                      _In_ BOOLEAN                         Priority)

{
    if (g_Callbacks.LogCallbackSendBuffer == NULL)
    {
        //
        // Ignore sending buffer
        //
        return FALSE;
    }

    return g_Callbacks.LogCallbackSendBuffer(OperationCode,
                                             Buffer,
                                             BufferLength,
                                             Priority);
}

/**
 * @brief routines callback to trigger events
 * @param EventType
 * @param CallingStage
 * @param Context
 * @param PostEventRequired
 * @param Regs
 *
 * @return VMM_CALLBACK_TRIGGERING_EVENT_STATUS_TYPE
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

/**
 * @brief routine callback to set last error
 * @param LastError
 *
 * @return VOID
 */
VOID
VmmCallbackSetLastError(UINT32 LastError)
{
    if (g_Callbacks.VmmCallbackSetLastError == NULL)
    {
        //
        // Ignore setting the last error
        //
        return;
    }

    g_Callbacks.VmmCallbackSetLastError(LastError);
}

/**
 * @brief routine callback to handle external VMCALLs
 *
 * @param CoreId
 * @param VmcallNumber
 * @param OptionalParam1
 * @param OptionalParam2
 * @param OptionalParam3
 *
 * @return BOOLEAN
 */
BOOLEAN
VmmCallbackVmcallHandler(UINT32 CoreId,
                         UINT64 VmcallNumber,
                         UINT64 OptionalParam1,
                         UINT64 OptionalParam2,
                         UINT64 OptionalParam3)
{
    if (g_Callbacks.VmmCallbackVmcallHandler == NULL)
    {
        //
        // Ignore handling external VMCALLs
        //
        return FALSE;
    }

    return g_Callbacks.VmmCallbackVmcallHandler(CoreId, VmcallNumber, OptionalParam1, OptionalParam2, OptionalParam3);
}

/**
 * @brief routine callback to handle registered MTF
 *
 * @param CoreId
 *
 * @return VOID
 */
VOID
VmmCallbackRegisteredMtfHandler(UINT32 CoreId)
{
    if (g_Callbacks.VmmCallbackRegisteredMtfHandler == NULL)
    {
        //
        // ignore it
        //
        return;
    }

    g_Callbacks.VmmCallbackRegisteredMtfHandler(CoreId);
}

/**
 * @brief routine callback to handle NMI requests
 *
 * @param CoreId
 * @param IsOnVmxNmiHandler
 *
 * @return VOID
 */
VOID
VmmCallbackNmiBroadcastRequestHandler(UINT32 CoreId, BOOLEAN IsOnVmxNmiHandler)
{
    if (g_Callbacks.VmmCallbackNmiBroadcastRequestHandler == NULL)
    {
        //
        // ignore it
        //
        return;
    }

    g_Callbacks.VmmCallbackNmiBroadcastRequestHandler(CoreId, IsOnVmxNmiHandler);
}

/**
 * @brief routine callback to query for termination of protected resources
 *
 * @param CoreId
 * @param ResourceType
 * @param Context
 * @param PassOver
 *
 * @return BOOLEAN
 */
BOOLEAN
VmmCallbackQueryTerminateProtectedResource(UINT32                               CoreId,
                                           PROTECTED_HV_RESOURCES_TYPE          ResourceType,
                                           PVOID                                Context,
                                           PROTECTED_HV_RESOURCES_PASSING_OVERS PassOver)
{
    if (g_Callbacks.VmmCallbackQueryTerminateProtectedResource == NULL)
    {
        //
        // ignore it
        //
        return FALSE;
    }

    return g_Callbacks.VmmCallbackQueryTerminateProtectedResource(CoreId, ResourceType, Context, PassOver);
}

/**
 * @brief routine callback to restore EPT state
 * @param CoreId
 *
 * @return BOOLEAN
 */
BOOLEAN
VmmCallbackRestoreEptState(UINT32 CoreId)
{
    if (g_Callbacks.VmmCallbackRestoreEptState == NULL)
    {
        //
        // ignore it as it's not handled
        //
        return FALSE;
    }

    return g_Callbacks.VmmCallbackRestoreEptState(CoreId);
}

/**
 * @brief routine callback to handle unhandled EPT violations
 * @param CoreId
 * @param ViolationQualification
 * @param GuestPhysicalAddr
 *
 * @return BOOLEAN
 */
BOOLEAN
VmmCallbackUnhandledEptViolation(UINT32 CoreId,
                                 UINT64 ViolationQualification,
                                 UINT64 GuestPhysicalAddr)
{
    if (g_Callbacks.VmmCallbackCheckUnhandledEptViolations == NULL)
    {
        //
        // ignore it as it's not handled
        //
        return FALSE;
    }

    return g_Callbacks.VmmCallbackCheckUnhandledEptViolations(CoreId, ViolationQualification, GuestPhysicalAddr);
}

/**
 * @brief routine callback to handle breakpoint exception
 *
 * @param CoreId
 *
 * @return BOOLEAN
 */
BOOLEAN
DebuggingCallbackHandleBreakpointException(UINT32 CoreId)
{
    if (g_Callbacks.DebuggingCallbackHandleBreakpointException == NULL)
    {
        //
        // re-inject it to not disrupt system normal execution
        //
        return FALSE;
    }

    return g_Callbacks.DebuggingCallbackHandleBreakpointException(CoreId);
}

/**
 * @brief routine callback to handle debug breakpoint exception
 *
 * @param CoreId
 *
 * @return BOOLEAN
 */
BOOLEAN
DebuggingCallbackHandleDebugBreakpointException(UINT32 CoreId)
{
    if (g_Callbacks.DebuggingCallbackHandleDebugBreakpointException == NULL)
    {
        //
        // re-inject it to not disrupt system normal execution
        //
        return FALSE;
    }

    return g_Callbacks.DebuggingCallbackHandleDebugBreakpointException(CoreId);
}

/**
 * @brief routine callback to handle conditional page-fault exception
 *
 * @param CoreId
 * @param Address
 * @param PageFaultErrorCode
 *
 * @return BOOLEAN
 */
BOOLEAN
DebuggingCallbackConditionalPageFaultException(UINT32 CoreId,
                                               UINT64 Address,
                                               UINT32 PageFaultErrorCode)
{
    if (g_Callbacks.DebuggingCallbackConditionalPageFaultException == NULL)
    {
        //
        // re-inject it to not disrupt system normal execution
        //
        return FALSE;
    }

    return g_Callbacks.DebuggingCallbackConditionalPageFaultException(CoreId, Address, PageFaultErrorCode);
}

/**
 * @brief routine callback to handle cr3 process change
 *
 * @param CoreId
 *
 * @return VOID
 */
VOID
InterceptionCallbackTriggerCr3ProcessChange(UINT32 CoreId)
{
    if (g_Callbacks.InterceptionCallbackTriggerCr3ProcessChange == NULL)
    {
        //
        // ignore it
        //
        return;
    }

    g_Callbacks.InterceptionCallbackTriggerCr3ProcessChange(CoreId);
}

/**
 * @brief routine callback to handle cr3 process change
 *
 * @param CoreId
 * @param NewCr3
 *
 * @return VOID
 */
VOID
InterceptionCallbackCr3VmexitsForThreadInterception(UINT32 CoreId, CR3_TYPE NewCr3)
{
    if (g_Callbacks.AttachingHandleCr3VmexitsForThreadInterception == NULL)
    {
        //
        // ignore it
        //
        return;
    }

    g_Callbacks.AttachingHandleCr3VmexitsForThreadInterception(CoreId, NewCr3);
}
