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
        g_GuestState = CrsAllocateNonPagedPool(BufferSizeInByte);

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
    CrsFreePool(g_GuestState);
    g_GuestState = NULL;
}
