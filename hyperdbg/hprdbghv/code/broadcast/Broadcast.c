/**
 * @file Broadcast.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Broadcast debugger function to all logical cores
 * @details This file uses DPC to run its functions on all logical cores
 * @version 0.1
 * @date 2020-04-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief routines to broadcast virtualization and vmx initialization
 *  on all cores
 *
 * @return VOID
 */
VOID
BroadcastVmxVirtualizationAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutinePerformVirtualization, NULL);
}

/**
 * @brief routines to set vm-exit on all #DBs and #BP on all cores
 *
 * @return VOID
 */
VOID
BroadcastEnableDbAndBpExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableDbAndBpExitingOnAllCores, NULL);
}

/**
 * @brief routines to unset vm-exit on all #DBs and #BP on all cores
 *
 * @return VOID
 */
VOID
BroadcastDisableDbAndBpExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableDbAndBpExitingOnAllCores, NULL);
}

/**
 * @brief routines to enable vm-exit for breakpoints (exception bitmap)
 *
 * @return VOID
 */
VOID
BroadcastEnableBreakpointExitingOnExceptionBitmapAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableBreakpointOnExceptionBitmapOnAllCores, NULL);
}

/**
 * @brief routines to disable vm-exit for breakpoints (exception bitmap)
 *
 * @return VOID
 */
VOID
BroadcastDisableBreakpointExitingOnExceptionBitmapAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableBreakpointOnExceptionBitmapOnAllCores, NULL);
}

/**
 * @brief routines to set vm-exit on all NMIs on all cores
 *
 * @return VOID
 */
VOID
BroadcastEnableNmiExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableNmiVmexitOnAllCores, NULL);
}

/**
 * @brief routines to set vm-exit on all NMIs on all cores
 *
 * @return VOID
 */
VOID
BroadcastDisableNmiExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableNmiVmexitOnAllCores, NULL);
}

/**
 * @brief routines to notify to invalidate their ept on all cores
 *
 * @return VOID
 */
VOID
BroadcastNotifyAllToInvalidateEptAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineInvalidateEptOnAllCores, (PVOID)TRUE);
}

/**
 * @brief a broadcast that causes vm-exit on all execution of rdtsc/rdtscp
 * @return VOID
 */
VOID
BroadcastEnableRdtscExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableRdtscExitingAllCores, NULL);
}

/**
 * @brief a broadcast that causes for disabling rdtsc/p exiting
 * @return VOID
 */
VOID
BroadcastDisableRdtscExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableRdtscExitingAllCores, NULL);
}

/**
 * @brief routines for !msrread command which
 * @details causes vm-exit on all msr reads
 * @param BitmapMask Bit mask of msr to put on msr bitmap
 * @return VOID
 */
VOID
BroadcastChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineChangeMsrBitmapReadOnAllCores, (PVOID)BitmapMask);
}

/**
 * @brief routines for disable (reset) !msrread command
 * @return VOID
 */
VOID
BroadcastResetChangeAllMsrBitmapReadAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineResetMsrBitmapReadOnAllCores, NULL);
}

/**
 * @brief routines for !msrwrite command which
 * @details causes vm-exit on all msr writes
 * @return VOID
 */
VOID
BroadcastChangeAllMsrBitmapWriteAllCores(UINT64 BitmapMask)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineChangeMsrBitmapWriteOnAllCores, (PVOID)BitmapMask);
}

/**
 * @brief routines for reset !msrwrite command which
 * @return VOID
 */
VOID
BroadcastResetAllMsrBitmapWriteAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineResetMsrBitmapWriteOnAllCores, NULL);
}

/**
 * @brief routines ONLY for disabling !tsc command
 * @return VOID
 */
VOID
BroadcastDisableRdtscExitingForClearingEventsAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableRdtscExitingForClearingTscEventsAllCores, NULL);
}

/**
 * @brief routines ONLY for disabling !crwrite command
 * @param Event
 * @return VOID
 */
VOID
BroadcastDisableMov2ControlRegsExitingForClearingEventsAllCores(PDEBUGGER_EVENT_OPTIONS BroadcastingOption)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableMov2CrExitingForClearingCrEventsAllCores, BroadcastingOption);
}

/**
 * @brief routines ONLY for disabling !dr command
 * @return VOID
 */
VOID
BroadcastDisableMov2DebugRegsExitingForClearingEventsAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableMov2DrExitingForClearingDrEventsAllCores, NULL);
}

/**
 * @brief routines for !pmc
 * @details causes vm-exit on all execution of rdpmc
 * @return VOID
 */
VOID
BroadcastEnableRdpmcExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableRdpmcExitingAllCores, NULL);
}

/**
 * @brief routines for disabling !pmc
 * @return VOID
 */
VOID
BroadcastDisableRdpmcExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableRdpmcExitingAllCores, NULL);
}

/**
 * @brief routines for !exception command which
 * @details causes vm-exit when exception occurred
 * @param ExceptionIndex index of exception on IDT
 *
 * @return VOID
 */
