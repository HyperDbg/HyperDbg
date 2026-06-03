/**
 * @file hyperdbg-app.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Run a target executable under Intel PT (via HyperDbg's hypertrace
 *        engine) and decode the captured trace with libipt.
 * @details
 *        Flow:
 *          1. load the HyperDbg VMM (local / VMI mode)
 *          2. create the target process suspended
 *          3. !pt filter for that process only (CR3 filter, resolved from PID)
 *          4. !pt enable  + pt_mmap  (map per-CPU PT buffers into this process)
 *          5. resume the target and wait for it to finish
 *          6. !pt pause   + !pt size (snapshot per-CPU valid byte counts)
 *          7. decode every per-CPU buffer with libipt and print the packets
 *          8. !pt disable (tears the mappings + buffers down) + unload VMM
 *
 * @version 0.2
 * @date 2023-02-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#include <string.h>

//
// libipt (Intel Processor Trace decoder).
//
// The include / library directories come from the 'LibIptDir' MSBuild macro
// defined in hyperdbg_app.vcxproj — set it to your libipt install root (the
// folder that contains 'include\intel-pt.h' and 'lib\libipt.lib'), and make
// sure 'libipt.dll' is reachable at run time (next to the .exe or on PATH).
//
#include <intel-pt.h>

/**
 * @brief Show messages coming from HyperDbg (driver + library)
 */
int
hyperdbg_show_messages(const char * Text)
{
    printf("%s", Text);
    return 0;
}

/**
 * @brief Human-readable name for a PT packet type
 */
static const char *
PtPacketName(enum pt_packet_type Type)
{
    switch (Type)
    {
    case ppt_psb:     return "PSB";
    case ppt_psbend:  return "PSBEND";
    case ppt_pad:     return "PAD";
    case ppt_fup:     return "FUP";
    case ppt_tip:     return "TIP";
    case ppt_tip_pge: return "TIP.PGE";
    case ppt_tip_pgd: return "TIP.PGD";
    case ppt_tnt_8:   return "TNT-8";
    case ppt_tnt_64:  return "TNT-64";
    case ppt_mode:    return "MODE";
    case ppt_pip:     return "PIP";
    case ppt_vmcs:    return "VMCS";
    case ppt_cbr:     return "CBR";
    case ppt_tsc:     return "TSC";
    case ppt_tma:     return "TMA";
    case ppt_mtc:     return "MTC";
    case ppt_cyc:     return "CYC";
    case ppt_stop:    return "STOP";
    case ppt_ovf:     return "OVF";
    case ppt_mnt:     return "MNT";
    case ppt_exstop:  return "EXSTOP";
    case ppt_ptw:     return "PTW";
    default:          return "UNKNOWN";
    }
}

/**
 * @brief Pretty-print a single decoded PT packet
 */
static void
PtPrintPacket(UINT32 Cpu, const struct pt_packet * Packet)
{
    switch (Packet->type)
    {
    case ppt_pad:
        //
        // Padding — skip to keep the dump readable
        //
        break;
    case ppt_fup:
    case ppt_tip:
    case ppt_tip_pge:
    case ppt_tip_pgd:
        printf("  [cpu %u] %-8s ip=0x%016llx\n", Cpu, PtPacketName(Packet->type), (unsigned long long)Packet->payload.ip.ip);
        break;
    case ppt_tnt_8:
    case ppt_tnt_64:
        printf("  [cpu %u] %-8s taken/not-taken bits=%u\n", Cpu, PtPacketName(Packet->type), Packet->payload.tnt.bit_size);
        break;
    case ppt_pip:
        printf("  [cpu %u] %-8s cr3=0x%016llx\n", Cpu, PtPacketName(Packet->type), (unsigned long long)Packet->payload.pip.cr3);
        break;
    case ppt_tsc:
        printf("  [cpu %u] %-8s tsc=%llu\n", Cpu, PtPacketName(Packet->type), (unsigned long long)Packet->payload.tsc.tsc);
        break;
    case ppt_cbr:
        printf("  [cpu %u] %-8s ratio=%u\n", Cpu, PtPacketName(Packet->type), Packet->payload.cbr.ratio);
        break;
    default:
        printf("  [cpu %u] %-8s\n", Cpu, PtPacketName(Packet->type));
        break;
    }
}

