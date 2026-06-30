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
    ShowMessages("!pt : enables, disables and configures Intel Processor Trace (PT).\n\n");

    ShowMessages("syntax : \t!pt enable\n");
    ShowMessages("syntax : \t!pt enable [size BufferSize (hex)]\n");
    ShowMessages("syntax : \t!pt enable [pid ProcessId (hex)] [size BufferSize (hex)]\n");
    ShowMessages("syntax : \t!pt enable [tid ThreadId (hex)] [size BufferSize (hex)]\n");
    ShowMessages("syntax : \t!pt enable [pname ProcessName (string)] [size BufferSize (hex)]\n");
    ShowMessages("syntax : \t!pt enable [cr3 Cr3Value (hex)] [size BufferSize (hex)]\n");
    ShowMessages("syntax : \t!pt disable\n");
    ShowMessages("syntax : \t!pt pause\n");
    ShowMessages("syntax : \t!pt resume\n");
    ShowMessages("syntax : \t!pt flush\n");
    ShowMessages("syntax : \t!pt dump print [type TypeOfDump (string)]\n");
    ShowMessages("syntax : \t!pt dump path [type TypeOfDump (string)]\n");
    ShowMessages("syntax : \t!pt filter [Mode (string)]\n");
    ShowMessages("syntax : \t!pt filter [range1 FromAddress (hex) ToAddress (hex)] [range2 FromAddress (hex) ToAddress (hex)] [range3 FromAddress (hex) ToAddress (hex)] [range4 FromAddress (hex) ToAddress (hex)]\n");
    ShowMessages("syntax : \t!pt filter [range1 module ModuleName (string)] [range2 module ModuleName (string)] [range3 module ModuleName (string)] [range4 module ModuleName (string)]\n");
    ShowMessages("syntax : \t!pt filter [stoprange1 FromAddress (hex) ToAddress (hex)] [stoprange2 FromAddress (hex) ToAddress (hex)] [stoprange3 FromAddress (hex) ToAddress (hex)] [stoprange4 FromAddress (hex) ToAddress (hex)]\n");
    ShowMessages("syntax : \t!pt filter [stoprange1 module ModuleName (string)] [stoprange2 module ModuleName (string)] [stoprange3 module ModuleName (string)] [stoprange4 module ModuleName (string)]\n");
    ShowMessages("syntax : \t!pt packet [PacketType (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : !pt enable\n");
    ShowMessages("\t\te.g : !pt enable size 0x200000\n");
    ShowMessages("\t\te.g : !pt enable pid 0x4a8\n");
    ShowMessages("\t\te.g : !pt enable pname notepad.exe\n");
    ShowMessages("\t\te.g : !pt enable tid 0x1234\n");
    ShowMessages("\t\te.g : !pt enable cr3 0x1aabb000\n");
    ShowMessages("\t\te.g : !pt enable pid 0x4a8 size 0x200000\n");
    ShowMessages("\t\te.g : !pt disable\n");
    ShowMessages("\t\te.g : !pt pause\n");
    ShowMessages("\t\te.g : !pt resume\n");
    ShowMessages("\t\te.g : !pt flush\n");
    ShowMessages("\t\te.g : !pt dump print type instruction\n");
    ShowMessages("\t\te.g : !pt dump print type packet\n");
    ShowMessages("\t\te.g : !pt dump path C:\\trace.txt type instruction\n");
    ShowMessages("\t\te.g : !pt filter user\n");
    ShowMessages("\t\te.g : !pt filter user kernel\n");
    ShowMessages("\t\te.g : !pt filter range1 0x140001000 0x140002000\n");
    ShowMessages("\t\te.g : !pt filter range1 module main\n");
    ShowMessages("\t\te.g : !pt filter range1 module ntdll range2 module nt\n");
    ShowMessages("\t\te.g : !pt filter stoprange1 0x140003000 0x140004000\n");
    ShowMessages("\t\te.g : !pt packet psb pip tsc\n");

    ShowMessages("\n");
    ShowMessages("Where:\n");
    ShowMessages("\t[Mode (string)] could be 'kernel' or/and 'user'\n");
    ShowMessages("\t[TypeOfDump (string)] could be 'instruction' or 'packet'\n");
    ShowMessages("\t[ModuleName (string)] could be 'main' (the main module of the process), 'nt', 'win32k', or any other module name\n");
    ShowMessages("\t[PacketType (string)] could be either or a combination of 'psb', 'pip', 'tsc', 'mtc', 'cyc', 'tnt', 'tip', 'fup', or 'mode'\n");
    ShowMessages("\n");
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
 * @brief Parse options for pt disable command
 *
 * @param CommandTokens The command tokens to parse
 * @param PtRequest The PT request structure to fill with parsed options
 *
 * @return VOID
 */
