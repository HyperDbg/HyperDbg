/**
 * @file GlobalVariableManagement.c
 * @author Behrooz Abbassi (BehroozAbbassi@hyperdbg.org)
 * @brief Management of global variables
 * @details
 * @version 0.1
 * @date 2022-03-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Allocate debugging state memory
 *
 * @return BOOLEAN
 */
BOOLEAN
GlobalDebuggingStateAllocateZeroedMemory(VOID)
{
    SSIZE_T BufferSizeInByte = sizeof(PROCESSOR_DEBUGGING_STATE) * KeQueryActiveProcessorCount(0);

    //
    // Allocate global variable to hold Debugging(s) state
    //
    g_DbgState = ExAllocatePoolWithTag(NonPagedPool, BufferSizeInByte, POOLTAG);

    if (!g_DbgState)
    {
        //
        // we use DbgPrint as the vmx-root or non-root is not initialized
        //

        LogInfo("err, insufficient memory for allocating debugging state\n");
        return FALSE;
    }

    //
    // Zero the memory
    //
    RtlZeroMemory(g_DbgState, BufferSizeInByte);

    return TRUE;
}

/**
 * @brief Free debugging state memory
 *
 * @return VOID
 */
VOID
GlobalDebuggingStateFreeMemory(VOID)
{
    ExFreePoolWithTag(g_DbgState, POOLTAG);
}

/**
 * @brief Allocate guest state memory
 *
 * @return BOOLEAN
 */
BOOLEAN
GlobalGuestStateAllocateZeroedMemory(VOID)
{
    SSIZE_T BufferSizeInByte = sizeof(VIRTUAL_MACHINE_STATE) * KeQueryActiveProcessorCount(0);

    //
    // Allocate global variable to hold Guest(s) state
    //
    if (!g_GuestState)
    {
        g_GuestState = ExAllocatePoolWithTag(NonPagedPool, BufferSizeInByte, POOLTAG);

        if (!g_GuestState)
        {
            LogError("Err, insufficient memory\n");
            return FALSE;
        }
    }

    //
    // Zero the memory
    //
    RtlZeroMemory(g_GuestState, BufferSizeInByte);

    return TRUE;
}

/**
 * @brief Free guest state memory
 *
 * @return VOID
 */
VOID
GlobalGuestStateFreeMemory(VOID)
{
    ExFreePoolWithTag(g_GuestState, POOLTAG);
}
