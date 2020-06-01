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
BroadcastDpcDisableMsrBitmapsAllCores(KDPC * Dpc, PVOID DeferredContext, PVOID SystemArgument1, PVOID SystemArgument2)
{
    //
    // Disable msr bitmaps from vmx-root
    //
    AsmVmxVmcall(VMCALL_DISABLE_MSR_BITMAP, 0, 0, 0);

    //
    // Wait for all DPCs to synchronize at this point
    //
    KeSignalCallDpcSynchronize(SystemArgument2);

    //
    // Mark the DPC as being complete
    //
    KeSignalCallDpcDone(SystemArgument1);
}