static VOID
CommandPtParseDisable(vector<CommandToken> & CommandTokens, HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    ShowMessages("PT disable requested\n");

    //
    // Set the PtRequest structure for disable operation
    //
    PtRequest->PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE;
}

/**
 * @brief Parse options for pt disable command
 *
 * @param CommandTokens The command tokens to parse
 * @param PtRequest The PT request structure to fill with parsed options
 *
 * @return VOID
 */
static VOID
CommandPtParsePause(vector<CommandToken> & CommandTokens, HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    ShowMessages("PT pause requested\n");

    //
    // Set the PtRequest structure for pause operation
    //
    PtRequest->PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_PAUSE;
}

/**
 * @brief Parse options for pt disable command
 *
 * @param CommandTokens The command tokens to parse
 * @param PtRequest The PT request structure to fill with parsed options
 *
 * @return VOID
 */
static VOID
CommandPtParseResume(vector<CommandToken> & CommandTokens, HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    ShowMessages("PT resume requested\n");

    //
    // Set the PtRequest structure for resume operation
    //
    PtRequest->PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_RESUME;
}

/**
 * @brief Parse options for pt disable command
 *
 * @param CommandTokens The command tokens to parse
 * @param PtRequest The PT request structure to fill with parsed options
 *
 * @return VOID
 */
static VOID
CommandPtParseFlush(vector<CommandToken> & CommandTokens, HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    ShowMessages("PT flush requested\n");

    //
    // Set the PtRequest structure for flush operation
    //
    PtRequest->PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FLUSH;
}

/**
 * @brief Parse and display enable options for !pt enable command
 *
 * @param CommandTokens The command tokens to parse
 * @param PtRequest The PT request structure to fill with parsed options
 *
 * @return VOID
 */
