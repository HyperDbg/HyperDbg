/**
 * @file pt.cpp
 * @author Masoud Rahimi Jafari (Masoodrahimy1379@gmail.com)
 * @brief !pt command
 * @details
 * @version 0.19
 * @date 2026-04-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsHyperTraceModuleLoaded;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of the !pt command
 *
 * @return VOID
 */
VOID
CommandPtHelp()
{
    ShowMessages("!pt : enables, disables and configures Intel Processor Trace (PT).\n");

    ShowMessages("syntax : \t!pt [Function (string)]\n");
    ShowMessages("syntax : \t!pt filter [user] [kernel] [cr3 <hex>] [buffer <hex>]\n");
    ShowMessages("\t              [range <start> <end>] [stoprange <start> <end>]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !pt enable\n");
    ShowMessages("\t\te.g : !pt disable\n");
    ShowMessages("\t\te.g : !pt pause\n");
    ShowMessages("\t\te.g : !pt resume\n");
    ShowMessages("\t\te.g : !pt size\n");
    ShowMessages("\t\te.g : !pt dump\n");
    ShowMessages("\t\te.g : !pt flush\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !pt filter user\n");
    ShowMessages("\t\te.g : !pt filter kernel\n");
    ShowMessages("\t\te.g : !pt filter user kernel\n");
    ShowMessages("\t\te.g : !pt filter user cr3 0x1aabb000\n");
    ShowMessages("\t\te.g : !pt filter user buffer 0x100000\n");
    ShowMessages("\t\te.g : !pt filter user range 0x140001000 0x140002000\n");
    ShowMessages("\t\te.g : !pt filter user stoprange 0x140003000 0x140004000\n");

    ShowMessages("\nlist of filter options: \n");
    ShowMessages("\t user                : trace CPL > 0\n");
    ShowMessages("\t kernel              : trace CPL == 0\n");
    ShowMessages("\t cr3 <addr>          : only trace when CR3 matches <addr> (0 = no filter)\n");
    ShowMessages("\t buffer <bytes>      : per-CPU output buffer size, must be 4KB * 2^N\n");
    ShowMessages("\t                       (4KB, 8KB, ... up to 128MB; default 2MB)\n");
    ShowMessages("\t range <start> <end> : keep trace inside [start..end] (up to 4 ranges)\n");
    ShowMessages("\t stoprange <s> <e>   : stop tracing when execution enters [s..e]\n");
    ShowMessages("\t (no option)         : trace user + kernel, no CR3 / IP filter (default)\n");
}

/**
 * @brief Send PT requests
 *
 * @param PtRequest
 *
 * @return VOID
 */
