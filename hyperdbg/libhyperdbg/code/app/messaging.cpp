/**
 * @file messaging.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Functions for handling messages
 * @details
 * @version 0.19
 * @date 2026-05-28
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

//
// Global Variables
//
extern PVOID   g_MessageHandler;
extern PVOID   g_MessageHandlerSharedBuffer;
extern BOOLEAN g_LogOpened;
extern BOOLEAN g_IsConnectedToRemoteDebugger;
extern BOOLEAN g_IsSerialConnectedToRemoteDebugger;

/**
 * @brief Set the function callback that will be called if any message
 * needs to be shown
 *
 * @param Handler Function that handles the messages
 * @return VOID
 */
VOID
SetTextMessageCallback(PVOID Handler)
{
    g_MessageHandler = Handler;
}

/**
 * @brief Set the function callback that will be called if any message
 * needs to be shown
 *
 * @param Handler Function that handles the messages
 * @return PVOID
 */
PVOID
SetTextMessageCallbackUsingSharedBuffer(PVOID Handler)
{
    g_MessageHandler             = Handler;
    g_MessageHandlerSharedBuffer = malloc(COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT);

    if (!g_MessageHandlerSharedBuffer)
    {
        g_MessageHandler = NULL;
        return NULL;
    }

    RtlZeroMemory(g_MessageHandlerSharedBuffer, COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT);

    return g_MessageHandlerSharedBuffer;
}

/**
 * @brief Unset the function callback that will be called if any message
 * needs to be shown
 *
 * @return VOID
 */
VOID
UnsetTextMessageCallback()
{
    g_MessageHandler = NULL;
    free(g_MessageHandlerSharedBuffer);
    g_MessageHandlerSharedBuffer = NULL;
}

/**
 * @brief Show messages
 *
 * @param Fmt format string message
 * @param ... arguments
 * @return VOID
 */
VOID
ShowMessages(const char * Fmt, ...)
{
    va_list ArgList;
    va_list Args;
    char    TempMessage[COMMUNICATION_BUFFER_SIZE + TCP_END_OF_BUFFER_CHARS_COUNT] = {0};

    if (g_MessageHandler == NULL && !g_IsConnectedToRemoteDebugger && !g_IsSerialConnectedToRemoteDebugger)
    {
        va_start(Args, Fmt);

        vprintf(Fmt, Args);

        va_end(Args);

        if (!g_LogOpened)
        {
            return;
        }
    }

    va_start(ArgList, Fmt);

    int SprintfResult = vsprintf_s(TempMessage, Fmt, ArgList);

    va_end(ArgList);

    if (SprintfResult != -1)
    {
        if (g_IsConnectedToRemoteDebugger)
        {
            //
            // vsprintf_s and vswprintf_s return the number of characters written,
            // not including the terminating null character, or a negative value
            // if an output error occurs.
            //
            RemoteConnectionSendResultsToHost(TempMessage, SprintfResult);
        }
        else if (g_IsSerialConnectedToRemoteDebugger)
        {
            KdSendUsermodePrints(TempMessage, SprintfResult);
        }

        if (g_LogOpened)
        {
            //
            // .logopen command executed
            //
            LogopenSaveToFile(TempMessage);
        }
        if (g_MessageHandler != NULL)
        {
            //
            // There is another handler
            //
            if (g_MessageHandlerSharedBuffer == NULL)
            {
                ((SendMessageWithParamCallback)g_MessageHandler)(TempMessage);
            }
            else
            {
                memcpy(g_MessageHandlerSharedBuffer, TempMessage, strlen(TempMessage) + 1);
                ((SendMessageWithSharedBufferCallback)g_MessageHandler)();
            }
        }
    }
}
