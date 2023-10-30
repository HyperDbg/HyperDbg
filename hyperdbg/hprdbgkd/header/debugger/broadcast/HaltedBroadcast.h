/**
 * @file HaltedBroadcast.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers for broadcasting functions in case of halted cores
 *
 * @version 0.7
 * @date 2023-10-19
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				    Functions					//
//////////////////////////////////////////////////

VOID
HaltedBroadcastChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask);

VOID
HaltedBroadcastChangeAllMsrBitmapWriteAllCores(UINT64 BitmapMask);

VOID
HaltedBroadcastChangeAllIoBitmapAllCores(UINT64 Port);

VOID
HaltedBroadcastEnableRdpmcExitingAllCores();

VOID
HaltedBroadcastEnableRdtscExitingAllCores();

VOID
HaltedBroadcastEnableMov2DebugRegsExitingAllCores();

VOID
HaltedBroadcastEnableExternalInterruptExitingAllCores();

VOID
HaltedBroadcastSetExceptionBitmapAllCores(UINT64 ExceptionIndex);

VOID
HaltedBroadcastUnSetExceptionBitmapAllCores(UINT64 ExceptionIndex);

VOID
HaltedBroadcastEnableMovToCrExitingAllCores(DEBUGGER_EVENT_OPTIONS * BroadcastingOption);

VOID
HaltedBroadcastEnableEferSyscallHookAllCores();

VOID
HaltedBroadcastInvalidateEptAllContextsAllCores();

VOID
HaltedBroadcastInvalidateSingleContextAllCores();

VOID
HaltedBroadcastUnhookSinglePageAllCores(EPT_SINGLE_HOOK_UNHOOKING_DETAILS * UnhookingDetail);

VOID
HaltedBroadcastSetDisableExternalInterruptExitingOnlyOnClearingInterruptEventsAllCores();

VOID
HaltedBroadcastResetMsrBitmapReadAllCores();

VOID
HaltedBroadcastResetMsrBitmapWriteAllCores();

VOID
HaltedBroadcastResetExceptionBitmapOnlyOnClearingExceptionEventsAllCores();

VOID
HaltedBroadcastResetIoBitmapAllCores();

VOID
HaltedBroadcastDisableRdtscExitingForClearingTscEventsAllCores();

VOID
HaltedBroadcastDisableRdpmcExitingAllCores();

VOID
HaltedBroadcastDisableEferSyscallEventsAllCores();

VOID
HaltedBroadcastDisableMov2DrExitingForClearingDrEventsAllCores();

VOID
HaltedBroadcastDisableMov2CrExitingForClearingCrEventsAllCores(DEBUGGER_EVENT_OPTIONS * BroadcastingOption);