/**
 * @brief Decode one CPU's raw PT byte stream with libipt and print packets
 *
 * @param Cpu  logical processor index (for labelling)
 * @param Buf  start of the mapped PT buffer (main + overflow)
 * @param Size number of valid bytes to decode
 */
static void
PtDecodeBuffer(UINT32 Cpu, const UINT8 * Buf, UINT64 Size)
{
    struct pt_config          Config;
    struct pt_packet_decoder * Decoder;
    UINT64                     Count = 0;
    int                        Status;

    printf("\n==== core %u : decoding %llu bytes ====\n", Cpu, (unsigned long long)Size);

    memset(&Config, 0, sizeof(Config));
    Config.size  = sizeof(Config);
    Config.begin = (uint8_t *)Buf;
    Config.end   = (uint8_t *)Buf + Size;

    Decoder = pt_pkt_alloc_decoder(&Config);
    if (Decoder == NULL)
    {
        printf("  err, could not allocate libipt packet decoder\n");
        return;
    }

    //
    // Sync to the first PSB in the stream. The buffer is circular, so the
    // very start may be mid-packet garbage — sync_forward skips to a real
    // packet boundary.
    //
    Status = pt_pkt_sync_forward(Decoder);
    if (Status < 0)
    {
        printf("  no synchronization point found (%s)\n", pt_errstr(pt_errcode(Status)));
        pt_pkt_free_decoder(Decoder);
        return;
    }

    for (;;)
    {
        struct pt_packet Packet;

        Status = pt_pkt_next(Decoder, &Packet, sizeof(Packet));
        if (Status < 0)
        {
            if (Status == -pte_eos)
            {
                //
                // End of stream
                //
                break;
            }

            //
            // Decode error — try to resync at the next PSB
            //
            Status = pt_pkt_sync_forward(Decoder);
            if (Status < 0)
            {
                break;
            }
            continue;
        }

        PtPrintPacket(Cpu, &Packet);
        Count++;
    }

    printf("  ---- core %u : %llu packets decoded ----\n", Cpu, (unsigned long long)Count);
    pt_pkt_free_decoder(Decoder);
}

/**
 * @brief Send a single PT operation and report failures
 */
static BOOLEAN
PtDo(HYPERTRACE_PT_OPERATION_REQUEST_TYPE Type, UINT32 TargetPid)
{
    HYPERTRACE_PT_OPERATION_PACKETS Op = {0};

    Op.PtOperationType = Type;

    if (Type == HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FILTER)
    {
        //
        // Trace user-mode execution of the target process only. The kernel
        // resolves TargetProcessId to the right CR3 for the requested mode
        // (the KVA-shadow user CR3 here, since we trace user mode) and
        // programs the PT CR3 filter, so other processes are not traced.
        //
        Op.TraceUser       = 1;
        Op.TraceKernel     = 0;
        Op.TargetProcessId = TargetPid;
    }

    return hyperdbg_u_pt_operation(&Op);
}

/**
 * @brief Run the target under Intel PT and decode the result
 */
