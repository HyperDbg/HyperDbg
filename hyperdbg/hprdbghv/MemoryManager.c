/**
 * @file MemoryManager.c
 * @author Sina Karvandi (sina@rayanfam.com)
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
 * @brief Read process memory
 * 
 * @details This function should not be called from vmx-root mode
 * 
 * @param PID Target Process Id
 * @param Address Target Address
 * @param MemType Type of memory
 * @param UserBuffer Buffer to save to the user
 * @param Size Size of read
 * @param ReturnSize Return Size
 * @return NTSTATUS 
 */
NTSTATUS
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
            return STATUS_UNSUCCESSFUL;
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

            return STATUS_SUCCESS;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            KeUnstackDetachProcess(&State);
            return STATUS_UNSUCCESSFUL;
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
                CopyAddress.PhysicalAddress.QuadPart = Address;
                MmCopyMemory(UserBuffer, CopyAddress, Size, MM_COPY_MEMORY_PHYSICAL, ReturnSize);
            }
            else
            {
                //
                // Type is not recognized
                //
                return STATUS_UNSUCCESSFUL;
            }

            //
            // MmCopyVirtualMemory(SourceProcess, Address, TargetProcess, UserBuffer, Size, KernelMode, ReturnSize);
            // memcpy(UserBuffer, Address, Size);
            //

            return STATUS_SUCCESS;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return STATUS_ACCESS_DENIED;
        }
    }
}
