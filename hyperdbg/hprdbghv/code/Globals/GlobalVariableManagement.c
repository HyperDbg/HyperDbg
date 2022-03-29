#include "..\hprdbghv\pch.h"

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

VOID GuestStateFreeMemory(VOID)
{
    ExFreePoolWithTag(g_GuestState, POOLTAG);
}