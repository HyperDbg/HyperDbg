/**
 * @file PlatformBroadcast.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Cross platform APIs for broadcasting routines
 * @details
 * @version 0.19
 * @date 2026-05-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#if defined(__linux__)
#    include "../../../../include/SDK/HyperDbgSdk.h"
#endif // defined(__linux__)

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

#if defined(_WIN32) || defined(_WIN64)

NTKERNELAPI
_IRQL_requires_max_(APC_LEVEL)
_IRQL_requires_min_(PASSIVE_LEVEL)
_IRQL_requires_same_
VOID
KeGenericCallDpc(
    _In_ PKDEFERRED_ROUTINE Routine,
    _In_opt_ PVOID          Context);

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
VOID
KeSignalCallDpcDone(
    _In_ PVOID SystemArgument1);

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL)
_IRQL_requires_same_
LOGICAL
KeSignalCallDpcSynchronize(
    _In_ PVOID SystemArgument2);

#endif // defined(_WIN32) || defined(_WIN64)

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

VOID
PlatformBroadcastSynchronizeEndOfRoutine(PVOID SystemArgument1, PVOID SystemArgument2);