static int
RunAndTrace(const char * TargetPath)
{
    STARTUPINFOA                    Si = {0};
    PROCESS_INFORMATION             Pi = {0};
    HYPERTRACE_PT_MMAP_PACKETS      Mmap = {0};
    HYPERTRACE_PT_OPERATION_PACKETS Size = {0};

    Si.cb = sizeof(Si);

    //
    // Create the target suspended so PT is armed before its first instruction
    //
    if (!CreateProcessA(TargetPath, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &Si, &Pi))
    {
        printf("err, could not start '%s' (0x%x)\n", TargetPath, GetLastError());
        return 1;
    }

    printf("started '%s' (pid %u), arming Intel PT...\n", TargetPath, Pi.dwProcessId);

    //
    // 1) Filter for this process, 2) enable tracing, 3) map the buffers
    //
    if (!PtDo(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_FILTER, Pi.dwProcessId) ||
        !PtDo(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_ENABLE, 0))
    {
        printf("err, could not enable Intel PT\n");
        TerminateProcess(Pi.hProcess, 1);
        goto Cleanup;
    }

    if (!hyperdbg_u_pt_mmap(&Mmap))
    {
        printf("err, pt_mmap failed\n");
        PtDo(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE, 0);
        TerminateProcess(Pi.hProcess, 1);
        goto Cleanup;
    }

    printf("Intel PT enabled, %u per-CPU buffers mapped:\n", Mmap.NumCpus);
    for (UINT32 i = 0; i < Mmap.NumCpus; i++)
    {
        printf("  core %u : va=0x%016llx size=0x%llx\n",
               Mmap.Cpus[i].CpuId,
               (unsigned long long)Mmap.Cpus[i].UserVa,
               (unsigned long long)Mmap.Cpus[i].Size);
    }

    //
    // Let the target run to completion
    //
    printf("resuming target and waiting for it to finish...\n");
    ResumeThread(Pi.hThread);
    WaitForSingleObject(Pi.hProcess, INFINITE);
    printf("target finished, pausing Intel PT\n");

    //
    // Stop tracing (flushes hardware buffers), then snapshot byte counts
    //
    PtDo(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_PAUSE, 0);

    Size.PtOperationType = HYPERTRACE_PT_OPERATION_REQUEST_TYPE_SIZE;
    if (!hyperdbg_u_pt_operation(&Size))
    {
        printf("err, could not query PT sizes\n");
        PtDo(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE, 0);
        goto Cleanup;
    }

    //
    // Decode each CPU's trace (match the mmap descriptor by CpuId)
    //
    for (UINT32 i = 0; i < Mmap.NumCpus; i++)
    {
        UINT32 Cpu   = Mmap.Cpus[i].CpuId;
        UINT64 Bytes = (Cpu < Size.NumCpus) ? Size.BytesPerCpu[Cpu] : 0;

        if (Bytes == 0)
        {
            continue;
        }

        if (Bytes > Mmap.Cpus[i].Size)
        {
            Bytes = Mmap.Cpus[i].Size;
        }

        PtDecodeBuffer(Cpu, (const UINT8 *)(ULONG_PTR)Mmap.Cpus[i].UserVa, Bytes);
    }

    //
    // Disable PT (this also tears down the user mappings)
    //
    PtDo(HYPERTRACE_PT_OPERATION_REQUEST_TYPE_DISABLE, 0);

Cleanup:
    CloseHandle(Pi.hThread);
    CloseHandle(Pi.hProcess);
    return 0;
}

/**
 * @brief Load the HyperDbg VMM in local (VMI) mode
 *
 * @return int zero on success
 */
static int
hyperdbg_load()
{
    hyperdbg_u_set_text_message_callback(hyperdbg_show_messages);

    if (!hyperdbg_u_detect_vmx_support())
    {
        printf("err, vmx operation is not supported on this processor\n");
        return 1;
    }

    //
    // Install + load the VMM driver (mirrors the CLI's 'load vmm')
    //
    if (hyperdbg_u_install_vmm_driver() != 0 || hyperdbg_u_load_vmm() != 0)
    {
        printf("err, could not load the HyperDbg VMM\n");
        return 1;
    }

    return 0;
}

/**
 * @brief main function
 *
 * @return int
 */
int
main(int argc, char ** argv)
{
    if (argc < 2)
    {
        printf("usage: %s <path-to-exe-that-exits>\n", argv[0]);
        return 1;
    }

    if (hyperdbg_load() != 0)
    {
        printf("err, in loading HyperDbg\n");
        return 1;
    }

    RunAndTrace(argv[1]);

    //
    // Turn the hypervisor off
    //
    hyperdbg_u_unload_vmm();
    return 0;
}
