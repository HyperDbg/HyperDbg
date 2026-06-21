/**
 * @file platform-signal.c
 * @author Max Raulea (max.raulea@gmail.com)
 * @brief User mode cross-platform implementation of the console-control handler
 * @details See platform-signal.h. The Windows branch forwards to SetConsoleCtrlHandler.
 *          The Linux branch blocks the handled signals and dispatches them from a
 *          dedicated sigwait() thread. sigwait() (rather than a sigaction() handler)
 *          is used deliberately: BreakController calls ShowMessages (printf), socket
 *          I/O and Sleep, none of which are async-signal-safe. Running the handler on
 *          a normal thread woken by sigwait() sidesteps that entirely and mirrors the
 *          Windows model where the console-control handler also runs on its own thread.
 *
 * @version 0.20
 * @date 2026-06-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/platform-signal.h"
#endif // defined(__linux__)

#if defined(_WIN32)

BOOLEAN
PlatformInstallCtrlHandler(PLATFORM_CTRL_HANDLER Handler)
{
    return (BOOLEAN)SetConsoleCtrlHandler((PHANDLER_ROUTINE)Handler, TRUE);
}

#elif defined(__linux__)

#    include <signal.h>
#    include <pthread.h>

//
// The installed handler, read by the dispatch thread.
//
static PLATFORM_CTRL_HANDLER g_PlatformCtrlHandler = NULL;

//
// Map a POSIX signal onto the Win32 console-control event the shared handler
// understands. Only the two interactive break signals are mapped:
//   SIGINT  (CTRL+C)  -> CTRL_C_EVENT
//   SIGQUIT (CTRL+\)  -> CTRL_BREAK_EVENT
// SIGTERM/SIGHUP are intentionally left at their default disposition so the
// process stays killable/terminable the usual way; the CLOSE/LOGOFF/SHUTDOWN
// console events have no clean Linux analog.
//
static DWORD
PlatformMapSignalToCtrlType(int Signal)
{
    switch (Signal)
    {
    case SIGINT:
        return CTRL_C_EVENT;
    case SIGQUIT:
        return CTRL_BREAK_EVENT;
    default:
        return (DWORD)-1;
    }
}

//
// Dedicated thread: waits for the handled signals and dispatches them to the
// installed handler in ordinary (async-signal-safe) thread context.
//
static void *
PlatformSignalThread(void * Arg)
{
    sigset_t WaitSet;
    int      Signal;

    (void)Arg;

    sigemptyset(&WaitSet);
    sigaddset(&WaitSet, SIGINT);
    sigaddset(&WaitSet, SIGQUIT);

    while (1)
    {
        if (sigwait(&WaitSet, &Signal) != 0)
        {
            continue;
        }

        DWORD CtrlType = PlatformMapSignalToCtrlType(Signal);

        if (g_PlatformCtrlHandler != NULL && CtrlType != (DWORD)-1)
        {
            g_PlatformCtrlHandler(CtrlType);
        }
    }

    return NULL;
}

BOOLEAN
PlatformInstallCtrlHandler(PLATFORM_CTRL_HANDLER Handler)
{
    sigset_t  BlockSet;
    pthread_t Thread;

    g_PlatformCtrlHandler = Handler;

    //
    // Block the handled signals in this (main) thread. Done before any other
    // thread is created, the mask is inherited by every thread, guaranteeing
    // the signals are delivered to our sigwait() thread and nowhere else.
    //
    sigemptyset(&BlockSet);
    sigaddset(&BlockSet, SIGINT);
    sigaddset(&BlockSet, SIGQUIT);

    if (pthread_sigmask(SIG_BLOCK, &BlockSet, NULL) != 0)
    {
        return FALSE;
    }

    if (pthread_create(&Thread, NULL, PlatformSignalThread, NULL) != 0)
    {
        return FALSE;
    }

    //
    // Fire-and-forget: the thread lives for the life of the process.
    //
    pthread_detach(Thread);

    return TRUE;
}

#else
#    error "Unsupported platform"
#endif
