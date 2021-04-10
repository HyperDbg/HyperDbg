/**
 * @file DebuggerCommands.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @brief Implementation of Debugger Commands 
 * 
 * @version 0.1
 * @date 2020-04-23
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Read memory for different commands
 * 
 * @param ReadMemRequest request structure for reading memory
 * @param UserBuffer user buffer to copy the memory
 * @param ReturnSize size that should be returned to user mode buffers
 * @return NTSTATUS 
 */
NTSTATUS
DebuggerCommandReadMemory(PDEBUGGER_READ_MEMORY ReadMemRequest, PVOID UserBuffer, PSIZE_T ReturnSize)
{
    UINT32                    Pid;
    UINT32                    Size;
    UINT64                    Address;
    DEBUGGER_READ_MEMORY_TYPE MemType;

    Pid     = ReadMemRequest->Pid;
    Size    = ReadMemRequest->Size;
    Address = ReadMemRequest->Address;
    MemType = ReadMemRequest->MemoryType;

    if (Size && Address != NULL)
    {
        return MemoryManagerReadProcessMemoryNormal((HANDLE)Pid, Address, MemType, (PVOID)UserBuffer, Size, ReturnSize);
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }
}

/**
 * @brief Read memory for different commands from vmxroot mode
 * 
 * @param ReadMemRequest request structure for reading memory
 * @param UserBuffer user buffer to copy the memory
 * @param ReturnSize size that should be returned to user mode buffers
 * @return BOOLEAN 
 */
BOOLEAN
DebuggerCommandReadMemoryVmxRoot(PDEBUGGER_READ_MEMORY ReadMemRequest, UCHAR * UserBuffer, PSIZE_T ReturnSize)
{
    UINT32                    Pid;
    UINT32                    Size;
    UINT64                    Address;
    UINT64                    OffsetInUserBuffer;
    DEBUGGER_READ_MEMORY_TYPE MemType;
    PLIST_ENTRY               TempList = 0;

    Pid     = ReadMemRequest->Pid;
    Size    = ReadMemRequest->Size;
    Address = ReadMemRequest->Address;
    MemType = ReadMemRequest->MemoryType;

    //
    // read memory safe
    //
    if (MemType == DEBUGGER_READ_PHYSICAL_ADDRESS)
    {
        MemoryMapperReadMemorySafeByPhysicalAddress(Address, UserBuffer, Size);
    }
    else if (MemType == DEBUGGER_READ_VIRTUAL_ADDRESS)
    {
        //
        // Check whether the virtual memory is available in the current
        // memory layout and also is present in the RAM
        //
        if (!CheckMemoryAccessSafety(Address, Size))
        {
            ReadMemRequest->KernelStatus = DEBUGEER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }

        //
        // Read memory safely
        //
        MemoryMapperReadMemorySafeOnTargetProcess(Address, UserBuffer, Size);

        //
        // Check if the target memory is filled with breakpoint of 'bp' commands
        // if the memory is changed due to this command, then we'll changes it to
        // the previous byte
        //

        //
        // Iterate through the breakpoint list
        //
        TempList = &g_BreakpointsListHead;

        while (&g_BreakpointsListHead != TempList->Flink)
        {
            TempList                                      = TempList->Flink;
            PDEBUGGEE_BP_DESCRIPTOR CurrentBreakpointDesc = CONTAINING_RECORD(TempList, DEBUGGEE_BP_DESCRIPTOR, BreakpointsList);

            if (CurrentBreakpointDesc->Address >= Address && CurrentBreakpointDesc->Address <= Address + Size)
            {
                //
                // The address is found, we have to swap the byte if the target
                // byte is 0xcc
                //

                //
                // Find the address location at user buffer
                //
                OffsetInUserBuffer = CurrentBreakpointDesc->Address - Address;

                if (UserBuffer[OffsetInUserBuffer] == 0xcc)
                {
                    UserBuffer[OffsetInUserBuffer] = CurrentBreakpointDesc->PreviousByte;
                }
            }
        }
    }
    else
    {
        ReadMemRequest->KernelStatus = DEBUGGER_ERROR_MEMORY_TYPE_INVALID;
        return FALSE;
    }
    ReadMemRequest->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;
    *ReturnSize                  = Size;

    return TRUE;
}