static VOID
CommandPtParseEnable(vector<CommandToken> & CommandTokens, HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    BOOLEAN HasPid   = FALSE;
    BOOLEAN HasPname = FALSE;
    BOOLEAN HasTid   = FALSE;
    BOOLEAN HasCr3   = FALSE;
    BOOLEAN HasSize  = FALSE;
    UINT64  Pid      = 0;
    UINT64  Tid      = 0;
    UINT64  Cr3      = 0;
    UINT64  Size     = 0;
    string  Pname;

    for (size_t i = 2; i < CommandTokens.size(); i++)
    {
        if (CompareLowerCaseStrings(CommandTokens.at(i), "pid"))
        {
            if (i + 1 >= CommandTokens.size())
            {
                ShowMessages("err, 'pid' expects a hex process ID\n\n");
                CommandPtHelp();
                return;
            }
            i++;
            if (!ConvertTokenToUInt64(CommandTokens.at(i), &Pid))
            {
                ShowMessages("err, '%s' is not a valid hex process ID\n\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                CommandPtHelp();
                return;
            }
            HasPid = TRUE;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "pname"))
        {
            if (i + 1 >= CommandTokens.size())
            {
                ShowMessages("err, 'pname' expects a process name\n\n");
                CommandPtHelp();
                return;
            }
            i++;
            Pname    = GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i));
            HasPname = TRUE;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "tid"))
        {
            if (i + 1 >= CommandTokens.size())
            {
                ShowMessages("err, 'tid' expects a hex thread ID\n\n");
                CommandPtHelp();
                return;
            }
            i++;
            if (!ConvertTokenToUInt64(CommandTokens.at(i), &Tid))
            {
                ShowMessages("err, '%s' is not a valid hex thread ID\n\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                CommandPtHelp();
                return;
            }
            HasTid = TRUE;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "cr3"))
        {
            if (i + 1 >= CommandTokens.size())
            {
                ShowMessages("err, 'cr3' expects a hex CR3 value\n\n");
                CommandPtHelp();
                return;
            }
            i++;
            if (!ConvertTokenToUInt64(CommandTokens.at(i), &Cr3))
            {
                ShowMessages("err, '%s' is not a valid hex CR3 value\n\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                CommandPtHelp();
                return;
            }
            HasCr3 = TRUE;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "size"))
        {
            if (i + 1 >= CommandTokens.size())
            {
                ShowMessages("err, 'size' expects a hex buffer size\n\n");
                CommandPtHelp();
                return;
            }
            i++;
            if (!ConvertTokenToUInt64(CommandTokens.at(i), &Size))
            {
                ShowMessages("err, '%s' is not a valid hex size\n\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                CommandPtHelp();
                return;
            }
            HasSize = TRUE;
        }
        else
        {
            ShowMessages("err, unknown 'enable' option '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
            CommandPtHelp();
            return;
        }
    }

    //
    // pid, pname, tid, cr3 are mutually exclusive target selectors
    //
    INT32 SelectorCount = (HasPid ? 1 : 0) + (HasPname ? 1 : 0) + (HasTid ? 1 : 0) + (HasCr3 ? 1 : 0);
    if (SelectorCount > 1)
    {
        ShowMessages("err, only one of 'pid', 'pname', 'tid', 'cr3' may be specified at a time\n\n");
        CommandPtHelp();
        return;
    }

    //
    // Show parsed enable options
    //
    ShowMessages("PT enable:\n");

    if (HasPid)
    {
        ShowMessages("  target pid   : 0x%llx\n", Pid);

        PtRequest->EnableOptions.EnableByPid = 1;
        PtRequest->EnableOptions.Pid         = (UINT32)Pid;
    }
    else if (HasPname)
    {
        ShowMessages("  target pname : %s\n", Pname.c_str());

        PtRequest->EnableOptions.EnableByCr3 = 1;
        strcpy_s(PtRequest->EnableOptions.ProcessName, sizeof(PtRequest->EnableOptions.ProcessName), Pname.c_str());
    }
    else if (HasTid)
    {
        ShowMessages("  target tid   : 0x%llx\n", Tid);

        PtRequest->EnableOptions.EnableByTid = 1;
        PtRequest->EnableOptions.Tid         = (UINT32)Tid;
    }
    else if (HasCr3)
    {
        ShowMessages("  target cr3   : 0x%llx\n", Cr3);

        PtRequest->EnableOptions.EnableByCr3 = 1;
        PtRequest->EnableOptions.Cr3         = Cr3;
    }
    else
    {
        ShowMessages("  target       : all (no process/thread filter)\n");
    }

    if (HasSize)
    {
        ShowMessages("  buffer size  : 0x%llx bytes\n", Size);

        PtRequest->BufferSize = Size;
    }
    else
    {
        ShowMessages("  buffer size  : default (%llx bytes)\n", PT_DEFAULT_BUFFER_SIZE);

        PtRequest->BufferSize = PT_DEFAULT_BUFFER_SIZE;
    }

    //
    // Fill the PtRequest structure with parsed options
    //
    PtRequest->PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_ENABLE;
}

/**
 * @brief Parse and display !pt dump parameters
 *
 * @param CommandTokens The command tokens to parse
 * @param PtRequest The PT request structure to fill with parsed options
 *
 * @return VOID
 */
