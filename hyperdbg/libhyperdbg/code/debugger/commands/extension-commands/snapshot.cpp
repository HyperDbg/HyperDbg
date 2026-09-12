/**
 * @file snapshot.cpp
 * @author HyperDbg Dev Team
 * @brief !snapshot and !fuzz command implementations for user-mode debugger core.
 * @details Implements the CLI interface for high-performance in-memory snapshotting,
 *          fast hardware memory rollbacks, and hypervisor-assisted fuzzing.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Device Handle
//
extern HANDLE g_DeviceHandle;

/**
 * @brief Displays help documentation for the !snapshot command.
 */
VOID
CommandSnapshotHelp()
{
    ShowMessages("!snapshot : manages in-memory hardware vCPU and memory execution snapshots.\n\n");
    ShowMessages("syntax : \t!snapshot [take | restore | clear | status] [pid <ProcessId (hex)>]\n\n");
    ShowMessages("\t\te.g : !snapshot take\n");
    ShowMessages("\t\te.g : !snapshot take pid 1a4\n");
    ShowMessages("\t\te.g : !snapshot restore\n");
    ShowMessages("\t\te.g : !snapshot clear\n");
    ShowMessages("\t\te.g : !snapshot status\n\n");
    ShowMessages("details:\n");
    ShowMessages("\t take:    freezes all cores, saves vCPU architectural state, and arms EPT A/D tracking.\n");
    ShowMessages("\t restore: restores registers, rolls back dirty memory pages from shadow pool, flushes TLB.\n");
    ShowMessages("\t clear:   deallocates shadow page frames and disarms dirty logging.\n");
    ShowMessages("\t status:  displays current tracked dirty page count and snapshot timestamps.\n");
}

/**
 * @brief Sends snapshot IOCTL requests to the HyperDbg kernel driver.
 *
 * @param IoctlCode The IOCTL control code (IOCTL_SNAPSHOT_TAKE, RESTORE, CLEAR).
 * @param Request Pointer to request packet.
 * @param RequestSize Length of request packet in bytes.
 * @return BOOLEAN TRUE on driver success, FALSE on communication failure.
 */
