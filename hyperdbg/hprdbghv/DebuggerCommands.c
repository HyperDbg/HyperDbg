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
DebuggerCommandReadMemory(PDEBUGGER_READ_MEMORY DebuggerReadMem, PVOID UserBuffer, PSIZE_T ReturnSize)
{
    UINT32                    Pid;
    UINT32                    Size;
    UINT64                    Address;
    DEBUGGER_READ_MEMORY_TYPE MemType;

    Pid     = DebuggerReadMem->Pid;
    Size    = DebuggerReadMem->Size;
    Address = DebuggerReadMem->Address;
    MemType = DebuggerReadMem->MemoryType;

    if (Size && Address != NULL)
    {
        return MemoryManagerReadProcessMemoryNormal((HANDLE)Pid, Address, MemType, (PVOID)UserBuffer, Size, ReturnSize);
    }
    else
    {
        return STATUS_UNSUCCESSFUL;
    }
}