BOOLEAN
CommandPtSendRequest(HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    BOOL  Status;
    ULONG ReturnedLength;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send the request over serial kernel debugger
        //
        if (!KdSendHyperTracePtPacketsToDebuggee(PtRequest, SIZEOF_HYPERTRACE_PT_OPERATION_PACKETS))
        {
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        AssertShowMessageReturnStmt(g_IsHyperTraceModuleLoaded, g_DeviceHandle, ASSERT_MESSAGE_HYPERTRACE_NOT_LOADED, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

        //
        // Send IOCTL
        //
        Status = DeviceIoControl(
            g_DeviceHandle,                         // Handle to device
            IOCTL_PERFORM_HYPERTRACE_PT_OPERATION,  // IO Control Code (IOCTL)
            PtRequest,                              // Input Buffer to driver.
            SIZEOF_HYPERTRACE_PT_OPERATION_PACKETS, // Input buffer length
            PtRequest,                              // Output Buffer from driver.
            SIZEOF_HYPERTRACE_PT_OPERATION_PACKETS, // Length of output buffer in bytes.
            &ReturnedLength,                        // Bytes placed in buffer.
            NULL                                    // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", GetLastError());

            return FALSE;
        }

        if (PtRequest->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
}

/**
 * @brief Request to perform an PT operation
 *
 * @param PtRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgPerformPtOperation(HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    return CommandPtSendRequest(PtRequest);
}

/**
 * @brief Map the per-CPU PT output buffers into the current process
 *
 * @details On success MmapRequest->Cpus[0..NumCpus) hold one { UserVa, Size }
 *          per CPU, valid in this process until PT is disabled / flushed.
 *          Only meaningful in local (VMI) mode.
 *
 * @param MmapRequest
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgPtMmapSendRequest(HYPERTRACE_PT_MMAP_PACKETS * MmapRequest)
{
    BOOL  Status;
    ULONG ReturnedLength;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // The mmap surface maps into the caller's address space, which only
        // makes sense in local mode (no remote-debuggee transport for it).
        //
        ShowMessages("err, PT mmap is only available in local (VMI) mode\n");
        return FALSE;
    }

    AssertShowMessageReturnStmt(g_IsHyperTraceModuleLoaded, g_DeviceHandle, ASSERT_MESSAGE_HYPERTRACE_NOT_LOADED, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturnFalse);

    Status = DeviceIoControl(
        g_DeviceHandle,                    // Handle to device
        IOCTL_PERFORM_HYPERTRACE_PT_MMAP,  // IO Control Code (IOCTL)
        MmapRequest,                       // Input Buffer to driver.
        SIZEOF_HYPERTRACE_PT_MMAP_PACKETS, // Input buffer length
        MmapRequest,                       // Output Buffer from driver.
        SIZEOF_HYPERTRACE_PT_MMAP_PACKETS, // Length of output buffer in bytes.
        &ReturnedLength,                   // Bytes placed in buffer.
        NULL                               // synchronous call
    );

    if (!Status)
    {
        ShowMessages("ioctl failed with code 0x%x\n", GetLastError());
        return FALSE;
    }

    return MmapRequest->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL;
}

/**
 * @brief Parse a `!pt filter ...` clause into a HYPERTRACE_PT_OPERATION_PACKETS.
 *
 *        Returns TRUE on success, FALSE if the syntax was bad (caller
 *        should print help and bail).
 */
static BOOLEAN
CommandPtParseFilterOptions(vector<CommandToken> &            CommandTokens,
                            HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    BOOLEAN AnyMode = FALSE;

    PtRequest->TraceUser     = 0;
    PtRequest->TraceKernel   = 0;
    PtRequest->TargetCr3     = 0;
    PtRequest->BufferSize    = 0;
    PtRequest->NumAddrRanges = 0;

    for (size_t i = 2; i < CommandTokens.size(); i++)
    {
        if (CompareLowerCaseStrings(CommandTokens.at(i), "user"))
        {
            PtRequest->TraceUser = 1;
            AnyMode              = TRUE;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "kernel"))
        {
            PtRequest->TraceKernel = 1;
            AnyMode                = TRUE;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "cr3"))
        {
            if (i + 1 >= CommandTokens.size())
            {
                ShowMessages("err, '%s' expects a value\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                return FALSE;
            }
            i++;
            if (!ConvertTokenToUInt64(CommandTokens.at(i), &PtRequest->TargetCr3))
            {
                ShowMessages("err, '%s' is not a valid number\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                return FALSE;
            }
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "buffer"))
        {
            if (i + 1 >= CommandTokens.size())
            {
                ShowMessages("err, 'buffer' expects a size in bytes\n");
                return FALSE;
            }
            i++;
            if (!ConvertTokenToUInt64(CommandTokens.at(i), &PtRequest->BufferSize))
            {
                ShowMessages("err, '%s' is not a valid number\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                return FALSE;
            }
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "range") ||
                 CompareLowerCaseStrings(CommandTokens.at(i), "stoprange"))
        {
            BOOLEAN IsStop = CompareLowerCaseStrings(CommandTokens.at(i), "stoprange");

            if (i + 2 >= CommandTokens.size())
            {
                ShowMessages("err, '%s' expects <start> <end>\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                return FALSE;
            }

            if (PtRequest->NumAddrRanges >= PT_MAX_ADDR_RANGES)
            {
                ShowMessages("err, no more than %u address ranges supported\n",
                             (UINT32)PT_MAX_ADDR_RANGES);
                return FALSE;
            }

            UINT64 Start = 0, End = 0;

            i++;
            if (!ConvertTokenToUInt64(CommandTokens.at(i), &Start))
            {
                ShowMessages("err, '%s' is not a valid address\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                return FALSE;
            }
            i++;
            if (!ConvertTokenToUInt64(CommandTokens.at(i), &End))
            {
                ShowMessages("err, '%s' is not a valid address\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                return FALSE;
            }

            UINT32 Idx                             = PtRequest->NumAddrRanges;
            PtRequest->AddrRanges[Idx].Start       = Start;
            PtRequest->AddrRanges[Idx].End         = End;
            PtRequest->AddrRanges[Idx].IsStopRange = IsStop;
            PtRequest->NumAddrRanges               = Idx + 1;
        }
        else
        {
            ShowMessages("unknown filter option '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
            return FALSE;
        }
    }

    //
    // Default to both modes if neither was specified — matches LBR's
    // empty-filter behaviour.
    //
    if (!AnyMode)
    {
        PtRequest->TraceUser   = 1;
        PtRequest->TraceKernel = 1;
    }

    return TRUE;
}

/**
 * @brief !pt command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandPt(vector<CommandToken> CommandTokens, string Command)
{
    HYPERTRACE_PT_OPERATION_PACKETS PtRequest = {0};

    if (CommandTokens.size() == 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());

        CommandPtHelp();
        return;
    }

    if (CompareLowerCaseStrings(CommandTokens.at(1), "enable") && CommandTokens.size() == 2)
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_ENABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "disable") && CommandTokens.size() == 2)
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "pause") && CommandTokens.size() == 2)
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_PAUSE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "resume") && CommandTokens.size() == 2)
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_RESUME;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "size") && CommandTokens.size() == 2)
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_SIZE;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "dump") && CommandTokens.size() == 2)
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DUMP;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "flush") && CommandTokens.size() == 2)
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FLUSH;
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "filter"))
    {
        PtRequest.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FILTER;

        if (!CommandPtParseFilterOptions(CommandTokens, &PtRequest))
        {
            CommandPtHelp();
            return;
        }
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandPtHelp();
        return;
    }

    //
    // Send the PT operation request
    //
    if (CommandPtSendRequest(&PtRequest))
    {
        switch (PtRequest.PtOperationType)
        {
        case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_ENABLE:
            ShowMessages("PT enabled successfully\n");
            break;
        case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE:
            ShowMessages("PT disabled successfully\n");
            break;
        case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_PAUSE:
            ShowMessages("PT trace paused\n");
            break;
        case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_RESUME:
            ShowMessages("PT trace resumed\n");
            break;
        case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_SIZE:
            ShowMessages("PT buffer bytes-written per CPU:\n");
            for (UINT32 i = 0; i < PtRequest.NumCpus; i++)
            {
                ShowMessages("  core %u : 0x%llx\n", i, PtRequest.BytesPerCpu[i]);
            }
            break;
        case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DUMP:
            ShowMessages("PT trace state is shown\n");
            break;
        case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FLUSH:
            ShowMessages("PT trace state is flushed\n");
            break;
        case HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FILTER:
            ShowMessages("PT filter / config updated successfully\n");
            break;
        default:
            ShowMessages("unknown PT operation type\n");
            break;
        }
    }
    else
    {
        ShowErrorMessage(PtRequest.KernelStatus);
        return;
    }
}
