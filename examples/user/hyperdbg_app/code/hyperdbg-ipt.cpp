#include "pch.h"

#include <string.h>
#include <stdlib.h>
#include <dbghelp.h>
#include "../dependencies/libipt/intel-pt.h"
#include <Zydis/Zydis.h>

#pragma comment(lib, "dbghelp.lib")

static UINT64  g_ImageBase = 0;
static UINT64  g_CodeBase  = 0;
static UINT64  g_CodeSize  = 0;
static UINT8 * g_Code      = NULL;

static int
ShowMessages(const char * Text)
{
	printf("%s", Text);
	return 0;
}

static int
ReadImage(uint8_t * Buffer, size_t Size, const struct pt_asid * Asid, uint64_t Ip, void * Context)
{
    (void)Asid;
    (void)Context;

    if (g_Code == NULL || Ip < g_CodeBase || Ip >= g_CodeBase + g_CodeSize)
        return -pte_nomap;

    uint64_t Available = g_CodeBase + g_CodeSize - Ip;
    size_t   Count     = (Size < Available) ? Size : (size_t)Available;

    memcpy(Buffer, g_Code + (Ip - g_CodeBase), Count);
    return (int)Count;
}

typedef struct _PROC_BASIC_INFO
{
    LONG      ExitStatus;
    PVOID     PebBaseAddress;
    ULONG_PTR Reserved[4];
} PROC_BASIC_INFO;

typedef LONG(NTAPI * PFN_NT_QIP)(HANDLE, ULONG, PVOID, ULONG, PULONG);

static BOOLEAN
CaptureImage(HANDLE Process, UINT64 * TextStart, UINT64 * TextEnd)
{
    HMODULE            Ntdll = GetModuleHandleA("ntdll.dll");
    PFN_NT_QIP         NtQip = Ntdll ? (PFN_NT_QIP)GetProcAddress(Ntdll, "NtQueryInformationProcess") : NULL;
    PROC_BASIC_INFO    Pbi   = {0};
    ULONG              Ret   = 0;
    SIZE_T             Got   = 0;
    UINT64             Base  = 0;
    IMAGE_DOS_HEADER   Dos;
    IMAGE_NT_HEADERS64 Nt;
    UINT64             SectionBase;

    if (NtQip == NULL || NtQip(Process, 0, &Pbi, sizeof(Pbi), &Ret) < 0 || Pbi.PebBaseAddress == NULL)
        return FALSE;

    if (!ReadProcessMemory(Process, (PBYTE)Pbi.PebBaseAddress + 0x10, &Base, sizeof(Base), &Got) || Base == 0)
        return FALSE;

    if (!ReadProcessMemory(Process, (PVOID)Base, &Dos, sizeof(Dos), &Got) || Dos.e_magic != IMAGE_DOS_SIGNATURE)
        return FALSE;

    if (!ReadProcessMemory(Process, (PBYTE)Base + Dos.e_lfanew, &Nt, sizeof(Nt), &Got) || Nt.Signature != IMAGE_NT_SIGNATURE)
        return FALSE;

    g_ImageBase = Base;
    SectionBase = Base + Dos.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader) + Nt.FileHeader.SizeOfOptionalHeader;

    for (WORD i = 0; i < Nt.FileHeader.NumberOfSections; i++)
    {
        IMAGE_SECTION_HEADER Section;

        if (!ReadProcessMemory(Process, (PBYTE)SectionBase + (UINT64)i * sizeof(Section), &Section, sizeof(Section), &Got))
            return FALSE;

        if (memcmp(Section.Name, ".text", 6) != 0)
            continue;

        UINT64 Start = Base + Section.VirtualAddress;
        UINT64 Size  = Section.Misc.VirtualSize ? Section.Misc.VirtualSize : Section.SizeOfRawData;

        if (Size == 0)
            return FALSE;

        g_Code = (UINT8 *)malloc((size_t)Size);
        if (g_Code == NULL)
            return FALSE;

        if (!ReadProcessMemory(Process, (PVOID)Start, g_Code, (SIZE_T)Size, &Got) || Got != Size)
        {
            free(g_Code);
            g_Code = NULL;
            return FALSE;
        }

        g_CodeBase = Start;
        g_CodeSize = Size;
        *TextStart = Start;
        *TextEnd   = Start + Size - 1;
        return TRUE;
    }

    return FALSE;
}

