/**
 * @file MemoryManager.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Reading/Writing memory and all memory affairs
 *
 * @version 0.1
 * @date 2020-04-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Read physical memory from the physical address (used in VMI mode)
 *
 * @param PhysicalAddress Physical Address
 * @param Buffer Buffer to save the memory
 * @param BufferSize Size of the buffer
 *
 * @return BOOLEAN
 */
BOOLEAN
ReadPhysicalMemoryUsingMapIoSpace(PVOID PhysicalAddress, PVOID Buffer, SIZE_T BufferSize)
{
    PHYSICAL_ADDRESS PhysicalAddressTemp = {0};
    PhysicalAddressTemp.QuadPart         = (LONGLONG)PhysicalAddress;

    PVOID VirtualAddress = MmMapIoSpaceEx(PhysicalAddressTemp, BufferSize, PAGE_READWRITE);

    if (VirtualAddress == NULL)
    {
        return FALSE;
    }

    RtlCopyMemory(Buffer, VirtualAddress, BufferSize);

    MmUnmapIoSpace(VirtualAddress, BufferSize);

    return TRUE;
}

/**
 * @brief Write physical memory to the physical address (used in VMI mode)
 *
 * @param PhysicalAddress Physical Address
 * @param Buffer Buffer to write to the memory
 * @param BufferSize Size of the buffer
 *
 * @return BOOLEAN
 */
BOOLEAN
WritePhysicalMemoryUsingMapIoSpace(PVOID PhysicalAddress, PVOID Buffer, SIZE_T BufferSize)
{
    PHYSICAL_ADDRESS PhysicalAddressTemp = {0};
    PhysicalAddressTemp.QuadPart         = (LONGLONG)PhysicalAddress;

    PVOID VirtualAddress = MmMapIoSpaceEx(PhysicalAddressTemp, BufferSize, PAGE_READWRITE);
    if (VirtualAddress == NULL)
    {
        return FALSE;
    }

    RtlCopyMemory(VirtualAddress, Buffer, BufferSize);

    MmUnmapIoSpace(VirtualAddress, BufferSize);

    return TRUE;
}

/**
 * @brief Read process memory
 *
 * @details This function should not be called from vmx-root mode
 *
 * @param PID Target Process Id
 * @param Address Target Address
 * @param MemType Type of memory
 * @param UserBuffer Buffer to save to the user. This buffer must be in nonpageable memory.
 * @param Size Size of read
 * @param ReturnSize Return Size
 *
 * @return BOOLEAN
 */
BOOLEAN
MemoryManagerReadProcessMemoryNormal(HANDLE                    PID,
                                     PVOID                     Address,
                                     DEBUGGER_READ_MEMORY_TYPE MemType,
                                     PVOID                     UserBuffer,
                                     SIZE_T                    Size,
                                     PSIZE_T                   ReturnSize)
{
    PEPROCESS        SourceProcess;
    MM_COPY_ADDRESS  CopyAddress         = {0};
    KAPC_STATE       State               = {0};
    PHYSICAL_ADDRESS TempPhysicalAddress = {0};

    //
    // Check if we want another process memory, this way we attach to that process
    // the find the physical address of the memory and read it from here using physical
    // address

    //
    // The second thing that we consider here is reading a physical address doesn't
    // need to attach to another process
    //
    if (PsGetCurrentProcessId() != PID && MemType == DEBUGGER_READ_VIRTUAL_ADDRESS)
    {
        //
        // User needs another process memory
        //

        if (PsLookupProcessByProcessId(PID, &SourceProcess) != STATUS_SUCCESS)
        {
            //
            // if the process not found
            //
            return FALSE;
        }
        __try
        {
            KeStackAttachProcess(SourceProcess, &State);

            //
            // We're in context of another process let's read the memory
            //
            TempPhysicalAddress = MmGetPhysicalAddress(Address);

            KeUnstackDetachProcess(&State);

            //
            // Now we have to read the physical address
            //
            CopyAddress.PhysicalAddress.QuadPart = TempPhysicalAddress.QuadPart;
            MmCopyMemory(UserBuffer, CopyAddress, Size, MM_COPY_MEMORY_PHYSICAL, ReturnSize);

            ObDereferenceObject(SourceProcess);

            return TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KeUnstackDetachProcess(&State);

            ObDereferenceObject(SourceProcess);

            return FALSE;
        }
    }
    else
    {
        //
        // Process needs itself memory
        //
        __try
        {
            if (MemType == DEBUGGER_READ_VIRTUAL_ADDRESS)
            {
                CopyAddress.VirtualAddress = Address;
                MmCopyMemory(UserBuffer, CopyAddress, Size, MM_COPY_MEMORY_VIRTUAL, ReturnSize);
            }
            else if (MemType == DEBUGGER_READ_PHYSICAL_ADDRESS)
            {
                //
                // Check whether the physical memory is valid or not
                //
                if (!CheckAddressPhysical((UINT64)Address))
                {
                    return FALSE;
                }

                CopyAddress.PhysicalAddress.QuadPart = (LONGLONG)Address;

                if (MmCopyMemory(UserBuffer, CopyAddress, Size, MM_COPY_MEMORY_PHYSICAL, ReturnSize) != STATUS_SUCCESS && *ReturnSize == 0)
                {
                    //
                    // If the memory is not readable
                    // it might be an MMIO address
                    // Try again with another method
                    //
                    if (ReadPhysicalMemoryUsingMapIoSpace((PVOID)Address, UserBuffer, Size))
                    {
                        *ReturnSize = Size;
                        return TRUE;
                    }
                    else
                    {
                        //
                        // unknown error
                        //
                        *ReturnSize = 0;
                        return FALSE;
                    }
                }
            }
            else
            {
                //
                // Type is not recognized
                //
                return FALSE;
            }

            //
            // MmCopyVirtualMemory(SourceProcess, Address, TargetProcess, UserBuffer, Size, KernelMode, ReturnSize);
            // memcpy(UserBuffer, Address, Size);
            //

            return TRUE;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return FALSE;
        }
    }
}