/**
 * @brief Perform rdmsr, wrmsr commands
 * 
 * @param ReadOrWriteMsrRequest Msr read/write request
 * @param UserBuffer user buffer to save the results
 * @param ReturnSize return size to user-mode buffers
 * @return NTSTATUS
 */
NTSTATUS
DebuggerReadOrWriteMsr(PDEBUGGER_READ_AND_WRITE_ON_MSR ReadOrWriteMsrRequest, UINT64 * UserBuffer, PSIZE_T ReturnSize)
{
    NTSTATUS Status;
    UINT32   ProcessorCount = KeQueryActiveProcessorCount(0);

    //
    // We don't check whether the MSR is in valid range of hardware or not
    // because the user might send a non-valid MSR which means sth to the
    // Windows or VMM, e.g the range specified for VMMs in Hyper-v
    //

    if (ReadOrWriteMsrRequest->ActionType == DEBUGGER_MSR_WRITE)
    {
        //
        // Set Msr to be applied on the target cores
        //
        if (ReadOrWriteMsrRequest->CoreNumber == DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES)
        {
            //
            // Means that we should apply it on all cores
            //
            for (size_t i = 0; i < ProcessorCount; i++)
            {
                g_GuestState[i].DebuggingState.MsrState.Msr   = ReadOrWriteMsrRequest->Msr;
                g_GuestState[i].DebuggingState.MsrState.Value = ReadOrWriteMsrRequest->Value;
            }
            //
            // Broadcast to all cores to change their Msrs
            //
            KeGenericCallDpc(BroadcastDpcWriteMsrToAllCores, 0x0);
        }
        else
        {
            //
            // We have to change a single core's msr
            //

            //
            // Check if the core number is not invalid
            //
            if (ReadOrWriteMsrRequest->CoreNumber >= ProcessorCount)
            {
                return STATUS_INVALID_PARAMETER;
            }
            //
            // Otherwise it's valid
            //
            g_GuestState[ReadOrWriteMsrRequest->CoreNumber].DebuggingState.MsrState.Msr   = ReadOrWriteMsrRequest->Msr;
            g_GuestState[ReadOrWriteMsrRequest->CoreNumber].DebuggingState.MsrState.Value = ReadOrWriteMsrRequest->Value;

            //
            // Execute it on a single core
            //
            Status = DpcRoutineRunTaskOnSingleCore(ReadOrWriteMsrRequest->CoreNumber, DpcRoutinePerformWriteMsr, NULL);

            *ReturnSize = 0;
            return Status;
        }

        //
        // It's an wrmsr, nothing to return
        //
        *ReturnSize = 0;
        return STATUS_SUCCESS;
    }
    else if (ReadOrWriteMsrRequest->ActionType == DEBUGGER_MSR_READ)
    {
        //
        // Set Msr to be applied on the target cores
        //
        if (ReadOrWriteMsrRequest->CoreNumber == DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES)
        {
            //
            // Means that we should apply it on all cores
            //
            for (size_t i = 0; i < ProcessorCount; i++)
            {
                g_GuestState[i].DebuggingState.MsrState.Msr = ReadOrWriteMsrRequest->Msr;
            }

            //
            // Broadcast to all cores to read their Msrs
            //
            KeGenericCallDpc(BroadcastDpcReadMsrToAllCores, 0x0);

            //
            // When we reach here, all processor read their shits
            // so we have to fill that fucking buffer for usermode
            //
            for (size_t i = 0; i < ProcessorCount; i++)
            {
                UserBuffer[i] = g_GuestState[i].DebuggingState.MsrState.Value;
            }

            //
            // It's an rdmsr we have to return a value for all cores
            //

            *ReturnSize = sizeof(UINT64) * ProcessorCount;
            return STATUS_SUCCESS;
        }
        else
        {
            //
            // Apply to one core
            //

            //
            // Check if the core number is not invalid
            //
            if (ReadOrWriteMsrRequest->CoreNumber >= ProcessorCount)
            {
                *ReturnSize = 0;
                return STATUS_INVALID_PARAMETER;
            }
            //
            // Otherwise it's valid
            //
            g_GuestState[ReadOrWriteMsrRequest->CoreNumber].DebuggingState.MsrState.Msr = ReadOrWriteMsrRequest->Msr;

            //
            // Execute it on a single core
            //
            Status = DpcRoutineRunTaskOnSingleCore(ReadOrWriteMsrRequest->CoreNumber, DpcRoutinePerformReadMsr, NULL);

            if (Status != STATUS_SUCCESS)
            {
                *ReturnSize = 0;
                return Status;
            }
            //
            // Restore the result to the usermode
            //
            UserBuffer[0] = g_GuestState[ReadOrWriteMsrRequest->CoreNumber].DebuggingState.MsrState.Value;

            *ReturnSize = sizeof(UINT64);
            return STATUS_SUCCESS;
        }
    }
    else
    {
        *ReturnSize = 0;
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_UNSUCCESSFUL;
}

/**
 * @brief Edit physical and virtual memory
 * 
 * @param EditMemRequest edit memory request
 * @return NTSTATUS 
 */
NTSTATUS
DebuggerCommandEditMemory(PDEBUGGER_EDIT_MEMORY EditMemRequest)
{
    UINT32   LengthOfEachChunk  = 0;
    PVOID    DestinationAddress = 0;
    PVOID    SourceAddress      = 0;
    CR3_TYPE CurrentProcessCr3;

    //
    // set chunk size in each modification
    //
    if (EditMemRequest->ByteSize == EDIT_BYTE)
    {
        LengthOfEachChunk = 1;
    }
    else if (EditMemRequest->ByteSize == EDIT_DWORD)
    {
        LengthOfEachChunk = 4;
    }
    else if (EditMemRequest->ByteSize == EDIT_QWORD)
    {
        LengthOfEachChunk = 8;
    }
    else
    {
        //
        // Invalid parameter
        //
        EditMemRequest->Result = DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_PARAMETER;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Check if address is valid or not valid (virtual address)
    //
    if (EditMemRequest->MemoryType == EDIT_VIRTUAL_MEMORY)
    {
        if (EditMemRequest->ProcessId == PsGetCurrentProcessId() && VirtualAddressToPhysicalAddress(EditMemRequest->Address) == 0)
        {
            //
            // It's an invalid address in current process
            //
            EditMemRequest->Result = DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_CURRENT_PROCESS;
            return STATUS_UNSUCCESSFUL;
        }
        else if (VirtualAddressToPhysicalAddressByProcessId(EditMemRequest->Address, EditMemRequest->ProcessId) == 0)
        {
            //
            // It's an invalid address in another process
            //
            EditMemRequest->Result = DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_OTHER_PROCESS;
            return STATUS_UNSUCCESSFUL;
        }

        //
        // Edit the memory
        //
        for (size_t i = 0; i < EditMemRequest->CountOf64Chunks; i++)
        {
            DestinationAddress = (UINT64)EditMemRequest->Address + (i * LengthOfEachChunk);
            SourceAddress      = (UINT64)EditMemRequest + SIZEOF_DEBUGGER_EDIT_MEMORY + (i * sizeof(UINT64));

            //
            // Instead of directly accessing the memory we use the MemoryMapperWriteMemorySafe
            // It is because the target page might be read-only so we can make it writable
            //
            // RtlCopyBytes(DestinationAddress, SourceAddress, LengthOfEachChunk);
            MemoryMapperWriteMemoryUnsafe(DestinationAddress, SourceAddress, LengthOfEachChunk, EditMemRequest->ProcessId);
        }
    }
    else if (EditMemRequest->MemoryType == EDIT_PHYSICAL_MEMORY)
    {
        //
        // Edit the physical memory
        //
        for (size_t i = 0; i < EditMemRequest->CountOf64Chunks; i++)
        {
            DestinationAddress = (UINT64)EditMemRequest->Address + (i * LengthOfEachChunk);
            SourceAddress      = (UINT64)EditMemRequest + SIZEOF_DEBUGGER_EDIT_MEMORY + (i * sizeof(UINT64));

            MemoryMapperWriteMemorySafeByPhysicalAddress(DestinationAddress, SourceAddress, LengthOfEachChunk);
        }
    }
    else
    {
        //
        // Invalid parameter
        //
        EditMemRequest->Result = DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_PARAMETER;
        return STATUS_UNSUCCESSFUL;
    }

    //
    // Set the resutls
    //
    EditMemRequest->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

    return STATUS_SUCCESS;
}

/**
 * @brief Edit physical and virtual memory on vmxroot mode
 * 
 * @param EditMemRequest edit memory request
 * @return NTSTATUS 
 */
BOOLEAN
DebuggerCommandEditMemoryVmxRoot(PDEBUGGER_EDIT_MEMORY EditMemRequest)
{
    UINT32   LengthOfEachChunk  = 0;
    PVOID    DestinationAddress = 0;
    PVOID    SourceAddress      = 0;
    CR3_TYPE CurrentProcessCr3;
    //
    // THIS FUNCTION IS SAFE TO BE CALLED FROM VMX ROOT
    //

    //
    // set chunk size in each modification
    //
    if (EditMemRequest->ByteSize == EDIT_BYTE)
    {
        LengthOfEachChunk = 1;
    }
    else if (EditMemRequest->ByteSize == EDIT_DWORD)
    {
        LengthOfEachChunk = 4;
    }
    else if (EditMemRequest->ByteSize == EDIT_QWORD)
    {
        LengthOfEachChunk = 8;
    }
    else
    {
        //
        // Invalid parameter
        //
        EditMemRequest->Result = DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_PARAMETER;
        return FALSE;
    }

    if (EditMemRequest->MemoryType == EDIT_VIRTUAL_MEMORY)
    {
        //
        // Check whether the virtual memory is available in the current
        // memory layout and also is present in the RAM
        //
        if (!CheckMemoryAccessSafety(EditMemRequest->Address,
                                     EditMemRequest->ByteSize * EditMemRequest->CountOf64Chunks))
        {
            EditMemRequest->KernelStatus = DEBUGEER_ERROR_INVALID_ADDRESS;
            return FALSE;
        }

        //
        // Edit the memory
        //
        for (size_t i = 0; i < EditMemRequest->CountOf64Chunks; i++)
        {
            DestinationAddress = (UINT64)EditMemRequest->Address + (i * LengthOfEachChunk);
            SourceAddress      = (UINT64)EditMemRequest + SIZEOF_DEBUGGER_EDIT_MEMORY + (i * sizeof(UINT64));

            //
            // Instead of directly accessing the memory we use the MemoryMapperWriteMemorySafeOnTargetProcess
            // It is because the target page might be read-only so we can make it writable
            //

            // RtlCopyBytes(DestinationAddress, SourceAddress, LengthOfEachChunk);
            MemoryMapperWriteMemorySafeOnTargetProcess(DestinationAddress, SourceAddress, LengthOfEachChunk);
        }
    }
    else if (EditMemRequest->MemoryType == EDIT_PHYSICAL_MEMORY)
    {
        //
        // Edit the physical memory
        //
        for (size_t i = 0; i < EditMemRequest->CountOf64Chunks; i++)
        {
            DestinationAddress = (UINT64)EditMemRequest->Address + (i * LengthOfEachChunk);
            SourceAddress      = (UINT64)EditMemRequest + SIZEOF_DEBUGGER_EDIT_MEMORY + (i * sizeof(UINT64));

            MemoryMapperWriteMemorySafeByPhysicalAddress(DestinationAddress, SourceAddress, LengthOfEachChunk);
        }
    }
    else
    {
        //
        // Invalid parameter
        //
        EditMemRequest->Result = DEBUGGER_ERROR_EDIT_MEMORY_STATUS_INVALID_PARAMETER;
        return FALSE;
    }

    //
    // Set the resutls
    //
    EditMemRequest->Result = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

    return TRUE;
}

/**
 * @brief Search on virtual memory (not work on physical memory)
 * 
 * @details This function should NOT be called from vmx-root mode
 * Do NOT directly call this function as the virtual addresses
 * should be valid on the target process memory layout
 * instead call : SearchAddressWrapper
 * the address between StartAddress and EndAddress should be contiguous
 * 
 * @param AddressToSaveResults Address to save the search results
 * @param SearchMemRequest request structure of searching memory
 * @param StartAddress valid start address based on target process
 * @param EndAddress valid end address based on target process
 * @return VOID the results won't be returned, instead will be
 * saved into AddressToSaveResults
 */
VOID
PerformSearchAddress(UINT64 *                AddressToSaveResults,
                     PDEBUGGER_SEARCH_MEMORY SearchMemRequest,
                     UINT64                  StartAddress,
                     UINT64                  EndAddress)
{
    UINT64   Cmp64                 = 0;
    UINT32   IndexToArrayOfResults = 0;
    UINT32   LengthOfEachChunk     = 0;
    PVOID    DestinationAddress    = 0;
    PVOID    SourceAddress         = 0;
    PVOID    TempSourceAddress     = 0;
    BOOLEAN  StillMatch            = FALSE;
    CR3_TYPE CurrentProcessCr3;

    //
    // set chunk size in each modification
    //
    if (SearchMemRequest->ByteSize == SEARCH_BYTE)
    {
        LengthOfEachChunk = 1;
    }
    else if (SearchMemRequest->ByteSize == SEARCH_DWORD)
    {
        LengthOfEachChunk = 4;
    }
    else if (SearchMemRequest->ByteSize == SEARCH_QWORD)
    {
        LengthOfEachChunk = 8;
    }
    else
    {
        //
        // Invalid parameter
        //
        return;
    }

    //
    // Check if address is virtual address or physical address
    //
    if (SearchMemRequest->MemoryType == SEARCH_VIRTUAL_MEMORY)
    {
        //
        // Search the memory
        //

        //
        // Change the memory layout (cr3), if the user specified a
        // special process
        //
        if (SearchMemRequest->ProcessId != PsGetCurrentProcessId())
        {
            CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayout(SearchMemRequest->ProcessId);
        }

        //
        // Here we iterate through the buffer we received from
        // user-mode
        //
        SourceAddress = (UINT64)SearchMemRequest + SIZEOF_DEBUGGER_SEARCH_MEMORY;

        for (size_t BaseIterator = (size_t)StartAddress; BaseIterator < ((DWORD64)EndAddress); BaseIterator += LengthOfEachChunk)
        {
            //
            // Copy 64bit, 32bit or one byte value into Cmp64 buffer and then compare it
            //
            RtlCopyMemory(&Cmp64, (PVOID)BaseIterator, LengthOfEachChunk);

            //
            // Search the memory
            //
            // Check whether the byte matches the source or not
            //
            if (Cmp64 == *(UINT64 *)SourceAddress)
            {
                //
                // Indicate that it matches until now
                //
                StillMatch = TRUE;

                //
                // Try to check each element (we don't start from the very first element as
                // it checked before )
                //
                for (size_t i = LengthOfEachChunk; i < SearchMemRequest->CountOf64Chunks; i++)
                {
                    //
                    // I know, we have a double check here ;)
                    //
                    TempSourceAddress = (UINT64)SearchMemRequest + SIZEOF_DEBUGGER_SEARCH_MEMORY + (i * sizeof(UINT64));

                    //
                    // Add i to BaseIterator and recompute the Cmp64
                    //
                    RtlCopyMemory(&Cmp64, (PVOID)(BaseIterator + (LengthOfEachChunk * i)), LengthOfEachChunk);

                    if (!(Cmp64 == *(UINT64 *)TempSourceAddress))
                    {
                        //
                        // One thing didn't match so this is not the pattern
                        //
                        StillMatch = FALSE;

                        //
                        // Break from the loop
                        //
                        break;
                    }
                }

                //
                // Check if we find the pattern or not
                //
                if (StillMatch)
                {
                    //
                    // We found the a matching address, let's save the
                    // address for future use
                    //
                    AddressToSaveResults[IndexToArrayOfResults] = BaseIterator;
                    //
                    // Increase the array pointer if it doesn't exceed the limitation
                    //
                    if (MaximumSearchResults > IndexToArrayOfResults)
                    {
                        IndexToArrayOfResults++;
                    }
                    else
                    {
                        //
                        // The result buffer is full !
                        //
                        return;
                    }
                }
            }
            else
            {
                //
                // Not found in the place
                //
                continue;
            }
        }

        //
        // Restore the previous memory layout (cr3), if the user specified a
        // special process
        //
        if (SearchMemRequest->ProcessId != PsGetCurrentProcessId())
        {
            RestoreToPreviousProcess(CurrentProcessCr3);
        }
    }
    else if (SearchMemRequest->MemoryType == SEARCH_PHYSICAL_MEMORY)
    {
        DbgBreakPoint();
    }
    else
    {
        //
        // Invalid parameter
        //
        return;
    }
}

/**
 * @brief The wrapper to check for validity of addresses and call
 * the search routines for both physical and virtual memory
 * 
 * @details This function should NOT be called from vmx-root mode
 * The address between start address and end address will be checked
 * to make a contiguous address
 * 
 * @param AddressToSaveResults Address to save the search results
 * @param SearchMemRequest request structure of searching memory
 * @param StartAddress start address of searching based on target process
 * @param EndAddress start address of searching based on target process
 * @return VOID the results won't be returned, instead will be
 * saved into AddressToSaveResults 
 */
VOID
SearchAddressWrapper(PUINT64 AddressToSaveResults, PDEBUGGER_SEARCH_MEMORY SearchMemRequest, UINT64 StartAddress, UINT64 EndAddress)
{
    CR3_TYPE CurrentProcessCr3;
    UINT32   ProcId              = 0;
    UINT64   BaseAddress         = 0;
    UINT64   CurrentValue        = 0;
    UINT64   RealPhysicalAddress = 0;
    BOOLEAN  DoesBaseAddrSaved   = FALSE;

    if (SearchMemRequest->MemoryType == SEARCH_VIRTUAL_MEMORY)
    {
        //
        // It's a virtual address search
        //

        //
        // Align the page and search with alignement
        //
        StartAddress = PAGE_ALIGN(StartAddress);

        //
        // Switch to new process's memory layout
        //
        CurrentProcessCr3 = SwitchOnAnotherProcessMemoryLayout(SearchMemRequest->ProcessId);

        //
        // We will try to find a contigues address
        //
        while (StartAddress < EndAddress)
        {
            //
            // Check if address is valid or not
            // Generally, we can use VirtualAddressToPhysicalAddressByProcessId
            // but let's not change the cr3 multiple times
            //
            if (VirtualAddressToPhysicalAddress(StartAddress, SearchMemRequest->ProcessId) != 0)
            {
                //
                // Address is valid, let's add a page size to it
                // nothing to do
                //
                if (!DoesBaseAddrSaved)
                {
                    BaseAddress       = StartAddress;
                    DoesBaseAddrSaved = TRUE;
                }
            }
            else
            {
                //
                // Address is not valid anymore
                //
                break;
            }
            StartAddress += PAGE_SIZE;
        }

        //
        // Restore the original process
        //
        RestoreToPreviousProcess(CurrentProcessCr3);

        //
        // All of the address chunk was valid
        //
        if (DoesBaseAddrSaved && StartAddress > BaseAddress)
        {
            PerformSearchAddress(AddressToSaveResults, SearchMemRequest, BaseAddress, StartAddress);
        }
        else
        {
            return;
        }
    }
    else if (SearchMemRequest->MemoryType == SEARCH_PHYSICAL_MEMORY)
    {
        //
        // when we reached here, we know that it's a valid physical memory,
        // so we change the structure and pass it as a virtual address to
        // the search function
        //
        RealPhysicalAddress = SearchMemRequest->Address;

        //
        // Change the start address
        //
        if (SearchMemRequest->ProcessId == PsGetCurrentProcessId())
        {
            SearchMemRequest->Address = PhysicalAddressToVirtualAddress(StartAddress);
            EndAddress                = PhysicalAddressToVirtualAddress(EndAddress);
        }
        else
        {
            SearchMemRequest->Address = PhysicalAddressToVirtualAddressByProcessId(StartAddress, SearchMemRequest->ProcessId);
            EndAddress                = PhysicalAddressToVirtualAddressByProcessId(EndAddress, SearchMemRequest->ProcessId);
        }

        //
        // Change the type of memory
        //
        SearchMemRequest->MemoryType = SEARCH_VIRTUAL_MEMORY;

        //
        // Call the wrapper
        //
        PerformSearchAddress(AddressToSaveResults, SearchMemRequest, SearchMemRequest->Address, EndAddress);

        //
        // Restore the previous state
        //
        SearchMemRequest->MemoryType = SEARCH_PHYSICAL_MEMORY;
        SearchMemRequest->Address    = RealPhysicalAddress;

        //
        // Save the process id to avoid calling PsGetCurrentProcessId()
        // multiple times
        //
        ProcId = PsGetCurrentProcessId();

        //
        // Results should be ready (if any) in AddressToSaveResults
        // we should convert it to a physical format as the search
        // was on virtual format
        //
        for (size_t i = 0; i < MaximumSearchResults; i++)
        {
            CurrentValue = AddressToSaveResults[i];

            if (SearchMemRequest->ProcessId == ProcId && CurrentValue != NULL)
            {
                AddressToSaveResults[i] = VirtualAddressToPhysicalAddress(CurrentValue);
            }
            else if (CurrentValue != NULL)
            {
                AddressToSaveResults[i] = VirtualAddressToPhysicalAddressByProcessId(CurrentValue, SearchMemRequest->ProcessId);
            }
            else
            {
                break;
            }
        }

        return;
    }
}

/**
 * @brief Start searching memory
 * 
 * @param SearchMemRequest Request to search memory
 * @return NTSTATUS 
 */
NTSTATUS
DebuggerCommandSearchMemory(PDEBUGGER_SEARCH_MEMORY SearchMemRequest)
{
    PUINT64 SearchResultsStorage = NULL;
    PUINT64 UsermodeBuffer       = NULL;
    UINT64  AddressFrom          = 0;
    UINT64  AddressTo            = 0;
    UINT64  CurrentValue         = 0;
    UINT32  ResultsIndex         = 0;

    //
    // Check if process id is valid or not
    //
    if (SearchMemRequest->ProcessId != PsGetCurrentProcessId() && !IsProcessExist(SearchMemRequest->ProcessId))
    {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // User-mode buffer is same as SearchMemRequest
    //
    UsermodeBuffer = SearchMemRequest;

    //
    // We store the user-mode data in a seprate variable because
    // we will use them later when we Zeroed the SearchMemRequest
    //
    AddressFrom = SearchMemRequest->Address;
    AddressTo   = SearchMemRequest->Address + SearchMemRequest->Length;

    //
    // We support up to MaximumSearchResults search results
    //
    SearchResultsStorage = ExAllocatePoolWithTag(NonPagedPool, MaximumSearchResults * sizeof(UINT64), POOLTAG);

    if (SearchResultsStorage == NULL)
    {
        //
        // Not enough memory
        //
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Make sure there is nothing else in the buffer
    //
    RtlZeroMemory(SearchResultsStorage, MaximumSearchResults * sizeof(UINT64));

    //
    // Call the wrapper
    //
    SearchAddressWrapper(SearchResultsStorage, SearchMemRequest, AddressFrom, AddressTo);

    //
    // In this point, we to store the results (if any) to the user-mode
    // buffer SearchMemRequest itself is the user-mode buffer and we also
    // checked from the previous function that the output buffer is at
    // least SearchMemRequest bigger or equal to MaximumSearchResults * sizeof(UINT64)
    // so we need to clear everything here, and also we should keep in mind that
    // SearchMemRequest is no longer valid
    //
    RtlZeroMemory(SearchMemRequest, MaximumSearchResults * sizeof(UINT64));

    //
    // It's time to move the results from our temporary buffer to the user-mode
    // buffer, also there is something that we should check and that's the fact
    // that we used aligned page addresses so the results should be checked to
    // see whether the results are between the user's entered addresses or not
    //
    for (size_t i = 0; i < MaximumSearchResults; i++)
    {
        CurrentValue = SearchResultsStorage[i];

        if (CurrentValue == NULL)
        {
            //
            // Nothing left to move
            //
            break;
        }

        if (CurrentValue >= AddressFrom && CurrentValue <= AddressTo)
        {
            //
            // Move the variable
            //
            UsermodeBuffer[ResultsIndex] = CurrentValue;
            ResultsIndex++;
        }
    }

    //
    // Free the results pool
    //
    ExFreePoolWithTag(SearchResultsStorage, POOLTAG);

    return STATUS_SUCCESS;
}

/**
 * @brief Perform the flush requests to vmx-root and vmx non-root buffers
 * 
 * @param DebuggerFlushBuffersRequest Request to flush the buffers
 * @return NTSTATUS 
 */
NTSTATUS
DebuggerCommandFlush(PDEBUGGER_FLUSH_LOGGING_BUFFERS DebuggerFlushBuffersRequest)
{
    //
    // We try to flush buffers for both vmx-root and regular kernel buffer
    //
    DebuggerFlushBuffersRequest->CountOfMessagesThatSetAsReadFromVmxRoot    = LogMarkAllAsRead(TRUE);
    DebuggerFlushBuffersRequest->CountOfMessagesThatSetAsReadFromVmxNonRoot = LogMarkAllAsRead(FALSE);
    DebuggerFlushBuffersRequest->KernelStatus                               = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

    return STATUS_SUCCESS;
}

/**
 * @brief Perform the command finished signal
 * 
 * @param DebuggerFinishedExecutionRequest Request to 
 * signal debuggee about execution state
 * @return NTSTATUS 
 */
NTSTATUS
DebuggerCommandSignalExecutionState(PDEBUGGER_SEND_COMMAND_EXECUTION_FINISHED_SIGNAL DebuggerFinishedExecutionRequest)
{
    //
    // It's better to send the signal from vmx-root mode
    //
    AsmVmxVmcall(VMCALL_SIGNAL_DEBUGGER_EXECUTION_FINISHED, 0, 0, 0);

    DebuggerFinishedExecutionRequest->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

    return STATUS_SUCCESS;
}

/**
 * @brief Send the user-mode buffer to debugger
 * 
 * @param DebuggerSendUsermodeMessageRequest Request to send message to debugger
 * @return NTSTATUS 
 */
NTSTATUS
DebuggerCommandSendMessage(PDEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER DebuggerSendUsermodeMessageRequest)
{
    //
    // It's better to send the signal from vmx-root mode to avoid deadlock
    //
    AsmVmxVmcall(VMCALL_SEND_MESSAGES_TO_DEBUGGER,
                 (UINT64)DebuggerSendUsermodeMessageRequest + (SIZEOF_DEBUGGER_SEND_USERMODE_MESSAGES_TO_DEBUGGER),
                 DebuggerSendUsermodeMessageRequest->Length,
                 0);

    DebuggerSendUsermodeMessageRequest->KernelStatus = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

    return STATUS_SUCCESS;
}

/**
 * @brief Send general buffers from the debuggee to the debugger
 * 
 * @param DebuggeeBufferRequest Request to buffer that will be sent to the debugger
 * @return NTSTATUS 
 */
NTSTATUS
DebuggerCommandSendGeneralBufferToDebugger(PDEBUGGEE_SEND_GENERAL_PACKET_FROM_DEBUGGEE_TO_DEBUGGER DebuggeeBufferRequest)
{
    //
    // It's better to send the signal from vmx-root mode to avoid deadlock
    //
    AsmVmxVmcall(VMCALL_SEND_GENERAL_BUFFER_TO_DEBUGGER,
                 DebuggeeBufferRequest,
                 0,
                 0);

    DebuggeeBufferRequest->KernelResult = DEBUGEER_OPERATION_WAS_SUCCESSFULL;

    return STATUS_SUCCESS;
}
