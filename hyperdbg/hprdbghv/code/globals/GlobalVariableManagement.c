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
#include "..\hprdbghv\pch.h"

/**
 * @brief Allocate guest state memory
 * 
 * @return NTSTATUS
*/
NTSTATUS
GuestStateAllocateZeroedMemory(VOID)
{
    SSIZE_T BufferSizeInByte = sizeof(VIRTUAL_MACHINE_STATE) * KeQueryActiveProcessorCount(0);

    //
    // Allocate global variable to hold Guest(s) state
    //

    g_GuestState = ExAllocatePoolWithTag(NonPagedPool, BufferSizeInByte, POOLTAG);
    if (!g_GuestState)
    {
        //
        // we use DbgPrint as the vmx-root or non-root is not initialized
        //

        DbgPrint("err, insufficient memory\n");
        DbgBreakPoint();
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Zero the memory
    //
    RtlZeroMemory(g_GuestState, BufferSizeInByte);

    return STATUS_SUCCESS;
}

/**
 * @brief Free guest state memory
 * 
 * @return NTSTATUS
*/
VOID GuestStateFreeMemory(VOID)
{
    ExFreePoolWithTag(g_GuestState, POOLTAG);
}