static BOOLEAN
ResolveFunction(HANDLE Process, const char * Path, const char * Name, UINT64 * Start, UINT64 * End)
{
    union
    {
        SYMBOL_INFO Info;
        BYTE        Buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
    } Symbol = {0};
    BOOLEAN Ok = FALSE;

    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    if (!SymInitialize(Process, NULL, FALSE))
        return FALSE;

    if (SymLoadModuleEx(Process, NULL, Path, NULL, (DWORD64)g_ImageBase, 0, NULL, 0) != 0)
    {
        Symbol.Info.SizeOfStruct = sizeof(SYMBOL_INFO);
        Symbol.Info.MaxNameLen   = MAX_SYM_NAME;

        if (SymFromName(Process, Name, &Symbol.Info) && Symbol.Info.Address != 0)
        {
            *Start = Symbol.Info.Address;
            *End   = Symbol.Info.Address + (Symbol.Info.Size ? Symbol.Info.Size : 0x200) - 1;
            Ok     = TRUE;
        }
    }

    SymCleanup(Process);
    return Ok;
}

static BOOLEAN
PtOperation(HYPERTRACE_PT_OPERATION_REQUEST_TYPE Type)
{
    HYPERTRACE_PT_OPERATION_PACKETS Op = {};
    Op.PtOperationType = Type;
    return hyperdbg_u_pt_operation(&Op);
}

static BOOLEAN
PtFilter(UINT32 ProcessId, UINT64 Start, UINT64 End)
{
    HYPERTRACE_PT_OPERATION_PACKETS Op = {};

    Op.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FILTER;
    Op.TraceUser       = 1;
    Op.TargetProcessId = ProcessId;

    if (End > Start)
    {
        Op.NumAddrRanges       = 1;
        Op.AddrRanges[0].Start = Start;
        Op.AddrRanges[0].End   = End;
    }

    if (!hyperdbg_u_pt_operation(&Op))
        return FALSE;

    printf("[+] PT filter: cr3=0x%llx traceuser=%u ranges=%u buffer=0x%llx\n",
           (unsigned long long)Op.TargetCr3,
           Op.TraceUser,
           Op.NumAddrRanges,
           (unsigned long long)Op.BufferSize);
    return TRUE;
}

static const char *
PacketName(enum pt_packet_type Type)
{
    switch (Type)
    {
    case ppt_psb: return "PSB"; case ppt_psbend: return "PSBEND"; case ppt_pad: return "PAD";
    case ppt_fup: return "FUP"; case ppt_tip: return "TIP"; case ppt_tip_pge: return "TIP.PGE";
    case ppt_tip_pgd: return "TIP.PGD"; case ppt_tnt_8: return "TNT8"; case ppt_tnt_64: return "TNT64";
    case ppt_mode: return "MODE"; case ppt_pip: return "PIP"; case ppt_vmcs: return "VMCS";
    case ppt_cbr: return "CBR"; case ppt_tsc: return "TSC"; case ppt_tma: return "TMA";
    case ppt_mtc: return "MTC"; case ppt_cyc: return "CYC"; case ppt_ovf: return "OVF";
    case ppt_stop: return "STOP"; case ppt_exstop: return "EXSTOP"; case ppt_mnt: return "MNT";
    case ppt_ptw: return "PTW"; default: return "?";
    }
}

static uint64_t
ReconstructIp(const struct pt_packet_ip * Packet, uint64_t * LastIp)
{
    uint64_t Value = *LastIp;

    switch (Packet->ipc)
    {
    case pt_ipc_update_16: Value = (Value & ~0xffffull) | (Packet->ip & 0xffffull); break;
    case pt_ipc_update_32: Value = (Value & ~0xffffffffull) | (Packet->ip & 0xffffffffull); break;
    case pt_ipc_update_48: Value = (Value & ~0xffffffffffffull) | (Packet->ip & 0xffffffffffffull); break;
    case pt_ipc_sext_48:
        Value = Packet->ip & 0xffffffffffffull;
        if (Value & 0x800000000000ull)
            Value |= 0xffff000000000000ull;
        break;
    default: Value = Packet->ip; break;
    }

    *LastIp = Value;
    return Value;
}

