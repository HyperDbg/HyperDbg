/**
 * @file ScriptEngine.c
 * @author M.H. Gholamrezaei (mh@hyperdbg.org)
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
    //
    // Check if we are in vmx-root or not
    //
    if (VmFuncVmxGetCurrentExecutionMode() == TRUE)
    {
        return VmFuncGetRip();
    }
    else
    {
        //
        // Otherwise $ip doesn't mean anything
        //
        return (UINT64)NULL;
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
