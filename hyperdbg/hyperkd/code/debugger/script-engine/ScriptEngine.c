/**
 * @file ScriptEngine.c
 * @author Sina Karvandi (sina@hyperdbg.org)
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

/**
 * @brief Create and update the target core date and time
 * @param DbgState The processor debugging state
 *
 * @return VOID
 */
VOID
ScriptEngineUpdateTargetCoreDateTime(PROCESSOR_DEBUGGING_STATE * DbgState)
{
    LARGE_INTEGER SystemTime, LocalTime;
    KeQuerySystemTime(&SystemTime);
    ExSystemTimeToLocalTime(&SystemTime, &LocalTime);
    RtlTimeToTimeFields(&LocalTime, &DbgState->DateTimeHolder.TimeFields);

    sprintf_s(DbgState->DateTimeHolder.TimeBuffer,
              RTL_NUMBER_OF(DbgState->DateTimeHolder.TimeBuffer),
              "%02hd:%02hd:%02hd.%03hd",
              DbgState->DateTimeHolder.TimeFields.Hour,
              DbgState->DateTimeHolder.TimeFields.Minute,
              DbgState->DateTimeHolder.TimeFields.Second,
              DbgState->DateTimeHolder.TimeFields.Milliseconds);

    sprintf_s(DbgState->DateTimeHolder.DateBuffer,
              RTL_NUMBER_OF(DbgState->DateTimeHolder.DateBuffer),
              "%04hd-%02hd-%02hd",
              DbgState->DateTimeHolder.TimeFields.Year,
              DbgState->DateTimeHolder.TimeFields.Month,
              DbgState->DateTimeHolder.TimeFields.Day);
}

/**
 * @brief Update core's date time and return time
 *
 * @return UINT64
 */
UINT64
ScriptEngineGetTargetCoreTime()
{
    ULONG                       CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    PROCESSOR_DEBUGGING_STATE * DbgState    = &g_DbgState[CurrentCore];

    //
    // Update the core's date time
    //
    ScriptEngineUpdateTargetCoreDateTime(DbgState);

    //
    // Return the time
    //
    return (UINT64)&DbgState->DateTimeHolder.TimeBuffer;
}

/**
 * @brief Update core's date time and return date
 *
 * @return UINT64
 */
UINT64
ScriptEngineGetTargetCoreDate()
{
    ULONG                       CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    PROCESSOR_DEBUGGING_STATE * DbgState    = &g_DbgState[CurrentCore];

    //
    // Update the core's date time
    //
    ScriptEngineUpdateTargetCoreDateTime(DbgState);

    //
    // Return the date
    //
    return (UINT64)&DbgState->DateTimeHolder.DateBuffer;
}