static VOID
CommandPtParseDump(vector<CommandToken> & CommandTokens, HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    if (CommandTokens.size() < 3)
    {
        ShowMessages("err, 'dump' requires additional options\n\n");
        CommandPtHelp();
        return;
    }

    if (CompareLowerCaseStrings(CommandTokens.at(2), "print"))
    {
        //
        // !pt dump print type <instruction|packet>
        //
        if (CommandTokens.size() != 5 || !CompareLowerCaseStrings(CommandTokens.at(3), "type"))
        {
            ShowMessages("err, syntax: !pt dump print type <instruction|packet>\n\n");
            CommandPtHelp();
            return;
        }

        if (!CompareLowerCaseStrings(CommandTokens.at(4), "instruction") &&
            !CompareLowerCaseStrings(CommandTokens.at(4), "packet"))
        {
            ShowMessages("err, dump type must be 'instruction' or 'packet', got '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(4)).c_str());
            CommandPtHelp();
            return;
        }

        ShowMessages("PT dump to console:\n");
        ShowMessages("  output : console (print)\n");
        ShowMessages("  format : %s\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(4)).c_str());
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(2), "path"))
    {
        //
        // !pt dump path <path-string> type <instruction|packet>
        //
        if (CommandTokens.size() != 6 || !CompareLowerCaseStrings(CommandTokens.at(4), "type"))
        {
            ShowMessages("err, syntax: !pt dump path <path> type <instruction|packet>\n\n");
            CommandPtHelp();
            return;
        }

        if (!CompareLowerCaseStrings(CommandTokens.at(5), "instruction") &&
            !CompareLowerCaseStrings(CommandTokens.at(5), "packet"))
        {
            ShowMessages("err, dump type must be 'instruction' or 'packet', got '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(5)).c_str());
            CommandPtHelp();
            return;
        }

        ShowMessages("PT dump to file:\n");
        ShowMessages("  output : file\n");
        ShowMessages("  path   : %s\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(3)).c_str());
        ShowMessages("  format : %s\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(5)).c_str());
    }
    else
    {
        ShowMessages("err, unknown 'dump' sub-option '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)).c_str());
        CommandPtHelp();
    }
}

/**
 * @brief Parse and display !pt filter parameters
 *
 * @param CommandTokens The command tokens to parse
 * @param PtRequest The PT request structure to fill with parsed options
 *
 * @return VOID
 */
static VOID
CommandPtParseFilter(vector<CommandToken> & CommandTokens, HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    struct PtRangeEntry
    {
        BOOLEAN Active;
        BOOLEAN IsModule;
        UINT64  Start;
        UINT64  End;
        string  ModuleName;
    };

    PtRangeEntry Ranges[4]     = {};
    PtRangeEntry StopRanges[4] = {};
    BOOLEAN      TraceUser     = FALSE;
    BOOLEAN      TraceKernel   = FALSE;

    for (size_t i = 2; i < CommandTokens.size(); i++)
    {
        if (CompareLowerCaseStrings(CommandTokens.at(i), "user"))
        {
            TraceUser = TRUE;
        }
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "kernel"))
        {
            TraceKernel = TRUE;
        }
        else
        {
            //
            // Resolve range1..range4 / stoprange1..stoprange4
            //
            BOOLEAN IsStop   = FALSE;
            INT32   RangeIdx = -1;

            if (CompareLowerCaseStrings(CommandTokens.at(i), "range1"))
            {
                IsStop   = FALSE;
                RangeIdx = 0;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "range2"))
            {
                IsStop   = FALSE;
                RangeIdx = 1;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "range3"))
            {
                IsStop   = FALSE;
                RangeIdx = 2;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "range4"))
            {
                IsStop   = FALSE;
                RangeIdx = 3;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "stoprange1"))
            {
                IsStop   = TRUE;
                RangeIdx = 0;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "stoprange2"))
            {
                IsStop   = TRUE;
                RangeIdx = 1;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "stoprange3"))
            {
                IsStop   = TRUE;
                RangeIdx = 2;
            }
            else if (CompareLowerCaseStrings(CommandTokens.at(i), "stoprange4"))
            {
                IsStop   = TRUE;
                RangeIdx = 3;
            }
            else
            {
                ShowMessages("err, unknown filter option '%s'\n\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                CommandPtHelp();
                return;
            }

            PtRangeEntry * Entry = IsStop ? &StopRanges[RangeIdx] : &Ranges[RangeIdx];

            if (i + 1 >= CommandTokens.size())
            {
                ShowMessages("err, '%s' expects <start> <end> or 'module <name>'\n\n",
                             GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                CommandPtHelp();
                return;
            }

            if (CompareLowerCaseStrings(CommandTokens.at(i + 1), "module"))
            {
                //
                // range<N> module <module-name>
                //
                if (i + 2 >= CommandTokens.size())
                {
                    ShowMessages("err, 'module' expects a module name\n\n");
                    CommandPtHelp();
                    return;
                }
                i += 2;
                Entry->IsModule   = TRUE;
                Entry->ModuleName = GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i));
                Entry->Active     = TRUE;
            }
            else
            {
                //
                // range<N> <start> <end>
                //
                if (i + 2 >= CommandTokens.size())
                {
                    ShowMessages("err, '%s' expects <start> <end>\n\n",
                                 GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                    CommandPtHelp();
                    return;
                }
                i++;
                if (!ConvertTokenToUInt64(CommandTokens.at(i), &Entry->Start))
                {
                    ShowMessages("err, '%s' is not a valid address\n\n",
                                 GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                    CommandPtHelp();
                    return;
                }
                i++;
                if (!ConvertTokenToUInt64(CommandTokens.at(i), &Entry->End))
                {
                    ShowMessages("err, '%s' is not a valid address\n\n",
                                 GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
                    CommandPtHelp();
                    return;
                }
                Entry->IsModule = FALSE;
                Entry->Active   = TRUE;
            }
        }
    }

    //
    // Show parsed filter options
    //
    ShowMessages("PT filter:\n");

    if (TraceUser)
    {
        ShowMessages("  privilege  : user (CPL > 0)\n");

        PtRequest->FilterOptions.TraceUser = 1;
    }
    if (TraceKernel)
    {
        ShowMessages("  privilege  : kernel (CPL == 0)\n");

        PtRequest->FilterOptions.TraceKernel = 1;
    }
    if (!TraceUser && !TraceKernel)
    {
        ShowMessages("  privilege  : (default - user + kernel)\n");

        PtRequest->FilterOptions.TraceUser   = 1;
        PtRequest->FilterOptions.TraceKernel = 1;
    }

    for (INT32 r = 0; r < 4; r++)
    {
        if (Ranges[r].Active)
        {
            if (Ranges[r].IsModule)
                ShowMessages("  range%d     : module '%s'\n", r + 1, Ranges[r].ModuleName.c_str());
            else
                ShowMessages("  range%d     : 0x%llx - 0x%llx\n", r + 1, Ranges[r].Start, Ranges[r].End);
        }

        //
        // Set the PtRequest structure for filter operation
        //
        PtRequest->FilterOptions.AddrRanges[r].IsStopRange = FALSE;

        PtRequest->FilterOptions.AddrRanges[r].Start = Ranges[r].Start;
        PtRequest->FilterOptions.AddrRanges[r].End   = Ranges[r].End;
    }

    for (INT32 r = 0; r < 4; r++)
    {
        if (StopRanges[r].Active)
        {
            if (StopRanges[r].IsModule)
                ShowMessages("  stoprange%d : module '%s'\n", r + 1, StopRanges[r].ModuleName.c_str());
            else
                ShowMessages("  stoprange%d : 0x%llx - 0x%llx\n", r + 1, StopRanges[r].Start, StopRanges[r].End);
        }

        //
        // Set the PtRequest structure for filter operation
        //
        PtRequest->FilterOptions.AddrRanges[r].IsStopRange = TRUE;

        PtRequest->FilterOptions.AddrRanges[r].Start = Ranges[r].Start;
        PtRequest->FilterOptions.AddrRanges[r].End   = Ranges[r].End;
    }

    //
    // Set the PtRequest structure for filter operation
    //
    PtRequest->PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FILTER;
}