static UINT64
DecodeCorePackets(UINT32 Cpu, const UINT8 * Buffer, UINT64 Size)
{
    struct pt_config           Config;
    struct pt_packet_decoder * Decoder;
    UINT64                     Count  = 0;
    uint64_t                   LastIp = 0;
    int                        Status;

    pt_config_init(&Config);
    Config.begin = (uint8_t *)Buffer;
    Config.end   = (uint8_t *)Buffer + Size;

    Decoder = pt_pkt_alloc_decoder(&Config);
    if (Decoder == NULL)
    {
        printf("[-] core %u: cannot allocate packet decoder\n", Cpu);
        return 0;
    }

    for (;;)
    {
        Status = pt_pkt_sync_forward(Decoder);
        if (Status < 0)
            break;

        for (;;)
        {
            struct pt_packet Packet;

            Status = pt_pkt_next(Decoder, &Packet, sizeof(Packet));
            if (Status < 0)
                break;

            Count++;

            switch (Packet.type)
            {
            case ppt_tnt_8:
            case ppt_tnt_64:
                printf("    %-8s %2u  ", PacketName(Packet.type), Packet.payload.tnt.bit_size);
                for (uint8_t Bit = 0; Bit < Packet.payload.tnt.bit_size && Bit < 64; Bit++)
                    putchar(((Packet.payload.tnt.payload >> (Packet.payload.tnt.bit_size - 1 - Bit)) & 1) ? 'T' : 'N');
                putchar('\n');
                break;

            case ppt_tip:
            case ppt_fup:
            case ppt_tip_pge:
            case ppt_tip_pgd:
                if (Packet.payload.ip.ipc == pt_ipc_suppressed)
                    printf("    %-8s (ip suppressed)\n", PacketName(Packet.type));
                else
                {
                    uint64_t Ip = ReconstructIp(&Packet.payload.ip, &LastIp);
                    printf("    %-8s 0x%016llx  exe+0x%llx\n",
                           PacketName(Packet.type), (unsigned long long)Ip, (unsigned long long)(Ip - g_ImageBase));
                }
                break;

            case ppt_pip:
                printf("    %-8s cr3=0x%llx\n", PacketName(Packet.type), (unsigned long long)Packet.payload.pip.cr3);
                break;

            case ppt_cbr:
                //printf("    %-8s ratio=%u\n", PacketName(Packet.type), Packet.payload.cbr.ratio);
                break;

            case ppt_tsc:
                printf("    %-8s tsc=0x%llx\n", PacketName(Packet.type), (unsigned long long)Packet.payload.tsc.tsc);
                break;

            default:
                //printf("    %-8s\n", PacketName(Packet.type));
                break;
            }
        }
    }

    pt_pkt_free_decoder(Decoder);
    return Count;
}

static UINT64
DecodeCore(UINT32 Cpu, const UINT8 * Buffer, UINT64 Size)
{
    struct pt_config         Config;
    struct pt_insn_decoder * Decoder;
    struct pt_image *        Image;
    UINT64                   Count = 0;
    int                      Status;

    pt_config_init(&Config);
    Config.begin = (uint8_t *)Buffer;
    Config.end   = (uint8_t *)Buffer + Size;

    Decoder = pt_insn_alloc_decoder(&Config);
    if (Decoder == NULL)
    {
        printf("[-] core %u: cannot allocate instruction decoder\n", Cpu);
        return 0;
    }

    Image = pt_insn_get_image(Decoder);
    pt_image_set_callback(Image, ReadImage, NULL);

    for (;;)
    {
        Status = pt_insn_sync_forward(Decoder);
        if (Status < 0)
            break;

        for (;;)
        {
            struct pt_insn Insn;

            while (Status & pts_event_pending)
            {
                struct pt_event Event;
                Status = pt_insn_event(Decoder, &Event, sizeof(Event));
                if (Status < 0)
                    break;
            }

            if (Status < 0 || (Status & pts_eos))
                break;

            Status = pt_insn_next(Decoder, &Insn, sizeof(Insn));
            if (Status < 0)
                break;

            ZydisDisassembledInstruction Disasm;
            ZydisMachineMode             Mode = (Insn.mode == ptem_32bit) ? ZYDIS_MACHINE_MODE_LEGACY_32 : ZYDIS_MACHINE_MODE_LONG_64;

            if (ZYAN_SUCCESS(ZydisDisassembleIntel(Mode, Insn.ip, Insn.raw, Insn.size, &Disasm)))
                printf("    0x%016llx  exe+0x%-6llx  %s\n",
                       (unsigned long long)Insn.ip,
                       (unsigned long long)(Insn.ip - g_ImageBase),
                       Disasm.text);
            else
                printf("    0x%016llx  (undecodable)\n", (unsigned long long)Insn.ip);

            Count++;
        }

        if (Status >= 0 && (Status & pts_eos))
            break;
    }

    pt_insn_free_decoder(Decoder);
    return Count;
}

