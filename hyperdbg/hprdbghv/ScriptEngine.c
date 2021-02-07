/**
 * @file ScriptEngine.c
 * @author M.H. Gholamrezei (gholamrezaei.mh@gmail.com)
 * @brief Script engine parser and wrapper functions
 * @details
 * @version 0.1
 * @date 2020-10-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Get current ip from the debugger frame
 * 
 * @return UINT64 returns the rip of the current debuggee state frame
 */
UINT64
ScriptEngineWrapperGetInstructionPointer()
{
    UINT64 GuestRip = NULL;
    ULONG  CurrentProcessorIndex;

    CurrentProcessorIndex = KeGetCurrentProcessorNumber();

    //
    // Check if we are in vmx-root or not
    //
    if (g_GuestState[CurrentProcessorIndex].IsOnVmxRootMode)
    {
        __vmx_vmread(GUEST_RIP, &GuestRip);
        return GuestRip;
    }
    else
    {
        //
        // Otherwise $ip doesn't mean anything
        //
        return NULL;
    }
}

/**
 * @brief Get the address of reserved buffer
 * 
 * @param Action Corresponding action
 * @return UINT64 returns the requested buffer address from user
 */
UINT64
ScriptEngineWrapperGetAddressOfReservedBuffer(PDEBUGGER_EVENT_ACTION Action)
{
    return Action->RequestedBuffer.RequstBufferAddress;
}
