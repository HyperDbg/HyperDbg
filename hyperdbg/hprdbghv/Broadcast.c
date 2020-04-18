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