static void
RunAndTrace(const char * Path, const char * Function, BOOLEAN Packets, int PinCore)
{
    STARTUPINFOA                    Startup     = {0};
    PROCESS_INFORMATION             Process     = {0};
    HYPERTRACE_PT_MMAP_PACKETS      Mmap        = {0};
    HYPERTRACE_PT_OPERATION_PACKETS Sizes       = {};
    UINT64                          TextStart   = 0;
    UINT64                          TextEnd     = 0;
    UINT64                          FilterStart = 0;
    UINT64                          FilterEnd   = 0;
    UINT64                          Total       = 0;

    Startup.cb = sizeof(Startup);

    if (!CreateProcessA(Path, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &Startup, &Process))
    {
        printf("[-] cannot launch '%s' (error 0x%x)\n", Path, GetLastError());
        return;
    }

    printf("[+] launched '%s' (pid %u, suspended)\n", Path, Process.dwProcessId);

    if (PinCore >= 0)
    {
        DWORD_PTR Mask = (DWORD_PTR)1 << PinCore;
        if (SetProcessAffinityMask(Process.hProcess, Mask))
            printf("[+] pinned target to core %d (all trace should land on this core)\n", PinCore);
        else
            printf("[!] could not pin to core %d (error 0x%x); running unpinned\n", PinCore, GetLastError());
    }
    else
    {
        printf("[*] target unpinned (scheduler may migrate it across cores)\n");
    }

    if (!CaptureImage(Process.hProcess, &TextStart, &TextEnd))
    {
        printf("[-] cannot read target image / .text section\n");
        TerminateProcess(Process.hProcess, 1);
        goto Cleanup;
    }

    printf("[+] image base 0x%llx, .text 0x%llx-0x%llx (%llu bytes)\n",
           (unsigned long long)g_ImageBase,
           (unsigned long long)TextStart,
           (unsigned long long)TextEnd,
           (unsigned long long)g_CodeSize);

    FilterStart = TextStart;
    FilterEnd   = TextEnd;

    if (Function != NULL && ResolveFunction(Process.hProcess, Path, Function, &FilterStart, &FilterEnd))
        printf("[+] IP filter narrowed to '%s' 0x%llx-0x%llx (%llu bytes)\n",
               Function,
               (unsigned long long)FilterStart,
               (unsigned long long)FilterEnd,
               (unsigned long long)(FilterEnd - FilterStart + 1));
    else
        printf("[!] IP filter: whole .text (symbol '%s' not found - build the target with a PDB)\n",
               Function ? Function : "(none)");

    if (!PtFilter(Process.dwProcessId, FilterStart, FilterEnd) ||
        !PtOperation(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_ENABLE))
    {
        printf("[-] cannot enable Intel PT\n");
        TerminateProcess(Process.hProcess, 1);
        goto Cleanup;
    }

    if (!hyperdbg_u_pt_mmap(&Mmap))
    {
        printf("[-] pt_mmap failed\n");
        PtOperation(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE);
        TerminateProcess(Process.hProcess, 1);
        goto Cleanup;
    }

    printf("[+] PT enabled, %u per-core buffers mapped\n", Mmap.NumCpus);
    printf("[*] resuming target and waiting for it to exit...\n");

    ResumeThread(Process.hThread);
    WaitForSingleObject(Process.hProcess, INFINITE);
    printf("[+] target exited, decoding trace\n");

    PtOperation(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_PAUSE);

    Sizes.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_SIZE;
    if (!hyperdbg_u_pt_operation(&Sizes))
    {
        printf("[-] cannot query PT sizes\n");
        PtOperation(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE);
        goto Cleanup;
    }

    for (UINT32 i = 0; i < Mmap.NumCpus; i++)
    {
        UINT32 Cpu   = Mmap.Cpus[i].CpuId;
        UINT64 Bytes = (Cpu < Sizes.NumCpus) ? Sizes.BytesPerCpu[Cpu] : 0;

        if (Bytes == 0)
            continue;

        if (Bytes > Mmap.Cpus[i].Size)
            Bytes = Mmap.Cpus[i].Size;

        printf("\n[*] core %u: %llu bytes of trace\n", Cpu, (unsigned long long)Bytes);
        Total += Packets
                     ? DecodeCorePackets(Cpu, (const UINT8 *)(ULONG_PTR)Mmap.Cpus[i].UserVa, Bytes)
                     : DecodeCore(Cpu, (const UINT8 *)(ULONG_PTR)Mmap.Cpus[i].UserVa, Bytes);
    }

    printf("\n[+] decoded %llu %s total\n", (unsigned long long)Total, Packets ? "packet(s)" : "instruction(s)");

    PtOperation(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE);

Cleanup:
    if (g_Code != NULL)
    {
        free(g_Code);
        g_Code = NULL;
    }
    if (Process.hThread != NULL)
        CloseHandle(Process.hThread);
    if (Process.hProcess != NULL)
        CloseHandle(Process.hProcess);
}

