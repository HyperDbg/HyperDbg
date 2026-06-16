/**
 * @file platform-signal.h
 * @author Max Raulea (max.raulea@gmail.com)
 * @brief User mode cross-platform interface for the console-control (CTRL+C / CTRL+BREAK) handler
 * @details HyperDbg installs a single handler (BreakController) that pauses the
 *          debuggee when the user hits CTRL+C / CTRL+BREAK. The handler body is
 *          platform independent; only the OS mechanism that delivers the event
 *          and the thread context it runs in are OS specific. This interface
 *          isolates that registration. Windows maps onto SetConsoleCtrlHandler;
 *          Linux maps onto POSIX signals serviced by a dedicated sigwait() thread
 *          so the handler runs in ordinary thread context (matching Windows, which
 *          also dispatches the handler on its own thread).
 *
 * @version 0.1
 * @date 2026-06-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//
// Console-control handler signature. Matches the Win32 PHANDLER_ROUTINE shape
// (and BreakController) so the same function pointer is usable on both platforms.
// CtrlType is one of the CTRL_*_EVENT codes.
//
typedef BOOL (*PLATFORM_CTRL_HANDLER)(DWORD CtrlType);

//
// INSTALL the process console-control handler.
// Windows: registers Handler via SetConsoleCtrlHandler.
// Linux:   blocks the relevant signals process-wide and starts a dedicated
//          sigwait() thread that translates them into CTRL_*_EVENT codes and
//          invokes Handler. Must be called once, before other threads spawn,
//          so the blocked-signal mask is inherited by every thread.
// Returns TRUE on success, FALSE on failure.
//
BOOLEAN
PlatformInstallCtrlHandler(PLATFORM_CTRL_HANDLER Handler);