VOID
BroadcastSetExceptionBitmapAllCores(UINT64 ExceptionIndex)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineSetExceptionBitmapOnAllCores, (PVOID)ExceptionIndex);
}

/**
 * @brief routines for disabling exception bitmap
 * @details removes vm-exit when exception occurred
 * @param ExceptionIndex index of exception on IDT
 *
 * @return VOID
 */
VOID
BroadcastUnsetExceptionBitmapAllCores(UINT64 ExceptionIndex)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineUnsetExceptionBitmapOnAllCores, (PVOID)ExceptionIndex);
}

/**
 * @brief routines for reset !exception command
 * @return VOID
 */
VOID
BroadcastResetExceptionBitmapAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineResetExceptionBitmapOnlyOnClearingExceptionEventsOnAllCores, NULL);
}

/**
 * @brief routines for !crwrite
 * @details causes vm-exit on all accesses to debug registers
 * @param Event
 * @return VOID
 */
VOID
BroadcastEnableMovControlRegisterExitingAllCores(PDEBUGGER_EVENT_OPTIONS BroadcastingOption)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableMovControlRegisterExitingAllCores, BroadcastingOption);
}

/**
 * @brief routines for disabling !crwrite
 * @param Event
 * @return VOID
 */
VOID
BroadcastDisableMovToControlRegistersExitingAllCores(PDEBUGGER_EVENT_OPTIONS BroadcastingOption)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableMovControlRegisterExitingAllCores, BroadcastingOption);
}

/**
 * @brief routines for !dr
 * @details causes vm-exit on all accesses to debug registers
 * @return VOID
 */
VOID
BroadcastEnableMovDebugRegistersExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineEnableMovDebigRegisterExitingAllCores, NULL);
}

/**
 * @brief routines for disabling !dr
 * @return VOID
 */
VOID
BroadcastDisableMovDebugRegistersExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineDisableMovDebigRegisterExitingAllCores, NULL);
}

/**
 * @brief routines for !interrupt command which
 * @details causes vm-exit when external interrupt occurs
 * @return VOID
 */
VOID
BroadcastSetExternalInterruptExitingAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineSetEnableExternalInterruptExitingOnAllCores, NULL);
}

/**
 * @brief routines for ONLY terminate !interrupt command
 * @return VOID
 */
VOID
BroadcastUnsetExternalInterruptExitingOnlyOnClearingInterruptEventsAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineSetDisableExternalInterruptExitingOnlyOnClearingInterruptEventsOnAllCores, NULL);
}

/**
 * @brief routines for !ioin and !ioout command which
 * @details causes vm-exit on all i/o instructions or one port
 * @return VOID
 */
VOID
BroadcastIoBitmapChangeAllCores(UINT64 Port)
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineChangeIoBitmapOnAllCores, (PVOID)Port);
}

/**
 * @brief routines for reset !ioin and !ioout command
 * @return VOID
 */
VOID
BroadcastIoBitmapResetAllCores()
{
    //
    // Broadcast to all cores
    //
    KeGenericCallDpc(DpcRoutineResetIoBitmapOnAllCores, NULL);
}

/**
 * @brief routines for debugging threads (enable mov-to-cr3 exiting)
 *
 * @return VOID
 */
VOID
BroadcastEnableMovToCr3ExitingOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineEnableMovToCr3Exiting, 0x0);
}

/**
 * @brief routines for changing EPTP to an MBEC supported EPTP
 *
 * @return VOID
 */
VOID
BroadcastChangeToMbecSupportedEptpOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineChangeToMbecSupportedEptp, 0x0);
}

/**
 * @brief routines for restoring EPTP to normal EPTP
 *
 * @return VOID
 */
VOID
BroadcastRestoreToNormalEptpOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineRestoreToNormalEptp, 0x0);
}

/**
 * @brief routines for disabling MBEC
 *
 * @return VOID
 */
VOID
BroadcasDisableMbecOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineEnableOrDisableMbec, 0x0);
}

/**
 * @brief routines for enabling MBEC
 *
 * @return VOID
 */
VOID
BroadcasEnableMbecOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineEnableOrDisableMbec, (PVOID)0x1);
}

/**
 * @brief routines for debugging threads (disable mov-to-cr3 exiting)
 *
 * @return VOID
 */
VOID
BroadcastDisableMovToCr3ExitingOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineDisableMovToCr3Exiting, 0x0);
}

/**
 * @brief routines for enabling syscall hooks on all cores
 *
 * @return VOID
 */
VOID
BroadcastEnableEferSyscallEventsOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineEnableEferSyscallEvents, 0x0);
}

/**
 * @brief routines for disabling syscall hooks on all cores
 *
 * @return VOID
 */
VOID
BroadcastDisableEferSyscallEventsOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineDisableEferSyscallEvents, 0x0);
}

/**
 * @brief routines for enabling PML on all cores
 *
 * @return VOID
 */
VOID
BroadcastEnablePmlOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineEnablePml, 0x0);
}

/**
 * @brief routines for disabling PML on all cores
 *
 * @return VOID
 */
VOID
BroadcastDisablePmlOnAllProcessors()
{
    KeGenericCallDpc(DpcRoutineDisablePml, 0x0);
}