static int
LoadVmmAndTrace()
{
    hyperdbg_u_set_text_message_callback((PVOID)ShowMessages);

    if (!hyperdbg_u_detect_vmx_support())
    {
        printf("[-] VT-x (VMX) is not supported / enabled on this processor\n");
        return 1;
    }

    printf("[*] loading HyperDbg VMM...\n");
    if (hyperdbg_u_install_kd_driver() == 1 || hyperdbg_u_load_vmm() == 1)
    {
        printf("[-] cannot load the HyperDbg VMM\n");
        return 1;
    }

    printf("[+] HyperDbg VMM is running\n");

    printf("[*] loading HyperTrace...\n");

    if ( hyperdbg_u_load_hypertrace_module() == 1)
    {
        printf("[-] cannot load the HyperDbg HyperTrace\n");
        return 1;
    }

    printf("[+] HyperDbg HyperTrace is running\n");

    return 0;
}

int
main2(int argc, char ** argv)
{
    const char * function = "main";
    BOOLEAN      packets  = FALSE;
    int          pinCore  = 0;

    if (argc < 2)
    {
        printf("HyperDbg Intel PT tracer\n");
        printf("usage: %s <path-to-exe-that-exits> [function] [-p] [-c core]\n", argv[0]);
        printf("  [function]  symbol to IP-filter (default 'main'; pass '*' for whole .text)\n");
        printf("  -p          dump raw PT packets (TNT/TIP/FUP/PSB/...) instead of instructions\n");
        printf("  -c core     pin the target to this logical core (default 0; -1 = unpinned)\n");
        return 1;
    }

    for (int i = 2; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0 || strcmp(argv[i], "--packets") == 0)
            packets = TRUE;
        else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc)
            pinCore = atoi(argv[++i]);
        else if (strcmp(argv[i], "*") == 0)
            function = NULL;
        else
            function = argv[i];
    }

    if (LoadVmmAndTrace() != 0)
        return 1;

    RunAndTrace(argv[1], function, packets, pinCore);

    printf("[*] unloading HyperDbg VMM...\n");

    //
    // Unload the driver
    //
    hyperdbg_u_unload_vmm();
    hyperdbg_u_unload_kd();
    hyperdbg_u_stop_kd_driver();
    hyperdbg_u_uninstall_kd_driver();

    printf("[+] done\n");

    return 0;
}
