/**
 * @file DebuggerCommands.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Implementation of Debugger Commands 
 * 
 * @version 0.1
 * @date 2020-04-23
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

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

NTSTATUS
DebuggerCommandEditMemory(PDEBUGGER_EDIT_MEMORY EditMemRequest)
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
        EditMemRequest->Result = DEBUGGER_EDIT_MEMORY_STATUS_INVALID_PARAMETER;
        return;
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
            EditMemRequest->Result = DEBUGGER_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_CURRENT_PROCESS;
            return;
        }
        else if (VirtualAddressToPhysicalAddressByProcessId(EditMemRequest->Address, EditMemRequest->ProcessId) == 0)
        {
            //
            // It's an invalid address in another process
            //
            EditMemRequest->Result = DEBUGGER_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_OTHER_PROCESS;
            return;
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
            MemoryMapperWriteMemorySafe(DestinationAddress, SourceAddress, LengthOfEachChunk, EditMemRequest->ProcessId);
        }
    }
    else
    {
        //
        // It's a physical address so let's check if it's valid or not
        // honestly, I don't know if it's a good way to check whether the
        // physical address is valid or not by converting it to virtual address
        // there might be address which are not mapped to a virtual address but
        // we nedd to modify them, so it might be wrong to check it this way but
        // let's implement it like this for now, if you know a better way to check
        // please ping me (@Sinaei)
        //
        if (EditMemRequest->ProcessId == PsGetCurrentProcessId() && PhysicalAddressToVirtualAddress(EditMemRequest->Address) == 0)
        {
            //
            // It's an invalid address in current process
            //
            EditMemRequest->Result = DEBUGGER_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_CURRENT_PROCESS;
            return;
        }
        else if (PhysicalAddressToVirtualAddressByProcessId(EditMemRequest->Address, EditMemRequest->ProcessId) == 0)
        {
            //
            // It's an invalid address in another process
            //
            EditMemRequest->Result = DEBUGGER_EDIT_MEMORY_STATUS_INVALID_ADDRESS_BASED_ON_OTHER_PROCESS;
            return;
        }

        //
        // Edit the memory
        //
        for (size_t i = 0; i < EditMemRequest->CountOf64Chunks; i++)
        {
            DestinationAddress = (UINT64)EditMemRequest->Address + (i * LengthOfEachChunk);
            SourceAddress      = (UINT64)EditMemRequest + SIZEOF_DEBUGGER_EDIT_MEMORY + (i * sizeof(UINT64));

            MemoryMapperWriteMemorySafeByPhysicalAddress(DestinationAddress, SourceAddress, LengthOfEachChunk, EditMemRequest->ProcessId);
        }
    }

    //
    // Set the resutls
    //
    EditMemRequest->Result = DEBUGGER_EDIT_MEMORY_STATUS_SUCCESS;
}

NTSTATUS
DebuggerCommandSearchMemory(PDEBUGGER_SEARCH_MEMORY SearchMemRequest)
{
}
