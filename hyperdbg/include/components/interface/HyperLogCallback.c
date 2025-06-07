/**
 * @file HyperLogCallback.c
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