/**
 * @file crash.cpp
 * @author HyperDbg Dev Team
 * @brief !crash command implementation for crash triage, LBR inspection, and post-mortem dumps.
 * @details Extracts hypervisor-trapped hardware exceptions (#GP, #PF, #UD), LBR callstack
 *          traces, and 64-bit crash deduplication hashes from the kernel driver.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"
#include <fstream>
#include <iomanip>

//
// Global Device Handle
//
extern HANDLE g_DeviceHandle;

/**
 * @brief Translates an x86 hardware exception vector number to human-readable mnemonic.
 */
static const CHAR *
CrashGetExceptionMnemonic(UINT32 Vector)
{
    switch (Vector)
    {
    case 0:  return "#DE (Divide-by-Zero Error)";
    case 1:  return "#DB (Debug Exception)";
    case 2:  return "#NMI (Non-Maskable Interrupt)";
    case 3:  return "#BP (Breakpoint Exception)";
    case 4:  return "#OF (Overflow Trap)";
    case 5:  return "#BR (BOUND Range Exceeded)";
    case 6:  return "#UD (Invalid Opcode / Undefined Instruction)";
    case 7:  return "#NM (Device Not Available / No Math Coprocessor)";
    case 8:  return "#DF (Double Fault)";
    case 10: return "#TS (Invalid TSS)";
    case 11: return "#NP (Segment Not Present)";
    case 12: return "#SS (Stack Fault)";
    case 13: return "#GP (General Protection Fault)";
    case 14: return "#PF (Page Fault)";
    case 16: return "#MF (x87 FPU Floating-Point Error)";
    case 17: return "#AC (Alignment Check)";
    case 18: return "#MC (Machine Check)";
    case 19: return "#XM (SIMD Floating-Point Exception)";
    case 20: return "#VE (Virtualization Exception)";
    case 21: return "#CP (Control Protection Exception)";
    default: return "#UNKNOWN (Reserved / Software Exception)";
    }
}

/**
 * @brief Queries the driver for the most recent crash report.
 */
