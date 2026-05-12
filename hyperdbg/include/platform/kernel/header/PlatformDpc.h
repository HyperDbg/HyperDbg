/**
 * @file PlatformDpc.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for Deferred Procedure Call (DPC) management
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

#if defined(_WIN32) || defined(_WIN64)

VOID
PlatformDpcInitialize(PRKDPC Dpc, PKDEFERRED_ROUTINE DeferredRoutine, PVOID DeferredContext);

BOOLEAN
PlatformDpcInsertQueueDpc(PRKDPC Dpc, PVOID SystemArgument1, PVOID SystemArgument2);

#endif // defined(_WIN32) || defined(_WIN64)
