/**
 * @file Broadcast.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Broadcast debugger function to all logical cores
 * @details This file uses DPC to run its functions on all logical cores
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once
#include "Dpc.h"
#include "Vmcall.h"
#include "DebuggerCommands.h"
#include "GlobalVariables.h"
#include "InlineAsm.h"

/**
 * @brief Broadcast syscall hook to all cores
 * 
 * @return VOID 
 */
VOID
BroadcastDpcEnableEferSyscallEvents(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Enable Syscall hook from vmx-root
    //
    AsmVmxVmcall(VMCALL_ENABLE_SYSCALL_HOOK_EFER, 0, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Broadcast syscall unhook to all cores
 * 
 * @return VOID 
 */
VOID
BroadcastDpcDisableEferSyscallEvents(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Disable Syscall hook from vmx-root
    //
    AsmVmxVmcall(VMCALL_DISABLE_SYSCALL_HOOK_EFER, 0, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Broadcast msr write
 * 
 * @return VOID 
 */
VOID
BroadcastDpcWriteMsrToAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    ULONG CurrentProcessorIndex = 0;

    CurrentProcessorIndex = KeGetCurrentProcessorNumber();

    //
    // write on MSR
    //
    __writemsr(g_GuestState[CurrentProcessorIndex].DebuggingState.MsrState.Msr, g_GuestState[CurrentProcessorIndex].DebuggingState.MsrState.Value);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Broadcast msr read
 * 
 * @return VOID 
 */
VOID
BroadcastDpcReadMsrToAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    ULONG CurrentProcessorIndex = 0;

    CurrentProcessorIndex = KeGetCurrentProcessorNumber();

    //
    // read on MSR
    //
    g_GuestState[CurrentProcessorIndex].DebuggingState.MsrState.Value = __readmsr(g_GuestState[CurrentProcessorIndex].DebuggingState.MsrState.Msr);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Disable Msr Bitmaps on all cores (vm-exit on all msrs)
 * 
 * @return VOID 
 */
VOID
BroadcastDpcChangeMsrBitmapReadOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Disable msr bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_CHANGE_MSR_BITMAP_READ, DeferredContext, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Disable Msr Bitmaps on all cores (vm-exit on all msrs)
 * 
 * @return VOID 
 */
VOID
BroadcastDpcChangeMsrBitmapWriteOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Disable msr bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_CHANGE_MSR_BITMAP_WRITE, DeferredContext, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Enables rdtsc/rdtscp exiting in primary cpu-based controls
 * 
 * @return VOID 
 */
VOID
BroadcastDpcEnableRdtscExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Enables rdtsc/rdtscp exiting in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_SET_RDTSC_EXITING, 0, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Enables rdpmc exiting in primary cpu-based controls
 * 
 * @return VOID 
 */
VOID
BroadcastDpcEnableRdpmcExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Enables rdpmc exiting in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_SET_RDPMC_EXITING, 0, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Enable Exception Bitmaps on all cores
 * 
 * @return VOID 
 */
VOID
BroadcastDpcSetExceptionBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Enable Exception Bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_SET_EXCEPTION_BITMAP, DeferredContext, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Enables mov debug registers exitings
 * 
 * @return VOID 
 */
VOID
BroadcastDpcEnableMovDebigRegisterExitingAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Enables mov debug registers exitings in primary cpu-based controls
    //
    AsmVmxVmcall(VMCALL_ENABLE_MOV_TO_DEBUG_REGS_EXITING, 0, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Enable vm-exit on all cores for external interrupts
 * 
 * @return VOID 
 */
VOID
BroadcastDpcSetEnableExternalInterruptExitingOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Enable External Interrupts vm-exit from vmx-root
    //
    AsmVmxVmcall(VMCALL_ENABLE_EXTERNAL_INTERRUPT_EXITING, 0, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}

/**
 * @brief Change I/O Bitmaps on all cores
 * 
 * @return VOID 
 */
VOID
BroadcastDpcChangeIoBitmapOnAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Change I/O Bitmaps on all cores
    //
    AsmVmxVmcall(VMCALL_CHANGE_IO_BITMAP, DeferredContext, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}