static BOOLEAN
CrashFetchReport(PFUZZ_CRASH_REPORT Report)
{
    BOOL  Status;
    ULONG ReturnedLength = 0;

    if (g_DeviceHandle == NULL || g_DeviceHandle == INVALID_HANDLE_VALUE)
    {
        ShowMessages("err, HyperDbg driver is not loaded or device handle is invalid\n");
        return FALSE;
    }

    RtlZeroMemory(Report, sizeof(FUZZ_CRASH_REPORT));

    Status = PlatformDeviceIoControl(
        g_DeviceHandle,
        IOCTL_FUZZ_GET_CRASH_REPORT,
        Report,
        sizeof(FUZZ_CRASH_REPORT),
        Report,
        sizeof(FUZZ_CRASH_REPORT),
        &ReturnedLength,
        NULL
    );

    if (!Status)
    {
        ShowMessages("err, failed to query crash report from driver (error: 0x%x)\n", GetLastError());
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Displays help documentation for the !crash command.
 */
VOID
CommandCrashHelp()
{
    ShowMessages("!crash : inspects and serializes hypervisor-trapped hardware crashes.\n\n");
    ShowMessages("syntax : \t!crash [triage | lbr | dump | clear] [file <path>]\n\n");
    ShowMessages("\t\te.g : !crash\n");
    ShowMessages("\t\te.g : !crash triage\n");
    ShowMessages("\t\te.g : !crash lbr\n");
    ShowMessages("\t\te.g : !crash dump\n");
    ShowMessages("\t\te.g : !crash dump file C:\\crashes\\crash_01.json\n");
    ShowMessages("\t\te.g : !crash clear\n\n");
    ShowMessages("details:\n");
    ShowMessages("\t triage: displays faulting instruction, exception vector, fault address, registers, and hash.\n");
    ShowMessages("\t lbr:    dumps the 32 Last Branch Record hardware branches leading up to the fault.\n");
    ShowMessages("\t dump:   serializes crash metadata and execution state to a structured JSON artifact.\n");
    ShowMessages("\t clear:  resets the kernel crash report cache.\n");
}

/**
 * @brief Handler for the !crash debugger command.
 *
 * @param CommandTokens Tokenized command arguments.
 * @param Command Raw command string.
 */
VOID
CommandCrash(vector<CommandToken> CommandTokens, string Command)
{
    UNREFERENCED_PARAMETER(Command);

    string SubCommand = "triage";

    if (CommandTokens.size() >= 2)
    {
        SubCommand = GetLowerStringFromCommandToken(CommandTokens.at(1));
    }

    if (SubCommand == "help")
    {
        CommandCrashHelp();
        return;
    }

    FUZZ_CRASH_REPORT Report = {0};

    if (!CrashFetchReport(&Report))
    {
        return;
    }

    if (Report.FaultingRip == 0 && Report.CrashHash == 0)
    {
        ShowMessages("[*] No active crashes logged in the current hypervisor session.\n");
        return;
    }

    if (SubCommand == "triage")
    {
        ShowMessages("=====================================================\n");
        ShowMessages("            HYPERDBG CRASH TRIAGE REPORT             \n");
        ShowMessages("=====================================================\n");
        ShowMessages(" Exception Vector  : %u -> %s\n",
                     Report.ExceptionVector,
                     CrashGetExceptionMnemonic(Report.ExceptionVector));
        ShowMessages(" Hardware Error Code: 0x%08x\n", Report.HardwareErrorCode);
        ShowMessages(" Faulting RIP       : 0x%016llx\n", Report.FaultingRip);
        ShowMessages(" Faulting Address   : 0x%016llx (CR2 / Operand)\n", Report.FaultingAddress);
        ShowMessages(" Callstack Deduplication Hash: 0x%016llx\n\n", Report.CrashHash);

        ShowMessages(" Architectural Register State at Crash:\n");
        ShowMessages(" RAX: %016llx  RBX: %016llx  RCX: %016llx\n",
                     Report.RegistersAtCrash.Rax, Report.RegistersAtCrash.Rbx, Report.RegistersAtCrash.Rcx);
        ShowMessages(" RDX: %016llx  RSI: %016llx  RDI: %016llx\n",
                     Report.RegistersAtCrash.Rdx, Report.RegistersAtCrash.Rsi, Report.RegistersAtCrash.Rdi);
        ShowMessages(" RSP: %016llx  RBP: %016llx  RFL: %016llx\n",
                     Report.RegistersAtCrash.Rsp, Report.RegistersAtCrash.Rbp, Report.RegistersAtCrash.Rflags);
        ShowMessages(" R8 : %016llx  R9 : %016llx  R10: %016llx\n",
                     Report.RegistersAtCrash.R8, Report.RegistersAtCrash.R9, Report.RegistersAtCrash.R10);
        ShowMessages(" R11: %016llx  R12: %016llx  R13: %016llx\n",
                     Report.RegistersAtCrash.R11, Report.RegistersAtCrash.R12, Report.RegistersAtCrash.R13);
        ShowMessages(" R14: %016llx  R15: %016llx\n\n",
                     Report.RegistersAtCrash.R14, Report.RegistersAtCrash.R15);

        ShowMessages(" Control Registers:\n");
        ShowMessages(" CR0: %016llx  CR2: %016llx  CR3: %016llx  CR4: %016llx\n",
                     Report.RegistersAtCrash.Cr0, Report.RegistersAtCrash.Cr2,
                     Report.RegistersAtCrash.Cr3, Report.RegistersAtCrash.Cr4);

        ShowMessages(" LBR Trace Depth   : %u branches recorded (run '!crash lbr' for full ring)\n",
                     Report.LbrEntryCount);
        ShowMessages("=====================================================\n\n");
    }
    else if (SubCommand == "lbr")
    {
        ShowMessages("=====================================================\n");
        ShowMessages("           LBR HARDWARE BRANCH RING DUMP             \n");
        ShowMessages("=====================================================\n");
        ShowMessages(" Recorded Branches Leading to Crash (Most Recent First):\n\n");

        UINT32 ValidEntries = min(Report.LbrEntryCount, (UINT32)SNAPSHOT_MAX_LBR_DEPTH);

        if (ValidEntries == 0)
        {
            ShowMessages(" [*] No LBR branch history available for this crash.\n");
        }
        else
        {
            for (UINT32 i = 0; i < ValidEntries; i++)
            {
                ShowMessages(" [%02u] From: 0x%016llx  -->  To: 0x%016llx\n",
                             i,
                             Report.LbrStack[i].From,
                             Report.LbrStack[i].To);
            }
        }
        ShowMessages("=====================================================\n\n");
    }
    else if (SubCommand == "dump")
    {
        string OutputPath = "crash_report.json";

        for (size_t i = 2; i < CommandTokens.size(); i++)
        {
            if (GetLowerStringFromCommandToken(CommandTokens.at(i)) == "file" && i + 1 < CommandTokens.size())
            {
                OutputPath = GetCaseSensitiveStringFromCommandToken(CommandTokens.at(i + 1));
                i++;
            }
        }

        std::ofstream JsonFile(OutputPath);
        if (!JsonFile.is_open())
        {
            ShowMessages("err, could not open output file '%s' for writing\n", OutputPath.c_str());
            return;
        }

        JsonFile << "{\n";
        JsonFile << "  \"crash_triage\": {\n";
        JsonFile << "    \"exception_vector\": " << Report.ExceptionVector << ",\n";
        JsonFile << "    \"mnemonic\": \"" << CrashGetExceptionMnemonic(Report.ExceptionVector) << "\",\n";
        JsonFile << "    \"hardware_error_code\": " << Report.HardwareErrorCode << ",\n";
        JsonFile << "    \"faulting_rip\": \"0x" << std::hex << Report.FaultingRip << std::dec << "\",\n";
        JsonFile << "    \"faulting_address\": \"0x" << std::hex << Report.FaultingAddress << std::dec << "\",\n";
        JsonFile << "    \"crash_hash\": \"0x" << std::hex << Report.CrashHash << std::dec << "\"\n";
        JsonFile << "  },\n";
        JsonFile << "  \"registers\": {\n";
        JsonFile << "    \"rax\": \"0x" << std::hex << Report.RegistersAtCrash.Rax << "\",\n";
        JsonFile << "    \"rbx\": \"0x" << std::hex << Report.RegistersAtCrash.Rbx << "\",\n";
        JsonFile << "    \"rcx\": \"0x" << std::hex << Report.RegistersAtCrash.Rcx << "\",\n";
        JsonFile << "    \"rdx\": \"0x" << std::hex << Report.RegistersAtCrash.Rdx << "\",\n";
        JsonFile << "    \"rsi\": \"0x" << std::hex << Report.RegistersAtCrash.Rsi << "\",\n";
        JsonFile << "    \"rdi\": \"0x" << std::hex << Report.RegistersAtCrash.Rdi << "\",\n";
        JsonFile << "    \"rsp\": \"0x" << std::hex << Report.RegistersAtCrash.Rsp << "\",\n";
        JsonFile << "    \"rbp\": \"0x" << std::hex << Report.RegistersAtCrash.Rbp << "\",\n";
        JsonFile << "    \"rflags\": \"0x" << std::hex << Report.RegistersAtCrash.Rflags << "\",\n";
        JsonFile << "    \"cr2\": \"0x" << std::hex << Report.RegistersAtCrash.Cr2 << "\",\n";
        JsonFile << "    \"cr3\": \"0x" << std::hex << Report.RegistersAtCrash.Cr3 << std::dec << "\"\n";
        JsonFile << "  },\n";
        JsonFile << "  \"lbr_stack\": [\n";

        UINT32 ValidEntries = min(Report.LbrEntryCount, (UINT32)SNAPSHOT_MAX_LBR_DEPTH);
        for (UINT32 i = 0; i < ValidEntries; i++)
        {
            JsonFile << "    {\"from\": \"0x" << std::hex << Report.LbrStack[i].From
                     << "\", \"to\": \"0x" << Report.LbrStack[i].To << std::dec << "\"}";
            if (i + 1 < ValidEntries)
            {
                JsonFile << ",";
            }
            JsonFile << "\n";
        }

        JsonFile << "  ]\n";
        JsonFile << "}\n";
        JsonFile.close();

        ShowMessages("[+] Successfully serialized crash report to: %s\n", OutputPath.c_str());
    }
    else if (SubCommand == "clear")
    {
        DWORD ReturnedLength = 0;
        if (!DeviceIoControl(
                g_DeviceHandle,
                IOCTL_FUZZ_CLEAR_CRASH_REPORT,
                NULL,
                0,
                NULL,
                0,
                &ReturnedLength,
                NULL))
        {
            ShowMessages("err, failed to clear crash report in driver (error: 0x%x)\n", GetLastError());
            return;
        }

        ShowMessages("[+] Kernel crash report cache cleared successfully.\n");
    }
    else
    {
        ShowMessages("err, unknown crash sub-command '%s'\n\n", SubCommand.c_str());
        CommandCrashHelp();
    }
}

