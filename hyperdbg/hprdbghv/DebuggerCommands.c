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
#include "Broadcast.h"
#include "Dpc.h"
#include "Debugger.h"
#include "Logging.h"
#include "Common.h"
#include "GlobalVariables.h"

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
DebuggerReadOrWriteMsr(PDEBUGGER_READ_AND_WRITE_ON_MSR ReadOrWriteMsrRequest, PSIZE_T ReturnSize)
{
    //
    // We don't check whether the MSR is in valid range of hardware or not
    // because the user might send a non-valid MSR which means sth to the
    // Windows or VMM, e.g the range specified for VMMs in Hyper-v
    //

    if (ReadOrWriteMsrRequest->ActionType == DEBUGGER_MSR_WRITE)
    {
        //
        // Apply the wrmsr
        //
        __writemsr(ReadOrWriteMsrRequest->Msr, ReadOrWriteMsrRequest->Value);

        //
        // It's an wrmsr, nothing to return
        //
        *ReturnSize = 0;
    }
    else if (ReadOrWriteMsrRequest->ActionType == DEBUGGER_MSR_READ)
    {
        //
        // Apply the rdmsr
        //
        ReadOrWriteMsrRequest->Value = __readmsr(ReadOrWriteMsrRequest->Msr);

        //
        // As this is a pointer to system buffer so we send its
        // size to the I/O manager and I/O manager sends usermode
        // buffer to the usermode again, this time with the actual
        // value of MSR which is read by rdmsr
        //
        *ReturnSize = SIZEOF_READ_AND_WRITE_ON_MSR;
    }
    else
    {
        *ReturnSize = 0;
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}