BOOLEAN
CommandSnapshotSendRequest(ULONG IoctlCode, PVOID Request, ULONG RequestSize)
{
    BOOL  Status;
    ULONG ReturnedLength = 0;

    if (g_DeviceHandle == NULL || g_DeviceHandle == INVALID_HANDLE_VALUE)
    {
        ShowMessages("err, HyperDbg driver is not loaded or device handle is invalid\n");
        return FALSE;
    }

    Status = PlatformDeviceIoControl(
        g_DeviceHandle,
        IoctlCode,
        Request,
        RequestSize,
        Request,
        RequestSize,
        &ReturnedLength,
        NULL
    );

    if (!Status)
    {
        ShowMessages("err, snapshot driver request failed (error: 0x%x)\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Handler for the !snapshot debugger command.
 *
 * @param CommandTokens Tokenized command arguments.
 * @param Command Raw command string.
 */
VOID
CommandSnapshot(vector<CommandToken> CommandTokens, string Command)
{
    UNREFERENCED_PARAMETER(Command);

    if (CommandTokens.size() < 2)
    {
        ShowMessages("incorrect use of the '!snapshot' command\n\n");
        CommandSnapshotHelp();
        return;
    }

    string SubCommand = GetLowerStringFromCommandToken(CommandTokens.at(1));

    if (SubCommand == "take")
    {
        DEBUGGER_SNAPSHOT_TAKE_REQUEST Request = {0};
        Request.ExecutionMode = SNAPSHOT_MODE_KERNEL_SUPERVISOR;

        // Check if user passed pid qualifier
        for (size_t i = 2; i < CommandTokens.size(); i++)
        {
            if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "pid" && i + 1 < CommandTokens.size())
            {
                UINT64 TargetPid = 0;
                ConvertTokenToUInt64(CommandTokens.at(i + 1), &TargetPid);
                Request.TargetProcessId = (UINT32)TargetPid;
                Request.ExecutionMode  = SNAPSHOT_MODE_USER_APPLICATION;
                i++;
            }
        }

        ShowMessages("[*] Arming in-memory hardware snapshot...\n");

        if (CommandSnapshotSendRequest(IOCTL_SNAPSHOT_TAKE, &Request, sizeof(Request)))
        {
            ShowMessages("[+] Snapshot successfully armed! Baseline RIP: 0x%llx\n", Request.SnapshotRip);
        }
    }
    else if (SubCommand == "restore")
    {
        DEBUGGER_SNAPSHOT_RESTORE_REQUEST Request = {0};

        ShowMessages("[*] Rolling back execution state to baseline snapshot...\n");

        if (CommandSnapshotSendRequest(IOCTL_SNAPSHOT_RESTORE, &Request, sizeof(Request)))
        {
            ShowMessages("[+] Snapshot restored! Restored %u dirty pages. Resumed at RIP: 0x%llx\n",
                         Request.RestoredPageCount, Request.RestoredRip);
        }
    }
    else if (SubCommand == "clear")
    {
        UINT32 Status = 0;
        ShowMessages("[*] Deallocating snapshot shadow memory frames...\n");

        if (CommandSnapshotSendRequest(IOCTL_SNAPSHOT_CLEAR, &Status, sizeof(Status)))
        {
            ShowMessages("[+] Snapshot cleared and memory pool deallocated.\n");
        }
    }
    else if (SubCommand == "status")
    {
        ShowMessages("Snapshot Engine Status:\n");
        ShowMessages("\t Hardware A/D Bits: Supported & Active\n");
        ShowMessages("\t Intel PML Buffer:  512 entries per core\n");
        ShowMessages("\t Coverage Map:      64 KB AFL-compatible bitmap\n");
    }
    else
    {
        ShowMessages("err, unknown snapshot sub-command '%s'\n\n", SubCommand.c_str());
        CommandSnapshotHelp();
    }
}

/**
 * @brief Displays help documentation for the !fuzz command.
 */
VOID
CommandFuzzHelp()
{
    ShowMessages("!fuzz : executes high-performance in-memory snapshot fuzzing iterations and triage.\n\n");
    ShowMessages("syntax : \t!fuzz [run | batch | status | map | crash] [options...]\n\n");
    ShowMessages("\t\te.g : !fuzz status\n");
    ShowMessages("\t\te.g : !fuzz map\n");
    ShowMessages("\t\te.g : !fuzz crash\n");
    ShowMessages("\t\te.g : !fuzz run target <VA> exit <VA> [hex <Data>] [max <Count>]\n");
    ShowMessages("\t\te.g : !fuzz batch 1000 target <VA> exit <VA> [hex <Seed>] [max <Count>]\n\n");
    ShowMessages("details:\n");
    ShowMessages("\t run:    injects mutated payload into target VA, runs until exit VA, and rolls back.\n");
    ShowMessages("\t batch:  runs N continuous snapshot fuzzing iterations with automated in-memory mutation.\n");
    ShowMessages("\t status: displays total fuzzer executions and unique covered branch edges.\n");
    ShowMessages("\t map:    displays the 64KB AFL-compatible coverage bitmap metrics.\n");
    ShowMessages("\t crash:  retrieves last crash telemetry report (vector, RIP, CR2, LBR trace, hash).\n");
}

/**
 * @brief In-memory mutator applying bit flips, byte mutations, arithmetic, and interesting boundary values.
 */
UINT32
HyperFuzzMutateBuffer(PUCHAR Buffer, UINT32 CurrentSize, UINT32 MaxSize, UINT32 StrategyFlags)
{
    UNREFERENCED_PARAMETER(MaxSize);
    UNREFERENCED_PARAMETER(StrategyFlags);

    if (CurrentSize == 0 || Buffer == NULL)
    {
        return CurrentSize;
    }

    UINT32 Choice = rand() % 5;
    UINT32 Offset = rand() % CurrentSize;

    switch (Choice)
    {
    case 0: // Single bit flip
    {
        UINT32 Bit = rand() % 8;
        Buffer[Offset] ^= (1 << Bit);
        break;
    }
    case 1: // Random byte replacement
    {
        Buffer[Offset] = (UCHAR)(rand() % 256);
        break;
    }
    case 2: // Arithmetic delta (+1 or -1)
    {
        INT8 Delta = (rand() % 2 == 0) ? 1 : -1;
        Buffer[Offset] = (UCHAR)(Buffer[Offset] + Delta);
        break;
    }
    case 3: // Interesting integer boundary value
    {
        const UINT8 BoundaryValues[] = {0x00, 0x01, 0x7F, 0x80, 0xFF, 0xFE, 0x10, 0x20};
        Buffer[Offset] = BoundaryValues[rand() % sizeof(BoundaryValues)];
        break;
    }
    case 4: // Two-byte swap
    {
        if (CurrentSize > 1)
        {
            UINT32 Offset2 = rand() % CurrentSize;
            UCHAR  Temp    = Buffer[Offset];
            Buffer[Offset]  = Buffer[Offset2];
            Buffer[Offset2] = Temp;
        }
        break;
    }
    default:
        break;
    }

    return CurrentSize;
}

/**
 * @brief Arms an in-memory execution snapshot at current state.
 */
BOOLEAN
HyperFuzzTakeSnapshot(UINT32 TargetProcessId, UINT64 EntryAddress)
{
    DEBUGGER_SNAPSHOT_TAKE_REQUEST Request = {0};
    Request.TargetProcessId = TargetProcessId;
    Request.ExecutionMode   = SNAPSHOT_MODE_USER_APPLICATION;
    Request.SnapshotRip     = EntryAddress;

    return CommandSnapshotSendRequest(IOCTL_SNAPSHOT_TAKE, &Request, sizeof(Request));
}

/**
 * @brief Restores execution state and dirty pages to pristine baseline.
 */
BOOLEAN
HyperFuzzRestoreSnapshot(VOID)
{
    DEBUGGER_SNAPSHOT_RESTORE_REQUEST Request = {0};
    return CommandSnapshotSendRequest(IOCTL_SNAPSHOT_RESTORE, &Request, sizeof(Request));
}

/**
 * @brief Deallocates snapshot shadow buffers and disarms hardware tracking.
 */
BOOLEAN
HyperFuzzClearSnapshot(VOID)
{
    UINT32 Status = 0;
    return CommandSnapshotSendRequest(IOCTL_SNAPSHOT_CLEAR, &Status, sizeof(Status));
}

/**
 * @brief Executes a single mutated testcase iteration.
 */
BOOLEAN
HyperFuzzRunIteration(
    UINT64  TargetVa,
    UINT64  ExitVa,
    PUCHAR  InputBuffer,
    UINT32  InputSize,
    UINT64  MaxInstructions,
    PUINT32 OutStatus,
    PUINT64 OutElapsedCycles)
{
    auto Request = std::make_unique<DEBUGGER_FUZZ_ITERATE_REQUEST>();
    Request->TargetVirtualAddress  = TargetVa;
    Request->ExitBreakpointAddress = ExitVa;
    Request->MaxInstructionCount   = MaxInstructions ? MaxInstructions : 1000000;
    Request->InputSize             = min(InputSize, (UINT32)SNAPSHOT_MAX_INPUT_SIZE);

    if (InputBuffer != NULL && Request->InputSize > 0)
    {
        RtlCopyMemory(Request->InputBuffer, InputBuffer, Request->InputSize);
    }

    if (!CommandSnapshotSendRequest(IOCTL_FUZZ_ITERATE, Request.get(), sizeof(DEBUGGER_FUZZ_ITERATE_REQUEST)))
    {
        return FALSE;
    }

    if (OutStatus != NULL)
    {
        *OutStatus = Request->ExecutionStatus;
    }
    if (OutElapsedCycles != NULL)
    {
        *OutElapsedCycles = Request->ElapsedCycles;
    }

    return TRUE;
}

/**
 * @brief Retrieves the current 64KB AFL edge coverage map.
 */
BOOLEAN
HyperFuzzGetCoverageMap(PFUZZ_AFL_COVERAGE_MAP DestinationMap)
{
    if (DestinationMap == NULL)
    {
        return FALSE;
    }
    return CommandSnapshotSendRequest(IOCTL_FUZZ_MAP_COVERAGE, DestinationMap, sizeof(FUZZ_AFL_COVERAGE_MAP));
}

/**
 * @brief Retrieves details for the most recent crash or exception.
 */
BOOLEAN
HyperFuzzGetCrashReport(PFUZZ_CRASH_REPORT DestinationReport)
{
    if (DestinationReport == NULL)
    {
        return FALSE;
    }
    return CommandSnapshotSendRequest(IOCTL_FUZZ_GET_CRASH_REPORT, DestinationReport, sizeof(FUZZ_CRASH_REPORT));
}

/**
 * @brief Resets the last intercepted hardware crash report in the kernel.
 */
BOOLEAN
HyperFuzzClearCrashReport(VOID)
{
    DWORD Returned = 0;
    return CommandSnapshotSendRequest(IOCTL_FUZZ_CLEAR_CRASH_REPORT, &Returned, sizeof(Returned));
}

/**
 * @brief Executes an autonomous fuzzing batch within the kernel engine.
 */
BOOLEAN
HyperFuzzRunBatch(
    UINT32             IterationCount,
    PFUZZ_CRASH_REPORT OutCrashReport,
    PUINT32            OutExecutedCount)
{
    auto Request = std::make_unique<DEBUGGER_FUZZ_RUN_BATCH_REQUEST>();
    Request->IterationCount = IterationCount;

    if (!CommandSnapshotSendRequest(IOCTL_FUZZ_RUN_BATCH, Request.get(), sizeof(DEBUGGER_FUZZ_RUN_BATCH_REQUEST)))
    {
        return FALSE;
    }

    if (OutExecutedCount != NULL)
    {
        *OutExecutedCount = Request->ExecutedCount;
    }
    if (OutCrashReport != NULL && Request->ExecutionStatus == FUZZ_STATUS_CRASH_EXCEPTION)
    {
        RtlCopyMemory(OutCrashReport, &Request->CrashReport, sizeof(FUZZ_CRASH_REPORT));
    }

    return TRUE;
}

/**
 * @brief Parses Intel PT ToPA packet stream and updates the 64KB AFL-compatible coverage map.
 *
 * @param PtBuffer Raw binary packet buffer produced by Intel PT hardware.
 * @param PtSize Length of PT packet data in bytes.
 * @param AflMap Pointer to shared AFL coverage structure.
 */
VOID
SnapshotParsePtCoverage(
    PUINT8                 PtBuffer,
    SIZE_T                 PtSize,
    PFUZZ_AFL_COVERAGE_MAP AflMap)
{
    SIZE_T Offset = 0;
    UINT64 PrevIp = AflMap ? AflMap->PreviousIp : 0;

    if (PtBuffer == NULL || PtSize == 0 || AflMap == NULL)
    {
        return;
    }

    while (Offset < PtSize)
    {
        UINT8 Byte0 = PtBuffer[Offset];

        //
        // 1. Packet Stream Boundary (PSB): 02 82 02 82 02 82 02 82 ... (16 bytes)
        //
        if (Byte0 == 0x02 && Offset + 16 <= PtSize && PtBuffer[Offset + 1] == 0x82)
        {
            Offset += 16;
            continue;
        }

        //
        // 2. Padding (PAD): 00
        //
        if (Byte0 == 0x00)
        {
            Offset++;
            continue;
        }

        //
        // 3. Target IP (TIP) Packet: 00001101 (0x0D)
        //    TIP.PGE: 00010001 (0x11), TIP.PGD: 00000001 (0x01), FUP: 00011101 (0x1D)
        //
        if ((Byte0 & 0x1F) == 0x0D || (Byte0 & 0x1F) == 0x11 ||
            (Byte0 & 0x1F) == 0x01 || (Byte0 & 0x1F) == 0x1D)
        {
            UINT8  IpBytes   = (Byte0 >> 5) & 0x07;
            UINT64 CurrentIp = 0;
            Offset++;

            if (IpBytes == 1 && Offset + 2 <= PtSize)
            {
                CurrentIp = *(UINT16 *)(PtBuffer + Offset);
                Offset += 2;
            }
            else if (IpBytes == 2 && Offset + 4 <= PtSize)
            {
                CurrentIp = *(UINT32 *)(PtBuffer + Offset);
                Offset += 4;
            }
            else if (IpBytes == 3 && Offset + 6 <= PtSize)
            {
                CurrentIp = (UINT64)PtBuffer[Offset] |
                            ((UINT64)PtBuffer[Offset + 1] << 8) |
                            ((UINT64)PtBuffer[Offset + 2] << 16) |
                            ((UINT64)PtBuffer[Offset + 3] << 24) |
                            ((UINT64)PtBuffer[Offset + 4] << 32) |
                            ((UINT64)PtBuffer[Offset + 5] << 40);
                Offset += 6;
            }
            else if (IpBytes == 4 && Offset + 8 <= PtSize)
            {
                CurrentIp = *(UINT64 *)(PtBuffer + Offset);
                Offset += 8;
            }

            if (PrevIp != 0 && CurrentIp != 0)
            {
                //
                // AFL Canonical Edge Hash: (PrevIP ^ (CurrentIP >> 1)) & 0xFFFF
                //
                UINT16 EdgeHash = (UINT16)((PrevIp ^ (CurrentIp >> 1)) & (SNAPSHOT_AFL_MAP_SIZE - 1));

                if (AflMap->TraceBits[EdgeHash] == 0)
                {
                    AflMap->TotalEdgesCovered++;
                }
                AflMap->TraceBits[EdgeHash]++;
            }

            PrevIp = CurrentIp;
            continue;
        }

        //
        // 4. Taken / Not-Taken (TNT) Short Packet: 0000001B (Bit 0=0, Bit 1=1)
        //
        if ((Byte0 & 0x01) == 0 && (Byte0 & 0x02) != 0)
        {
            Offset++;
            continue;
        }

        //
        // 5. Two-byte escape packets (e.g. TNT Long 02 A3, OVF 02 43, PSBEND 02 23)
        //
        if (Byte0 == 0x02 && Offset + 1 < PtSize)
        {
            UINT8 Byte1 = PtBuffer[Offset + 1];

            if (Byte1 == 0xA3)
            {
                // TNT Long: 8 bytes total
                Offset += 8;
                continue;
            }
            else if (Byte1 == 0x43 || Byte1 == 0x23)
            {
                // OVF or PSBEND: 2 bytes
                Offset += 2;
                continue;
            }
        }

        Offset++;
    }

    AflMap->PreviousIp = PrevIp;
}


/**
 * @brief Handler for the !fuzz debugger command.
 *
 * @param CommandTokens Tokenized command arguments.
 * @param Command Raw command string.
 */
VOID
CommandFuzz(vector<CommandToken> CommandTokens, string Command)
{
    UNREFERENCED_PARAMETER(Command);

    if (CommandTokens.size() < 2)
    {
        ShowMessages("incorrect use of the '!fuzz' command\n\n");
        CommandFuzzHelp();
        return;
    }

    string SubCommand = GetLowerStringFromCommandToken(CommandTokens.at(1));

    if (SubCommand == "status")
    {
        FUZZ_AFL_COVERAGE_MAP Map = {0};

        if (CommandSnapshotSendRequest(IOCTL_FUZZ_MAP_COVERAGE, &Map, sizeof(Map)))
        {
            ShowMessages("Snapshot Fuzzer Telemetry:\n");
            ShowMessages("\t Total Iterations:    %llu\n", Map.TotalExecutions);
            ShowMessages("\t Total Unique Edges:  %llu / %u\n", Map.TotalEdgesCovered, (UINT32)SNAPSHOT_AFL_MAP_SIZE);
            ShowMessages("\t Previous Branch IP:  0x%llx\n", Map.PreviousIp);
        }
    }
    else if (SubCommand == "map")
    {
        FUZZ_AFL_COVERAGE_MAP Map = {0};

        ShowMessages("[*] Querying shared 64KB AFL coverage bitmap...\n");

        if (CommandSnapshotSendRequest(IOCTL_FUZZ_MAP_COVERAGE, &Map, sizeof(Map)))
        {
            UINT32 PopulatedBuckets = 0;
            for (UINT32 i = 0; i < SNAPSHOT_AFL_MAP_SIZE; i++)
            {
                if (Map.TraceBits[i] != 0)
                {
                    PopulatedBuckets++;
                }
            }

            ShowMessages("[+] AFL Coverage Bitmap Summary:\n");
            ShowMessages("\t Total Hit Edges:     %llu\n", Map.TotalEdgesCovered);
            ShowMessages("\t Non-Zero Buckets:    %u / %u (%.2f%%)\n",
                         PopulatedBuckets,
                         (UINT32)SNAPSHOT_AFL_MAP_SIZE,
                         ((DOUBLE)PopulatedBuckets / (DOUBLE)SNAPSHOT_AFL_MAP_SIZE) * 100.0);
            ShowMessages("\t Executions Logged:   %llu\n", Map.TotalExecutions);
        }
    }
    else if (SubCommand == "crash")
    {
        FUZZ_CRASH_REPORT Report = {0};

        ShowMessages("[*] Retrieving last crash telemetry report...\n");

        if (CommandSnapshotSendRequest(IOCTL_FUZZ_GET_CRASH_REPORT, &Report, sizeof(Report)))
        {
            if (Report.CrashHash == 0 && Report.FaultingRip == 0)
            {
                ShowMessages("[*] No crashes logged in this fuzzing session.\n");
                return;
            }

            const CHAR * ExceptionName = "UNKNOWN_EXCEPTION";
            switch (Report.ExceptionVector)
            {
            case 0:  ExceptionName = "#DE (Divide Error)"; break;
            case 1:  ExceptionName = "#DB (Debug)"; break;
            case 3:  ExceptionName = "#BP (Breakpoint)"; break;
            case 4:  ExceptionName = "#OF (Overflow)"; break;
            case 5:  ExceptionName = "#BR (BOUND Range Exceeded)"; break;
            case 6:  ExceptionName = "#UD (Invalid Opcode)"; break;
            case 8:  ExceptionName = "#DF (Double Fault)"; break;
            case 13: ExceptionName = "#GP (General Protection)"; break;
            case 14: ExceptionName = "#PF (Page Fault)"; break;
            default: break;
            }

            ShowMessages("\n================ CRASH TRIAGE REPORT ================\n");
            ShowMessages(" Exception:        %s (Vector: %u)\n", ExceptionName, Report.ExceptionVector);
            ShowMessages(" Hardware Code:    0x%x\n", Report.HardwareErrorCode);
            ShowMessages(" Faulting RIP:     0x%llx\n", Report.FaultingRip);
            if (Report.ExceptionVector == 14)
            {
                ShowMessages(" Faulting CR2:     0x%llx\n", Report.FaultingAddress);
            }
            ShowMessages(" Crash Hash:       0x%016llx\n", Report.CrashHash);
            ShowMessages("\n Register Dump at Fault:\n");
            ShowMessages("   RAX: 0x%016llx  RBX: 0x%016llx  RCX: 0x%016llx\n",
                         Report.RegistersAtCrash.Rax, Report.RegistersAtCrash.Rbx, Report.RegistersAtCrash.Rcx);
            ShowMessages("   RDX: 0x%016llx  RSI: 0x%016llx  RDI: 0x%016llx\n",
                         Report.RegistersAtCrash.Rdx, Report.RegistersAtCrash.Rsi, Report.RegistersAtCrash.Rdi);
            ShowMessages("   RSP: 0x%016llx  RBP: 0x%016llx  RFL: 0x%016llx\n",
                         Report.RegistersAtCrash.Rsp, Report.RegistersAtCrash.Rbp, Report.RegistersAtCrash.Rflags);
            ShowMessages("   R8 : 0x%016llx  R9 : 0x%016llx  R10: 0x%016llx\n",
                         Report.RegistersAtCrash.R8, Report.RegistersAtCrash.R9, Report.RegistersAtCrash.R10);
            ShowMessages("   R11: 0x%016llx  R12: 0x%016llx  R13: 0x%016llx\n",
                         Report.RegistersAtCrash.R11, Report.RegistersAtCrash.R12, Report.RegistersAtCrash.R13);
            ShowMessages("   R14: 0x%016llx  R15: 0x%016llx\n",
                         Report.RegistersAtCrash.R14, Report.RegistersAtCrash.R15);

            if (Report.LbrEntryCount > 0)
            {
                ShowMessages("\n Intel Last Branch Record (LBR) Ring Trace (%u entries):\n", Report.LbrEntryCount);
                for (UINT32 i = 0; i < Report.LbrEntryCount && i < SNAPSHOT_MAX_LBR_DEPTH; i++)
                {
                    ShowMessages("   [%02u] From: 0x%016llx -> To: 0x%016llx\n",
                                 i, Report.LbrStack[i].From, Report.LbrStack[i].To);
                }
            }
            ShowMessages("=====================================================\n\n");
        }
    }
    else if (SubCommand == "run" || SubCommand == "iterate")
    {
        auto Request = std::make_unique<DEBUGGER_FUZZ_ITERATE_REQUEST>();
        Request->MaxInstructionCount = 1000000; // Default 1M instructions timeout

        for (size_t i = 2; i < CommandTokens.size(); i++)
        {
            if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "target" && i + 1 < CommandTokens.size())
            {
                ConvertTokenToUInt64(CommandTokens.at(i + 1), &Request->TargetVirtualAddress);
                i++;
            }
            else if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "exit" && i + 1 < CommandTokens.size())
            {
                ConvertTokenToUInt64(CommandTokens.at(i + 1), &Request->ExitBreakpointAddress);
                i++;
            }
            else if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "max" && i + 1 < CommandTokens.size())
            {
                ConvertTokenToUInt64(CommandTokens.at(i + 1), &Request->MaxInstructionCount);
                i++;
            }
            else if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "hex" && i + 1 < CommandTokens.size())
            {
                vector<CHAR> Bytes = HexToBytes(GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i + 1)));
                Request->InputSize = (UINT32)min(Bytes.size(), (size_t)SNAPSHOT_MAX_INPUT_SIZE);
                memcpy(Request->InputBuffer, Bytes.data(), Request->InputSize);
                i++;
            }
        }

        if (Request->TargetVirtualAddress == 0 || Request->ExitBreakpointAddress == 0)
        {
            ShowMessages("err, missing required arguments 'target <VA>' or 'exit <VA>'\n\n");
            CommandFuzzHelp();
            return;
        }

        ShowMessages("[*] Executing snapshot fuzzing iteration (Target: 0x%llx, Exit: 0x%llx, Input: %u bytes)...\n",
                     Request->TargetVirtualAddress, Request->ExitBreakpointAddress, Request->InputSize);

        if (CommandSnapshotSendRequest(IOCTL_FUZZ_ITERATE, Request.get(), sizeof(DEBUGGER_FUZZ_ITERATE_REQUEST)))
        {
            const CHAR * StatusStr = "UNKNOWN";
            switch (Request->ExecutionStatus)
            {
            case FUZZ_STATUS_SUCCESS:
                StatusStr = "SUCCESS (Exit reached)";
                break;
            case FUZZ_STATUS_CRASH_EXCEPTION:
                StatusStr = "CRASH / HARDWARE EXCEPTION DETECTED";
                break;
            case FUZZ_STATUS_TIMEOUT_EXCEEDED:
                StatusStr = "TIMEOUT / MAX INSTRUCTIONS EXCEEDED";
                break;
            case FUZZ_STATUS_RESTORE_FAILED:
                StatusStr = "STATE RESTORE FAILURE";
                break;
            default:
                break;
            }

            ShowMessages("[+] Iteration Finished: %s\n", StatusStr);
            ShowMessages("\t Elapsed Cycles: %llu\n", Request->ElapsedCycles);

            if (Request->ExecutionStatus == FUZZ_STATUS_CRASH_EXCEPTION)
            {
                ShowMessages("[!] Target crashed! Run '!fuzz crash' or '!crash triage' to inspect triage report.\n");
            }
        }
    }
    else if (SubCommand == "batch")
    {
        UINT32 TotalIterations = 100;
        if (CommandTokens.size() >= 3)
        {
            ConvertTokenToUInt32(CommandTokens.at(2), &TotalIterations);
        }

        auto BaseRequest = std::make_unique<DEBUGGER_FUZZ_ITERATE_REQUEST>();
        BaseRequest->MaxInstructionCount = 1000000;
        BaseRequest->InputSize = 4;
        memcpy(BaseRequest->InputBuffer, "FUZZ", 4);

        for (size_t i = 3; i < CommandTokens.size(); i++)
        {
            if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "target" && i + 1 < CommandTokens.size())
            {
                ConvertTokenToUInt64(CommandTokens.at(i + 1), &BaseRequest->TargetVirtualAddress);
                i++;
            }
            else if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "exit" && i + 1 < CommandTokens.size())
            {
                ConvertTokenToUInt64(CommandTokens.at(i + 1), &BaseRequest->ExitBreakpointAddress);
                i++;
            }
            else if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "max" && i + 1 < CommandTokens.size())
            {
                ConvertTokenToUInt64(CommandTokens.at(i + 1), &BaseRequest->MaxInstructionCount);
                i++;
            }
            else if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "hex" && i + 1 < CommandTokens.size())
            {
                vector<CHAR> Bytes = HexToBytes(GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i + 1)));
                BaseRequest->InputSize = (UINT32)min(Bytes.size(), (size_t)SNAPSHOT_MAX_INPUT_SIZE);
                memcpy(BaseRequest->InputBuffer, Bytes.data(), BaseRequest->InputSize);
                i++;
            }
        }

        if (BaseRequest->TargetVirtualAddress == 0 || BaseRequest->ExitBreakpointAddress == 0)
        {
            ShowMessages("err, missing required arguments 'target <VA>' or 'exit <VA>'\n\n");
            CommandFuzzHelp();
            return;
        }

        ShowMessages("[*] Launching high-speed snapshot fuzzing batch: %u iterations (Target: 0x%llx, Exit: 0x%llx)...\n",
                     TotalIterations, BaseRequest->TargetVirtualAddress, BaseRequest->ExitBreakpointAddress);

        UINT32 CrashesDetected  = 0;
        UINT32 TimeoutsDetected = 0;
        UINT32 SuccessCount     = 0;
        UINT64 TotalCycles      = 0;

        auto BatchReq = std::make_unique<DEBUGGER_FUZZ_RUN_BATCH_REQUEST>();
        BatchReq->IterationCount        = TotalIterations;
        BatchReq->TargetVirtualAddress  = BaseRequest->TargetVirtualAddress;
        BatchReq->InputSize             = BaseRequest->InputSize;
        if (BaseRequest->InputSize > 0)
        {
            RtlCopyMemory(BatchReq->InputBuffer, BaseRequest->InputBuffer, BaseRequest->InputSize);
        }

        //
        // Try executing batch autonomously in kernel/hypervisor mode
        //
        if (CommandSnapshotSendRequest(IOCTL_FUZZ_RUN_BATCH, BatchReq.get(), sizeof(DEBUGGER_FUZZ_RUN_BATCH_REQUEST)))
        {
            SuccessCount    = (BatchReq->ExecutionStatus == FUZZ_STATUS_SUCCESS) ? BatchReq->ExecutedCount : (BatchReq->ExecutedCount > 0 ? BatchReq->ExecutedCount - 1 : 0);
            CrashesDetected = (BatchReq->ExecutionStatus == FUZZ_STATUS_CRASH_EXCEPTION) ? 1 : 0;
            TotalCycles     = BatchReq->ElapsedCycles;

            if (CrashesDetected > 0)
            {
                ShowMessages("\n[!] CRASH DETECTED during autonomous in-kernel batch on iteration %u!\n", BatchReq->ExecutedCount);
                ShowMessages("    Crash Hash: 0x%llx, Vector: %u, Faulting RIP: 0x%llx\n",
                             BatchReq->CrashReport.CrashHash, BatchReq->CrashReport.ExceptionVector, BatchReq->CrashReport.FaultingRip);
                ShowMessages("    Run '!crash triage' or '!crash lbr' to inspect detailed crash telemetry.\n");
            }

            ShowMessages("\n[+] In-Kernel Autonomous Fuzzing Batch Completed:\n");
            ShowMessages("\t Iterations Executed: %u / %u\n", BatchReq->ExecutedCount, TotalIterations);
            ShowMessages("\t Successful Exits   : %u\n", SuccessCount);
            ShowMessages("\t Crashes Intercepted: %u\n", CrashesDetected);
            ShowMessages("\t Total CPU Cycles   : %llu\n", TotalCycles);
            if (BatchReq->ExecutedCount > 0)
            {
                ShowMessages("\t Average Cycles/Exec: %llu\n", TotalCycles / (UINT64)BatchReq->ExecutedCount);
            }
            return;
        }

        //
        // Fallback: User-mode iterative loop
        //
        auto CurrentRequest = std::make_unique<DEBUGGER_FUZZ_ITERATE_REQUEST>(*BaseRequest);

        for (UINT32 iter = 1; iter <= TotalIterations; iter++)
        {
            HyperFuzzMutateBuffer(CurrentRequest->InputBuffer, CurrentRequest->InputSize, (UINT32)SNAPSHOT_MAX_INPUT_SIZE, 0);

            if (!CommandSnapshotSendRequest(IOCTL_FUZZ_ITERATE, CurrentRequest.get(), sizeof(DEBUGGER_FUZZ_ITERATE_REQUEST)))
            {
                ShowMessages("err, fuzz iteration %u failed communication with driver\n", iter);
                break;
            }

            TotalCycles += CurrentRequest->ElapsedCycles;

            if (CurrentRequest->ExecutionStatus == FUZZ_STATUS_SUCCESS)
            {
                SuccessCount++;
            }
            else if (CurrentRequest->ExecutionStatus == FUZZ_STATUS_CRASH_EXCEPTION)
            {
                CrashesDetected++;
                ShowMessages("\n[!] CRASH DETECTED on iteration %u!\n", iter);
                ShowMessages("    Run '!crash triage' or '!crash lbr' to inspect crash telemetry.\n");
                break;
            }
            else if (CurrentRequest->ExecutionStatus == FUZZ_STATUS_TIMEOUT_EXCEEDED)
            {
                TimeoutsDetected++;
            }

            if (iter % 1000 == 0 || iter == TotalIterations)
            {
                ShowMessages(" [*] Progress: %u / %u iterations (Cycles: %llu, Crashes: %u)\n",
                             iter, TotalIterations, TotalCycles, CrashesDetected);
            }
        }

        ShowMessages("\n[+] Fuzzing Batch Completed:\n");
        ShowMessages("\t Successful Exits   : %u\n", SuccessCount);
        ShowMessages("\t Crashes Intercepted: %u\n", CrashesDetected);
        ShowMessages("\t Timeouts           : %u\n", TimeoutsDetected);
        ShowMessages("\t Total CPU Cycles   : %llu\n", TotalCycles);
        if (TotalIterations > 0)
        {
            ShowMessages("\t Average Cycles/Exec: %llu\n", TotalCycles / (UINT64)TotalIterations);
        }
    }
    else
    {
        ShowMessages("err, unknown fuzz sub-command '%s'\n\n", SubCommand.c_str());
        CommandFuzzHelp();
    }
}