/**
 * @brief Parse and display !pt packet parameters
 * @param CommandTokens The command tokens to parse
 * @param PtRequest The PT request structure to fill with parsed options
 *
 * @return VOID
 */
static VOID
CommandPtParsePacket(vector<CommandToken> & CommandTokens, HYPERTRACE_PT_OPERATION_PACKETS * PtRequest)
{
    if (CommandTokens.size() < 3)
    {
        ShowMessages("err, 'packet' requires at least one option\n\n");
        CommandPtHelp();
        return;
    }

    BOOLEAN PktPsb  = FALSE;
    BOOLEAN PktPip  = FALSE;
    BOOLEAN PktTsc  = FALSE;
    BOOLEAN PktMtc  = FALSE;
    BOOLEAN PktCyc  = FALSE;
    BOOLEAN PktTnt  = FALSE;
    BOOLEAN PktTip  = FALSE;
    BOOLEAN PktFup  = FALSE;
    BOOLEAN PktMode = FALSE;

    for (size_t i = 2; i < CommandTokens.size(); i++)
    {
        if (CompareLowerCaseStrings(CommandTokens.at(i), "psb"))
            PktPsb = TRUE;
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "pip"))
            PktPip = TRUE;
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "tsc"))
            PktTsc = TRUE;
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "mtc"))
            PktMtc = TRUE;
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "cyc"))
            PktCyc = TRUE;
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "tnt"))
            PktTnt = TRUE;
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "tip"))
            PktTip = TRUE;
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "fup"))
            PktFup = TRUE;
        else if (CompareLowerCaseStrings(CommandTokens.at(i), "mode"))
            PktMode = TRUE;
        else
        {
            ShowMessages("err, unknown 'packet' option '%s'\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i)).c_str());
            CommandPtHelp();
            return;
        }
    }

    //
    // Show parsed packet options
    //
    ShowMessages("PT packet filter:\n");

    if (PktPsb)
    {
        ShowMessages("  packet type: PSB  (Packet Stream Boundary)\n");

        PtRequest->PacketOptions.PSB = 1;
    }
    if (PktPip)
    {
        ShowMessages("  packet type: PIP  (Paging Information Packet)\n");

        PtRequest->PacketOptions.PIP = 1;
    }
    if (PktTsc)
    {
        ShowMessages("  packet type: TSC  (Timestamp Counter)\n");

        PtRequest->PacketOptions.TSC = 1;
    }
    if (PktMtc)
    {
        ShowMessages("  packet type: MTC  (Mini Timestamp Counter)\n");

        PtRequest->PacketOptions.MTC = 1;
    }
    if (PktCyc)
    {
        ShowMessages("  packet type: CYC  (Cycle Counter)\n");

        PtRequest->PacketOptions.CYC = 1;
    }
    if (PktTnt)
    {
        ShowMessages("  packet type: TNT  (Taken/Not-Taken)\n");

        PtRequest->PacketOptions.TNT = 1;
    }
    if (PktTip)
    {
        ShowMessages("  packet type: TIP  (Target IP)\n");

        PtRequest->PacketOptions.TNT = 1;
    }
    if (PktFup)
    {
        ShowMessages("  packet type: FUP  (Flow Update Packet)\n");

        PtRequest->PacketOptions.FUP = 1;
    }
    if (PktMode)
    {
        ShowMessages("  packet type: MODE (Mode packet)\n");

        PtRequest->PacketOptions.MODE = 1;
    }

    //
    // Set the PtRequest structure for packet operation
    //
    PtRequest->PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_PACKET;
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
    HYPERTRACE_PT_OPERATION_PACKETS PtRequest = {};

    if (CommandTokens.size() == 1)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandPtHelp();
        return;
    }

    //
    // Parse subcommands
    //
    if (CompareLowerCaseStrings(CommandTokens.at(1), "enable"))
    {
        //
        // Parse and display enable options for !pt enable command
        //
        CommandPtParseEnable(CommandTokens, &PtRequest);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "disable"))
    {
        if (CommandTokens.size() != 2)
        {
            ShowMessages("err, 'disable' takes no arguments\n\n");
            CommandPtHelp();
            return;
        }

        //
        // Parse and display disable options for !pt disable command
        //
        CommandPtParseDisable(CommandTokens, &PtRequest);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "pause"))
    {
        if (CommandTokens.size() != 2)
        {
            ShowMessages("err, 'pause' takes no arguments\n\n");
            CommandPtHelp();
            return;
        }

        //
        // Parse and display pause options for !pt pause command
        //
        CommandPtParsePause(CommandTokens, &PtRequest);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "resume"))
    {
        if (CommandTokens.size() != 2)
        {
            ShowMessages("err, 'resume' takes no arguments\n\n");
            CommandPtHelp();
            return;
        }

        //
        // Parse and display resume options for !pt resume command
        //
        CommandPtParseResume(CommandTokens, &PtRequest);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "flush"))
    {
        if (CommandTokens.size() != 2)
        {
            ShowMessages("err, 'flush' takes no arguments\n\n");
            CommandPtHelp();
            return;
        }

        //
        // Parse and display flush options for !pt flush command
        //
        CommandPtParseFlush(CommandTokens, &PtRequest);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "dump"))
    {
        //
        // Parse and display dump options for !pt dump command
        //
        CommandPtParseDump(CommandTokens, &PtRequest);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "filter"))
    {
        //
        // Parse and display filter options for !pt filter command
        //
        CommandPtParseFilter(CommandTokens, &PtRequest);
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "packet"))
    {
        //
        // Parse and display packet options for !pt packet command
        //
        CommandPtParsePacket(CommandTokens, &PtRequest);
    }
    else
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandPtHelp();
    }

    //
    // Send the PT request to the debugger
    //
    CommandPtSendRequest(&PtRequest);
}
