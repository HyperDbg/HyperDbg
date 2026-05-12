/**
 * @file PlatformProcess.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for process and thread queries
 * @details
 * @version 0.19
 * @date 2026-05-09
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

HANDLE
PlatformProcessGetCurrentThreadId(VOID);

HANDLE
PlatformProcessGetCurrentProcessId(VOID);

PVOID
PlatformProcessGetCurrentProcess(VOID);

PVOID
PlatformProcessGetCurrentThread(VOID);

PVOID
PlatformProcessGetCurrentThreadTeb(VOID);
