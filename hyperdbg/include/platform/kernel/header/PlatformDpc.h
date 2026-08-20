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

VOID
PlatformDpcInitialize(PRKDPC Dpc, PKDEFERRED_ROUTINE DeferredRoutine, PVOID DeferredContext);

VOID
PlatformDpcSetTargetProcessor(PRKDPC Dpc, CCHAR Number);

BOOLEAN
PlatformDpcInsertQueueDpc(PRKDPC Dpc, PVOID SystemArgument1, PVOID SystemArgument2);

#if defined(__linux__)

//////////////////////////////////////////////////
//      Linux stand-in for WDK generic DPC      //
//        (placeholder stub, see the .c)        //
//////////////////////////////////////////////////

VOID
KeGenericCallDpc(PKDEFERRED_ROUTINE Routine, PVOID Context);

#endif // defined(__linux__)
