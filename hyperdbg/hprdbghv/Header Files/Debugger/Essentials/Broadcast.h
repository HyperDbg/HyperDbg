/**
 * @file Broadcast.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The broadcast (DPC) function to all the cores for debugger commands
 * @details 
 * @version 0.1
 * @date 2020-04-17
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
BroadcastDpcEnableMovToCr3Exiting(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcDisableMovToCr3Exiting(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcEnableEferSyscallEvents(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcDisableEferSyscallEvents(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcWriteMsrToAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcReadMsrToAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcChangeMsrBitmapReadOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcResetMsrBitmapReadOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcChangeMsrBitmapWriteOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcResetMsrBitmapWriteOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcEnableRdtscExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcDisableRdtscExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcEnableRdpmcExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcDisableRdpmcExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcSetExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcResetExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcEnableMovDebigRegisterExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcDisableMovDebigRegisterExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcSetEnableExternalInterruptExitingOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcSetDisableExternalInterruptExitingOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcChangeIoBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcResetIoBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcEnableBreakpointOnExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcDisableBreakpointOnExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcDisableBreakpointOnExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcEnableNmiVmexitOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcDisableNmiVmexitOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcVmExitAndHaltSystemAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcEnableDbAndBpExitingOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);

VOID
BroadcastDpcDisableDbAndBpExitingOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2);
