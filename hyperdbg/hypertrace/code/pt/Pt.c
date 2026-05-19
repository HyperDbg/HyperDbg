/**
 * @file Pt.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Processor Trace (PT) tracing implementation for HyperTrace module
 * @details Programs Intel PT MSRs and manages per-CPU ToPA / output buffers.
 *          The engine half (PtEngine*) deals with a single PT_PER_CPU at a
 *          time and is OS-agnostic. The HyperDbg wrappers (PtCheck, PtStart,
 *          PtStop, PtPause, PtResume, PtSize, PtDump, PtFlush) operate on the
 *          global per-CPU state list (g_PtStateList) and mirror the LBR API
 *          surface.
 * @version 0.19
 * @date 2026-04-29
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

//////////////////////////////////////////////////
//          Internal allocation helpers         //
//////////////////////////////////////////////////

/**
 * @brief Allocate physically contiguous memory for ToPA / output buffers.
 *
 * Intel PT requires the output region's physical address to be aligned to
 * its own size. We use MmAllocateContiguousMemorySpecifyCache with a
 * BoundaryAddressMultipleOf equal to the requested size — the allocator
 * then picks a base that does not cross any size-aligned boundary, which
 * for a power-of-two size means the base is a multiple of that size.
 *
 * @param Size       Bytes to allocate. Must be a power of two for size
 *                   alignment.
 * @param Alignment  Requested alignment in bytes. Pass `Size` for the
 *                   ToPA output region; pass PT_PAGE_SIZE for the ToPA
 *                   table itself and the overflow page.
 *
 * @return PVOID  Virtual address of the allocation, or NULL on failure.
 */
static PVOID
PtAllocateContiguousAligned(UINT64 Size, UINT64 Alignment)
{
    PHYSICAL_ADDRESS Lo       = {0};
    PHYSICAL_ADDRESS Hi       = {0};
    PHYSICAL_ADDRESS Boundary = {0};
    PVOID            Result;

    Hi.QuadPart       = MAXULONG64;
    Boundary.QuadPart = (LONGLONG)Alignment;

    Result = MmAllocateContiguousMemorySpecifyCache(
        (SIZE_T)Size,
        Lo,
        Hi,
        Boundary,
        MmCached);

    if (Result != NULL)
    {
        RtlSecureZeroMemory(Result, (SIZE_T)Size);
    }

    return Result;
}

/**
 * @brief Free a physically contiguous allocation made by PtAllocateContiguousAligned.
 */
static VOID
PtFreeContiguous(PVOID Va)
{
    if (Va != NULL)
    {
        MmFreeContiguousMemory(Va);
    }
}

/**
 * @brief Translate a kernel virtual address to a physical address.
 */
static UINT64
PtVaToPa(PVOID Va)
{
    PHYSICAL_ADDRESS Pa = MmGetPhysicalAddress(Va);
    return (UINT64)Pa.QuadPart;
}

//////////////////////////////////////////////////
//          User-mode mmap helpers              //
//////////////////////////////////////////////////

/**
 * @brief Build an MDL whose PFN list concatenates the main output
 *        buffer and the overflow page, then map the combined region
 *        into the current user process as one virtually contiguous
 *        range.
 *
 *        Main and overflow come from separate
 *        MmAllocateContiguousMemorySpecifyCache calls so they are each
 *        physically contiguous but not contiguous with each other. By
 *        constructing one MDL whose PFN array is [main pfns | overflow
 *        pfns], MmMapLockedPagesSpecifyCache produces a single user VA
 *        range of MainSize + OverflowSize bytes that the consumer reads
 *        as one stream (main first, overflow second). Pages are
 *        non-paged so MDL_PAGES_LOCKED is enough to satisfy the mapper.
 *
 *        Wrapped in SEH because MmMapLockedPagesSpecifyCache raises on
 *        quota / VAD failures rather than returning NULL.
 *
 * @return INT32  0 on success, -1 on failure (outputs untouched).
 */
static INT32
PtMmapCpuRegionToUser(PVOID  MainVa,
                      UINT64 MainPhysical,
                      SIZE_T MainSize,
                      UINT64 OverflowPhysical,
                      SIZE_T OverflowSize,
                      PMDL * OutMdl,
                      PVOID * OutUserVa)
{
    SIZE_T       TotalSize = MainSize + OverflowSize;
    ULONG        MainPages;
    ULONG        OverflowPages;
    ULONG        i;
    PFN_NUMBER * Pfns;
    PFN_NUMBER   MainPfnBase;
    PFN_NUMBER   OverflowPfnBase;
    PMDL         Mdl;
    PVOID        UserVa = NULL;

    if (MainVa == NULL || MainSize == 0 || OverflowSize == 0 || TotalSize == 0)
        return -1;

    //
    // Sizes must be whole pages — ToPA-backed buffers always are.
    //
    if ((MainSize & (PT_PAGE_SIZE - 1)) != 0 || (OverflowSize & (PT_PAGE_SIZE - 1)) != 0)
        return -1;

    //
    // IoAllocateMdl sizes the embedded PFN array from the byte range we
    // pass in. The seed VA only sets the byte-offset within the first
    // page (0 for our page-aligned contiguous allocations); we overwrite
    // the PFN list below so it stops describing MainVa past its own end.
    //
    Mdl = IoAllocateMdl(MainVa, (ULONG)TotalSize, FALSE, FALSE, NULL);
    if (Mdl == NULL)
        return -1;

    MainPages       = (ULONG)(MainSize >> PAGE_SHIFT);
    OverflowPages   = (ULONG)(OverflowSize >> PAGE_SHIFT);
    MainPfnBase     = (PFN_NUMBER)(MainPhysical >> PAGE_SHIFT);
    OverflowPfnBase = (PFN_NUMBER)(OverflowPhysical >> PAGE_SHIFT);

    Pfns = MmGetMdlPfnArray(Mdl);
    for (i = 0; i < MainPages; i++)
        Pfns[i] = MainPfnBase + i;
    for (i = 0; i < OverflowPages; i++)
        Pfns[MainPages + i] = OverflowPfnBase + i;

    //
    // Pages are physically present and non-pageable (contiguous memory);
    // tag the MDL so MmMapLockedPagesSpecifyCache accepts it.
    //
    Mdl->MdlFlags |= MDL_PAGES_LOCKED;

    __try
    {
        UserVa = MmMapLockedPagesSpecifyCache(
            Mdl,
            UserMode,
            MmCached,
            NULL,
            FALSE,
            NormalPagePriority);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        UserVa = NULL;
    }

    if (UserVa == NULL)
    {
        IoFreeMdl(Mdl);
        return -1;
    }

    *OutMdl    = Mdl;
    *OutUserVa = UserVa;
    return 0;
}

/**
 * @brief Tear down the combined user mapping produced by
 *        PtMmapCpuRegionToUser. Caller must be in the mapping process;
 *        see the cooperative single-process contract.
 */
static VOID
PtUnmapCpuRegionFromUser(PMDL Mdl, PVOID UserVa)
{
    if (UserVa != NULL && Mdl != NULL)
    {
        __try
        {
            MmUnmapLockedPages(UserVa, Mdl);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            //
            // Mapping process may have already exited.
            //
        }
    }

    if (Mdl != NULL)
    {
        IoFreeMdl(Mdl);
    }
}

//////////////////////////////////////////////////
//                Engine routines               //
//////////////////////////////////////////////////

/**
 * @brief Convert a buffer size in bytes to the ToPA Size field encoding.
 *        Valid sizes are 4KB * 2^N for N = 0..15.
 *
 * @param SizeInBytes
 * @return INT32  Encoding 0..15, or -1 if the size is not supported.
 */
INT32
PtEngineSizeToTopaEncoding(UINT64 SizeInBytes)
{
    UINT64 Pages;
    INT32  N;

    if (SizeInBytes < PT_PAGE_SIZE)
        return -1;

    Pages = SizeInBytes / PT_PAGE_SIZE;

    //
    // Must be an exact power-of-two number of pages
    //
    if ((Pages & (Pages - 1)) != 0)
        return -1;

    N = 0;
    while ((Pages >> N) != 1)
        N++;

    if (N > 15)
        return -1;

    return N;
}

/**
 * @brief Probe Intel PT capabilities via CPUID leaf 7 / leaf 0x14.
 *
 * @param OutCaps  Optional. If non-NULL, populated with detailed capabilities.
 *
 * @return INT32   0 on success (PT supported), -1 if PT is not supported.
 */
INT32
PtEngineQueryCapabilities(PT_CAPABILITIES * OutCaps)
{
    int    Info[4]    = {0};
    int    MaxSubleaf = 0;
    UINT64 VmxMisc;

    //
    // Check PT support: CPUID.(EAX=07H,ECX=0):EBX[25]
    //
    __cpuidex(Info, 0x07, 0);
    if ((Info[1] & (1 << 25)) == 0)
        return -1;

    if (OutCaps == NULL)
        return 0;

    RtlZeroMemory(OutCaps, sizeof(*OutCaps));

    //
    // Leaf 0x14, sub-leaf 0: capability flags
    //
    Info[0] = Info[1] = Info[2] = Info[3] = 0;
    __cpuidex(Info, 0x14, 0);

    MaxSubleaf = Info[0]; /* EAX: max valid sub-leaf */

    //
    // EBX capabilities
    //
    OutCaps->Cr3Filtering       = (Info[1] >> 0) & 1;
    OutCaps->PsbCycConfigurable = (Info[1] >> 1) & 1;
    OutCaps->IpFiltering        = (Info[1] >> 2) & 1;
    OutCaps->MtcSupport         = (Info[1] >> 3) & 1;
    OutCaps->PtwriteSupport     = (Info[1] >> 4) & 1;
    OutCaps->PowerEventTrace    = (Info[1] >> 5) & 1;

    //
    // ECX capabilities
    //
    OutCaps->TopaOutput        = (Info[2] >> 0) & 1;
    OutCaps->TopaMultiEntry    = (Info[2] >> 1) & 1;
    OutCaps->SingleRangeOutput = (Info[2] >> 2) & 1;
    OutCaps->TransportOutput   = (Info[2] >> 3) & 1;
    OutCaps->IpPayloadsAreLip  = (Info[2] >> 31) & 1;

    //
    // Sub-leaf 1: detailed numerics
    //
    if (MaxSubleaf >= 1)
    {
        Info[0] = Info[1] = Info[2] = Info[3] = 0;
        __cpuidex(Info, 0x14, 1);

        OutCaps->NumAddrRanges      = (UINT32)(Info[0] & 0x7);
        OutCaps->MtcPeriodBitmap    = (UINT16)((Info[0] >> 16) & 0xFFFF);
        OutCaps->CycThresholdBitmap = (UINT16)(Info[1] & 0xFFFF);
        OutCaps->PsbFreqBitmap      = (UINT16)((Info[1] >> 16) & 0xFFFF);
    }

    //
    // VMX support: IA32_VMX_MISC[14] indicates Intel PT may be used in VMX
    // operation. Reading IA32_VMX_MISC is unconditional on a VMX-capable
    // CPU; if the CPU isn't VMX-capable the bit is meaningless and the
    // value falls through as 0.
    //
    VmxMisc             = __readmsr(0x00000485 /* IA32_VMX_MISC */);
    OutCaps->VmxSupport = (VmxMisc >> 14) & 1;

    return 0;
}

/**
 * @brief Initialize a PT_TRACE_CONFIG with sensible defaults.
 *        Trace user + kernel, branch + TSC packets, 2 MB output buffer.
 */
VOID
PtEngineInitDefaultConfig(PT_TRACE_CONFIG * Config)
{
    UINT32 i;

    Config->TraceUser            = TRUE;
    Config->TraceKernel          = TRUE;
    Config->TargetCr3            = 0;
    Config->NumAddrRanges        = 0;
    Config->EnableBranch         = TRUE;
    Config->EnableTsc            = TRUE;
    Config->EnableMtc            = FALSE;
    Config->EnableCyc            = FALSE;
    Config->EnableRetCompression = FALSE;
    Config->MtcFreq              = 0;
    Config->CycThresh            = 0;
    Config->PsbFreq              = 0;
    Config->BufferSize           = PT_DEFAULT_BUFFER_SIZE;

    for (i = 0; i < PT_MAX_ADDR_RANGES; i++)
    {
        Config->AddrRanges[i].Start       = 0;
        Config->AddrRanges[i].End         = 0;
        Config->AddrRanges[i].IsStopRange = FALSE;
    }
}

/**
 * @brief Allocate the ToPA table, output buffer, and overflow zone for one
 *        per-CPU PT context, then build the ToPA entries.
 *
 *        ToPA layout:
 *          [0] main output buffer (Config->BufferSize), INT=1
 *          [1] overflow page (4 KB), INT=0
 *          [2] END, wraps back to ToPA table (circular)
 *
 * @return INT32  0 on success, -1 on allocation failure, -2 on bad config.
 */
INT32
PtEngineAllocateBuffers(PT_PER_CPU * Cpu, const PT_TRACE_CONFIG * Config)
{
    INT32           SizeEncoding;
    UINT64          BufSize;
    PT_TOPA_ENTRY * Topa;

    if (Cpu == NULL || Config == NULL)
        return -2;

    BufSize = Config->BufferSize;
    if (BufSize == 0)
        BufSize = PT_DEFAULT_BUFFER_SIZE;

    //
    // Validate buffer size is a valid ToPA encoding
    //
    SizeEncoding = PtEngineSizeToTopaEncoding(BufSize);
    if (SizeEncoding < 0)
        return -2;

    //
    // 1. ToPA table — one 4KB page, must be 4KB-aligned
    //
    Cpu->Buffer.TopaVa = (PT_TOPA_ENTRY *)PtAllocateContiguousAligned(PT_PAGE_SIZE, PT_PAGE_SIZE);
    if (Cpu->Buffer.TopaVa == NULL)
        return -1;
    Cpu->Buffer.TopaPhysical = PtVaToPa(Cpu->Buffer.TopaVa);

    //
    // 2. Main output buffer — must be aligned to its own size
    //
    Cpu->Buffer.OutputVa = PtAllocateContiguousAligned(BufSize, BufSize);
    if (Cpu->Buffer.OutputVa == NULL)
    {
        PtFreeContiguous(Cpu->Buffer.TopaVa);
        Cpu->Buffer.TopaVa = NULL;
        return -1;
    }
    Cpu->Buffer.OutputPhysical = PtVaToPa(Cpu->Buffer.OutputVa);
    Cpu->Buffer.OutputSize     = BufSize;

    //
    // 3. Overflow landing zone — 4KB
    //
    Cpu->Buffer.OverflowVa = PtAllocateContiguousAligned(PT_OVERFLOW_SIZE, PT_PAGE_SIZE);
    if (Cpu->Buffer.OverflowVa == NULL)
    {
        PtFreeContiguous(Cpu->Buffer.OutputVa);
        Cpu->Buffer.OutputVa = NULL;
        PtFreeContiguous(Cpu->Buffer.TopaVa);
        Cpu->Buffer.TopaVa = NULL;
        return -1;
    }
    Cpu->Buffer.OverflowPhysical = PtVaToPa(Cpu->Buffer.OverflowVa);

    //
    // 4. Build the ToPA table — 3 entries
    //
    Topa = Cpu->Buffer.TopaVa;

    //
    // Entry 0: main data buffer, INT=1 (trigger PMI when full)
    //
    Topa[0].Value    = 0;
    Topa[0].BaseAddr = Cpu->Buffer.OutputPhysical >> 12;
    Topa[0].Size     = (UINT64)SizeEncoding;
    Topa[0].Int      = 1;
    Topa[0].Stop     = 0;

    //
    // Entry 1: overflow landing zone (4 KB), no interrupt
    //
    Topa[1].Value    = 0;
    Topa[1].BaseAddr = Cpu->Buffer.OverflowPhysical >> 12;
    Topa[1].Size     = PT_TOPA_SIZE_4K;
    Topa[1].Int      = 0;
    Topa[1].Stop     = 0;

    //
    // Entry 2: END — circular, points back to this ToPA table
    //
    Topa[2].Value    = 0;
    Topa[2].End      = 1;
    Topa[2].BaseAddr = Cpu->Buffer.TopaPhysical >> 12;

    //
    // Copy config and initialize state
    //
    Cpu->Config             = *Config;
    Cpu->Config.BufferSize  = BufSize;
    Cpu->SavedCtl.Value     = 0;
    Cpu->State              = PT_STATE_READY;
    Cpu->TotalBytesCaptured = 0;

    return 0;
}

/**
 * @brief Free all PT buffers belonging to one per-CPU context.
 *        Must not be called while State == PT_STATE_TRACING.
 */
VOID
PtEngineFreeBuffers(PT_PER_CPU * Cpu)
{
    if (Cpu == NULL)
        return;

    //
    // Refuse to free while tracing — caller must stop first
    //
    if (Cpu->State == PT_STATE_TRACING)
        return;

    if (Cpu->Buffer.OverflowVa)
    {
        PtFreeContiguous(Cpu->Buffer.OverflowVa);
        Cpu->Buffer.OverflowVa       = NULL;
        Cpu->Buffer.OverflowPhysical = 0;
    }
    if (Cpu->Buffer.OutputVa)
    {
        PtFreeContiguous(Cpu->Buffer.OutputVa);
        Cpu->Buffer.OutputVa       = NULL;
        Cpu->Buffer.OutputPhysical = 0;
        Cpu->Buffer.OutputSize     = 0;
    }
    if (Cpu->Buffer.TopaVa)
    {
        PtFreeContiguous(Cpu->Buffer.TopaVa);
        Cpu->Buffer.TopaVa       = NULL;
        Cpu->Buffer.TopaPhysical = 0;
    }

    Cpu->State              = PT_STATE_DISABLED;
    Cpu->TotalBytesCaptured = 0;
    Cpu->SavedCtl.Value     = 0;
}

/**
 * @brief Build IA32_RTIT_CTL from a config + capabilities.
 *        Does not set TraceEn; caller is responsible for enabling last.
 */
static PT_RTIT_CTL_REGISTER
PtEngineBuildCtlFromConfig(const PT_TRACE_CONFIG * Cfg,
                           const PT_CAPABILITIES * Caps)
{
    PT_RTIT_CTL_REGISTER Ctl;
    Ctl.Value = 0;

    //
    // Core settings
    //
    Ctl.TraceEn  = 0; /* enabled last, after everything is set */
    Ctl.ToPA     = 1; /* always use ToPA output                */
    Ctl.FabricEn = 0; /* always output to memory               */

    //
    // Privilege filter
    //
    Ctl.Os   = Cfg->TraceKernel ? 1 : 0;
    Ctl.User = Cfg->TraceUser ? 1 : 0;

    //
    // CR3 filtering
    //
    Ctl.Cr3Filter = (Cfg->TargetCr3 != 0) ? 1 : 0;

    //
    // Branch tracing — the core functionality
    //
    Ctl.BranchEn = Cfg->EnableBranch ? 1 : 0;

    //
    // Timing packets
    //
    Ctl.TscEn = Cfg->EnableTsc ? 1 : 0;

    if (Caps->MtcSupport)
    {
        Ctl.MtcEn = Cfg->EnableMtc ? 1 : 0;
        if (Cfg->EnableMtc && ((1 << Cfg->MtcFreq) & Caps->MtcPeriodBitmap))
            Ctl.MtcFreq = Cfg->MtcFreq;
    }

    if (Caps->PsbCycConfigurable)
    {
        Ctl.CycEn = Cfg->EnableCyc ? 1 : 0;
        if (Cfg->EnableCyc && ((1 << Cfg->CycThresh) & Caps->CycThresholdBitmap))
            Ctl.CycThresh = Cfg->CycThresh;
        if ((1 << Cfg->PsbFreq) & Caps->PsbFreqBitmap)
            Ctl.PsbFreq = Cfg->PsbFreq;
    }

    //
    // RET compression
    //
    Ctl.DisRetc = Cfg->EnableRetCompression ? 0 : 1;

    //
    // IP filter range configuration
    //
    Ctl.Addr0Cfg = 0;
    Ctl.Addr1Cfg = 0;
    Ctl.Addr2Cfg = 0;
    Ctl.Addr3Cfg = 0;

    if (Cfg->NumAddrRanges > 0 && Caps->IpFiltering)
    {
        if (Cfg->NumAddrRanges >= 1 && Caps->NumAddrRanges >= 1)
            Ctl.Addr0Cfg = Cfg->AddrRanges[0].IsStopRange ? 2 : 1;
        if (Cfg->NumAddrRanges >= 2 && Caps->NumAddrRanges >= 2)
            Ctl.Addr1Cfg = Cfg->AddrRanges[1].IsStopRange ? 2 : 1;
        if (Cfg->NumAddrRanges >= 3 && Caps->NumAddrRanges >= 3)
            Ctl.Addr2Cfg = Cfg->AddrRanges[2].IsStopRange ? 2 : 1;
        if (Cfg->NumAddrRanges >= 4 && Caps->NumAddrRanges >= 4)
            Ctl.Addr3Cfg = Cfg->AddrRanges[3].IsStopRange ? 2 : 1;
    }

    return Ctl;
}

/**
 * @brief Start tracing on the CURRENT CPU using the passed PT_PER_CPU.
 *        Programs all PT MSRs and sets TraceEn=1.
 *
 *        Must be called from the target CPU (DPC or VMX root).
 *
 * @return INT32  0 on success, -1 on error.
 */
INT32
PtEngineStart(PT_PER_CPU * Cpu)
{
    PT_RTIT_CTL_REGISTER Ctl;
    PT_CAPABILITIES      Caps;
    UINT32               i;
    BOOLEAN              IsOnVmxRootMode = FALSE;

    if (Cpu == NULL)
        return -1;
    if (Cpu->State != PT_STATE_READY && Cpu->State != PT_STATE_STOPPED)
        return -1;
    if (Cpu->Buffer.TopaVa == NULL || Cpu->Buffer.OutputVa == NULL)
        return -1;

    if (PtEngineQueryCapabilities(&Caps) != 0)
        return -1;

    //
    // Detect VMX-root context if running under HyperDbg's hypervisor.
    //
    // Intel PT MSRs (IA32_RTIT_*) are NOT in the default MSR bitmap, so a
    // direct WRMSR to them in either VMX root or VMX non-root passes
    // through to hardware without trapping. This means the same MSR
    // sequence below works correctly from a DPC running in the guest as
    // well as from a VMX-root packet handler. We still read the mode for
    // logging and for any future hypervisor-only code (e.g. setting the
    // "Clear IA32_RTIT_CTL on VM-exit" VMCS control or saving guest-side
    // RTIT state across VM transitions).
    //
    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();
        LogInfo("PT: PtEngineStart on core %u (vmx-root=%u)\n",
                KeGetCurrentProcessorNumberEx(NULL),
                (UINT32)IsOnVmxRootMode);
    }

    //
    // Don't request more IP ranges or features than the CPU supports
    //
    if (Cpu->Config.NumAddrRanges > Caps.NumAddrRanges)
        return -1;
    if (Cpu->Config.NumAddrRanges > 0 && !Caps.IpFiltering)
        return -1;
    if (Cpu->Config.TargetCr3 != 0 && !Caps.Cr3Filtering)
        return -1;

    //
    // Step 1: Ensure tracing is off and clear status
    //
    {
        PT_RTIT_CTL_REGISTER Cur;
        Cur.Value = __readmsr(MSR_IA32_RTIT_CTL);
        if (Cur.TraceEn)
        {
            Cur.TraceEn = 0;
            __writemsr(MSR_IA32_RTIT_CTL, Cur.Value);
        }
    }

    __writemsr(MSR_IA32_RTIT_STATUS, 0);

    //
    // Step 2: Set output base and reset output position
    //
    __writemsr(MSR_IA32_RTIT_OUTPUT_BASE, Cpu->Buffer.TopaPhysical);
    {
        PT_OUTPUT_MASK_PTRS_REGISTER InitMask;
        InitMask.Value             = 0;
        InitMask.LowerMask         = 0x7F;
        InitMask.MaskOrTableOffset = 0;
        InitMask.OutputOffset      = 0;
        __writemsr(MSR_IA32_RTIT_OUTPUT_MASK_PTRS, InitMask.Value);
    }

    //
    // Step 3: Set CR3 filter
    //
    __writemsr(MSR_IA32_RTIT_CR3_MATCH, Cpu->Config.TargetCr3);

    //
    // Step 4: Set IP address ranges. IMPORTANT: only touch ADDRn MSRs the
    // CPU actually supports; writing to non-existent ADDRn MSRs causes #GP.
    //
    for (i = 0; i < Caps.NumAddrRanges; i++)
    {
        UINT32 MsrA = MSR_IA32_RTIT_ADDR0_A + (i * 2);
        UINT32 MsrB = MSR_IA32_RTIT_ADDR0_B + (i * 2);

        if (i < Cpu->Config.NumAddrRanges)
        {
            __writemsr(MsrA, Cpu->Config.AddrRanges[i].Start);
            __writemsr(MsrB, Cpu->Config.AddrRanges[i].End);
        }
        else
        {
            __writemsr(MsrA, 0);
            __writemsr(MsrB, 0);
        }
    }

    //
    // Step 5: Build IA32_RTIT_CTL and enable
    //
    Ctl = PtEngineBuildCtlFromConfig(&Cpu->Config, &Caps);

    //
    // Save the CTL value for resume
    //
    Cpu->SavedCtl = Ctl;

    //
    // Enable tracing — MUST be the last MSR write
    //
    Ctl.TraceEn           = 1;
    Cpu->SavedCtl.TraceEn = 1;
    __writemsr(MSR_IA32_RTIT_CTL, Ctl.Value);

    //
    // Step 6: Verify tracing started
    //
    {
        PT_RTIT_STATUS_REGISTER Status;
        Status.Value = __readmsr(MSR_IA32_RTIT_STATUS);
        if (Status.Error)
        {
            Ctl.TraceEn = 0;
            __writemsr(MSR_IA32_RTIT_CTL, Ctl.Value);
            Cpu->State = PT_STATE_ERROR;
            return -1;
        }
    }

    Cpu->State = PT_STATE_TRACING;
    return 0;
}

/**
 * @brief Read current output position, calculate bytes written, and
 *        optionally copy trace data into Out starting at Out->WriteOffset,
 *        then advance WriteOffset.
 */
static UINT64
PtEngineReadBytesWritten(const PT_BUFFER * Buf, PT_OUTPUT_BUFFER * Out)
{
    PT_OUTPUT_MASK_PTRS_REGISTER Mask;
    UINT32                       TopaIndex;
    UINT32                       BytesInEntry;
    UINT64                       Total = 0;

    Mask.Value = __readmsr(MSR_IA32_RTIT_OUTPUT_MASK_PTRS);

    TopaIndex    = (UINT32)(Mask.MaskOrTableOffset >> 0);
    BytesInEntry = (UINT32)Mask.OutputOffset;

    //
    // ToPA layout:
    //   Entry[0] = main buffer (OutputSize bytes)
    //   Entry[1] = overflow (4KB)
    //   Entry[2] = END
    //
    if (TopaIndex == 0)
    {
        Total = BytesInEntry;
    }
    else if (TopaIndex >= 1)
    {
        Total = Buf->OutputSize + BytesInEntry;
    }

    //
    // Copy trace data if the caller provided a buffer with enough room
    //
    if (Out != NULL && Out->Buffer != NULL && Total > 0)
    {
        if (Out->WriteOffset + Total <= Out->Length)
        {
            UINT8 * Dest       = (UINT8 *)Out->Buffer + Out->WriteOffset;
            UINT64  MainBytes  = (Total < Buf->OutputSize) ? Total : Buf->OutputSize;
            UINT64  SpillBytes = (Total > Buf->OutputSize) ? (Total - Buf->OutputSize) : 0;

            RtlCopyMemory(Dest, Buf->OutputVa, (SIZE_T)MainBytes);

            if (SpillBytes > 0)
                RtlCopyMemory(Dest + MainBytes, Buf->OverflowVa, (SIZE_T)SpillBytes);

            Out->WriteOffset += Total;
        }
        // else: not enough space — skip copy, WriteOffset unchanged
    }

    return Total;
}

/**
 * @brief Stop tracing on the CURRENT CPU. Reads final output position,
 *        copies trace data if requested, resets PT MSRs.
 *
 * @return UINT64  Total bytes captured (across all PMIs + final), or 0 on error.
 */
UINT64
PtEngineStop(PT_PER_CPU * Cpu, PT_OUTPUT_BUFFER * Out)
{
    UINT64  BytesThisRun;
    UINT32  i;
    BOOLEAN IsOnVmxRootMode = FALSE;

    if (Cpu == NULL)
        return 0;
    if (Cpu->State != PT_STATE_TRACING && Cpu->State != PT_STATE_PAUSED)
        return 0;

    //
    // Detect VMX-root context for symmetry with PtEngineStart. Direct WRMSR
    // works in both modes for PT MSRs (not in MSR bitmap by default).
    //
    if (g_RunningOnHypervisorEnvironment)
    {
        IsOnVmxRootMode = g_Callbacks.VmFuncVmxGetCurrentExecutionMode();
        LogInfo("PT: PtEngineStop on core %u (vmx-root=%u)\n",
                KeGetCurrentProcessorNumberEx(NULL),
                (UINT32)IsOnVmxRootMode);
    }

    //
    // Disable tracing
    //
    {
        PT_RTIT_CTL_REGISTER Ctl;
        Ctl.Value   = __readmsr(MSR_IA32_RTIT_CTL);
        Ctl.TraceEn = 0;
        __writemsr(MSR_IA32_RTIT_CTL, Ctl.Value);
    }

    //
    // Read final output position and copy data if requested
    //
    BytesThisRun = PtEngineReadBytesWritten(&Cpu->Buffer, Out);
    Cpu->TotalBytesCaptured += BytesThisRun;

    //
    // Check for hardware error
    //
    {
        PT_RTIT_STATUS_REGISTER Status;
        Status.Value = __readmsr(MSR_IA32_RTIT_STATUS);
        if (Status.Error)
        {
            Cpu->State = PT_STATE_ERROR;
            return 0;
        }
    }

    //
    // Reset all PT MSRs
    //
    __writemsr(MSR_IA32_RTIT_CTL, 0);
    __writemsr(MSR_IA32_RTIT_STATUS, 0);
    __writemsr(MSR_IA32_RTIT_OUTPUT_BASE, 0);
    __writemsr(MSR_IA32_RTIT_OUTPUT_MASK_PTRS, 0);
    __writemsr(MSR_IA32_RTIT_CR3_MATCH, 0);

    //
    // Clear all ADDRn MSRs that this CPU supports.
    // NumAddrRanges was validated against caps in PtEngineStart.
    //
    for (i = 0; i < Cpu->Config.NumAddrRanges; i++)
    {
        __writemsr(MSR_IA32_RTIT_ADDR0_A + (i * 2), 0);
        __writemsr(MSR_IA32_RTIT_ADDR0_B + (i * 2), 0);
    }

    Cpu->State = PT_STATE_STOPPED;
    return Cpu->TotalBytesCaptured;
}

/**
 * @brief Pause tracing on the CURRENT CPU. Preserves buffer state.
 */
INT32
PtEnginePause(PT_PER_CPU * Cpu)
{
    PT_RTIT_CTL_REGISTER Ctl;

    if (Cpu == NULL || Cpu->State != PT_STATE_TRACING)
        return -1;

    Ctl.Value   = __readmsr(MSR_IA32_RTIT_CTL);
    Ctl.TraceEn = 0;
    __writemsr(MSR_IA32_RTIT_CTL, Ctl.Value);

    Cpu->State = PT_STATE_PAUSED;
    return 0;
}

/**
 * @brief Resume tracing on the CURRENT CPU after pause.
 */
INT32
PtEngineResume(PT_PER_CPU * Cpu)
{
    PT_RTIT_CTL_REGISTER Ctl;

    if (Cpu == NULL || Cpu->State != PT_STATE_PAUSED)
        return -1;

    Ctl         = Cpu->SavedCtl;
    Ctl.TraceEn = 1;
    __writemsr(MSR_IA32_RTIT_CTL, Ctl.Value);

    Cpu->State = PT_STATE_TRACING;
    return 0;
}

/**
 * @brief Check whether the latest PMI was raised by Intel PT
 *        (IA32_PERF_GLOBAL_STATUS bit 55).
 */
BOOLEAN
PtEngineIsPtPmi()
{
    UINT64 GlobalStatus = __readmsr(MSR_IA32_PERF_GLOBAL_STATUS);
    return (GlobalStatus & PERF_GLOBAL_STATUS_TOPA_PMI) ? TRUE : FALSE;
}

/**
 * @brief Handle a ToPA PMI on the CURRENT CPU. Caller is responsible for
 *        having already disabled tracing (e.g. via VMCS clear of RTIT_CTL).
 *
 * @return UINT64 number of bytes captured in this PMI event.
 */
UINT64
PtEngineHandlePmi(PT_PER_CPU * Cpu, PT_OUTPUT_BUFFER * Out)
{
    UINT64 BytesThisEvent;

    if (Cpu == NULL)
        return 0;

    //
    // 1. Read how many bytes were written and copy them out
    //
    BytesThisEvent = PtEngineReadBytesWritten(&Cpu->Buffer, Out);
    Cpu->TotalBytesCaptured += BytesThisEvent;

    //
    // 2. Reset output position to start of buffer BEFORE acknowledging the
    //    PMI. PT keeps tracing during the handler (TraceEn stays 1) and the
    //    hardware is free to raise another ToPA PMI as soon as PendTopaPmi /
    //    PERF_GLOBAL_STATUS.TopaPmi are cleared. If MASK_PTRS still points
    //    deep into the overflow page (or past wrap) at that moment, a
    //    back-to-back PMI would observe stale state.
    //
    {
        PT_OUTPUT_MASK_PTRS_REGISTER ResetMask;
        ResetMask.Value             = 0;
        ResetMask.LowerMask         = 0x7F;
        ResetMask.MaskOrTableOffset = 0;
        ResetMask.OutputOffset      = 0;
        __writemsr(MSR_IA32_RTIT_OUTPUT_MASK_PTRS, ResetMask.Value);
    }

    //
    // 3. Clear PT pending PMI bits in IA32_RTIT_STATUS
    //
    {
        PT_RTIT_STATUS_REGISTER Status;
        Status.Value       = __readmsr(MSR_IA32_RTIT_STATUS);
        Status.PendTopaPmi = 0;
        Status.PendPsbPmi  = 0;
        Status.Error       = 0;
        Status.Stopped     = 0;
        __writemsr(MSR_IA32_RTIT_STATUS, Status.Value);
    }

    //
    // 4. Acknowledge the global PMI bit
    //
    __writemsr(MSR_IA32_PERF_GLOBAL_OVF_CTRL, PERF_GLOBAL_STATUS_TOPA_PMI);

    return BytesThisEvent;
}

//////////////////////////////////////////////////
//          HyperDbg-style wrappers             //
//      (mirror of Lbr.c API surface)           //
//////////////////////////////////////////////////

/**
 * @brief Check whether Intel PT is supported on the current CPU.
 *        Mirrors LbrCheck — must be called once before any Pt* operation.
 *
 *        If running under HyperDbg's hypervisor we also verify that the
 *        platform's IA32_VMX_MISC MSR advertises PT-in-VMX support, since
 *        without that bit set we may not be able to keep PT alive across
 *        VM transitions in a future VMCS-controlled implementation. This
 *        is a soft check — the current direct-MSR path still works because
 *        PT MSRs aren't trapped — but it surfaces the situation in the log.
 *
 * @return BOOLEAN  TRUE if Intel PT is available.
 */
BOOLEAN
PtCheck()
{
    PT_CAPABILITIES Caps = {0};

    if (PtEngineQueryCapabilities(&Caps) != 0)
        return FALSE;

    //
    // We require ToPA output for our buffer scheme
    //
    if (!Caps.TopaOutput)
    {
        LogInfo("PT: CPU does not support ToPA output; PT cannot be used here.\n");
        return FALSE;
    }

    if (g_RunningOnHypervisorEnvironment && !Caps.VmxSupport)
    {
        LogInfo("PT: IA32_VMX_MISC[14] is clear — Intel PT in VMX is not "
                "advertised on this CPU. Direct MSR programming still works "
                "from VMX non-root because PT MSRs are not trapped.\n");
    }

    return TRUE;
}

/**
 * @brief Allocate ToPA / output / overflow buffers for every active CPU.
 *
 *        Must be called at IRQL == PASSIVE_LEVEL (before broadcasting the
 *        per-core enable DPC), because MmAllocateContiguousMemorySpecifyCache
 *        is paged.
 *
 *        Idempotent: cores that already have buffers (State != DISABLED)
 *        are skipped.
 *
 * @return BOOLEAN  TRUE if every core ended up with a usable buffer set.
 */
BOOLEAN
PtAllocateAllCpuBuffers()
{
    UINT32 ProcessorsCount;
    UINT32 i;

    if (g_PtStateList == NULL)
        return FALSE;

    ProcessorsCount = KeQueryActiveProcessorCount(0);

    for (i = 0; i < ProcessorsCount; i++)
    {
        PT_PER_CPU *    Cpu = &g_PtStateList[i];
        PT_TRACE_CONFIG Cfg = Cpu->Config;

        if (Cpu->State != PT_STATE_DISABLED)
            continue;

        if (Cfg.BufferSize == 0)
            PtEngineInitDefaultConfig(&Cfg);

        if (PtEngineAllocateBuffers(Cpu, &Cfg) != 0)
        {
            LogInfo("PT: buffer allocation failed on core %u\n", i);
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Free ToPA / output / overflow buffers for every active CPU.
 *
 *        Must be called at IRQL == PASSIVE_LEVEL (after broadcasting any
 *        per-core disable DPC), because MmFreeContiguousMemory is paged.
 */
VOID
PtFreeAllCpuBuffers()
{
    UINT32 ProcessorsCount;
    UINT32 i;

    if (g_PtStateList == NULL)
        return;

    //
    // Drop any live user mappings before the underlying contiguous
    // memory goes away. Must run in the same process that called the
    // mmap IOCTL — see HYPERTRACE_PT_MMAP_PACKETS for the contract.
    //
    PtUnmapAllCpuBuffersFromUser();

    ProcessorsCount = KeQueryActiveProcessorCount(0);

    for (i = 0; i < ProcessorsCount; i++)
    {
        PtEngineFreeBuffers(&g_PtStateList[i]);
    }
}

/**
 * @brief Map every per-CPU PT main output buffer and 4 KB overflow page
 *        into the current user process as a single virtually contiguous
 *        region per CPU, and fill OutDescs[i] with the base UserVa and
 *        the total Size (main + overflow) for that CPU.
 *
 *        PT buffers must already exist (PtAllocateAllCpuBuffers must
 *        have run, i.e. PT is enabled). Idempotent within an enable
 *        cycle: a second call returns the already-cached mappings. On
 *        any per-CPU failure the partial work is rolled back and the
 *        function returns -1.
 *
 * @return INT32  0 on success, -1 on failure.
 */
INT32
PtMmapAllCpuBuffersToUser(PT_USER_BUFFER_DESC * OutDescs, UINT32 MaxDescs, UINT32 * OutNumCpus)
{
    UINT32 ProcessorsCount;
    UINT32 i;

    if (OutDescs == NULL || OutNumCpus == NULL || g_PtStateList == NULL)
        return -1;

    ProcessorsCount = KeQueryActiveProcessorCount(0);
    if (ProcessorsCount == 0 || ProcessorsCount > MaxDescs || ProcessorsCount > PT_MAX_CPUS_FOR_MMAP)
        return -1;

    if (!g_PtUserMappingsActive)
    {
        for (i = 0; i < ProcessorsCount; i++)
        {
            PT_PER_CPU * Cpu = &g_PtStateList[i];

            if (Cpu->Buffer.OutputVa == NULL || Cpu->Buffer.OverflowVa == NULL)
            {
                PtUnmapAllCpuBuffersFromUser();
                return -1;
            }

            if (PtMmapCpuRegionToUser(Cpu->Buffer.OutputVa,
                                      Cpu->Buffer.OutputPhysical,
                                      (SIZE_T)Cpu->Buffer.OutputSize,
                                      Cpu->Buffer.OverflowPhysical,
                                      (SIZE_T)PT_OVERFLOW_SIZE,
                                      &g_PtUserMappings[i].Mdl,
                                      &g_PtUserMappings[i].UserVa) != 0)
            {
                PtUnmapAllCpuBuffersFromUser();
                return -1;
            }
        }

        g_PtUserMappingsActive = TRUE;
    }

    for (i = 0; i < ProcessorsCount; i++)
    {
        OutDescs[i].CpuId    = i;
        OutDescs[i].Reserved = 0;
        OutDescs[i].UserVa   = (UINT64)(ULONG_PTR)g_PtUserMappings[i].UserVa;
        OutDescs[i].Size     = g_PtStateList[i].Buffer.OutputSize + PT_OVERFLOW_SIZE;
    }

    *OutNumCpus = ProcessorsCount;
    return 0;
}

/**
 * @brief Release every user mapping created by PtMmapAllCpuBuffersToUser.
 *        Called by PtFreeAllCpuBuffers (i.e. on PT disable / flush) so
 *        user VAs stop being usable before the backing memory is freed.
 *        Also used as a rollback path on partial mmap failure.
 *
 *        Always walks the full table (PtUnmapCpuRegionFromUser is
 *        NULL-safe) so rollback after a half-finished mapping still
 *        cleans up the CPUs that were mapped before the failure.
 */
VOID
PtUnmapAllCpuBuffersFromUser()
{
    UINT32 i;

    for (i = 0; i < PT_MAX_CPUS_FOR_MMAP; i++)
    {
        PtUnmapCpuRegionFromUser(g_PtUserMappings[i].Mdl,
                                 g_PtUserMappings[i].UserVa);
        g_PtUserMappings[i].Mdl    = NULL;
        g_PtUserMappings[i].UserVa = NULL;
    }

    g_PtUserMappingsActive = FALSE;
}

/**
 * @brief Start PT tracing on the CURRENT CPU. Buffers must already be
 *        allocated by PtAllocateAllCpuBuffers (called at PASSIVE_LEVEL).
 *
 *        Mirrors LbrStart but takes no parameters: per-CPU configuration is
 *        sourced from g_PtStateList[core].Config (defaulted at init time).
 *
 * @return BOOLEAN  TRUE on success.
 */
BOOLEAN
PtStart()
{
    UINT32       CurrentCore;
    PT_PER_CPU * Cpu;

    if (g_PtStateList == NULL)
    {
        LogInfo("PT: per-CPU state not initialized.\n");
        return FALSE;
    }

    CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    Cpu         = &g_PtStateList[CurrentCore];

    if (Cpu->State == PT_STATE_DISABLED)
    {
        //
        // Buffers should have been allocated at PASSIVE_LEVEL beforehand.
        // Allocating from a DPC is unsafe (MmAllocateContiguousMemory* is
        // a paged routine) so just bail out.
        //
        LogInfo("PT: buffers not allocated for core %u\n", CurrentCore);
        return FALSE;
    }

    if (PtEngineStart(Cpu) != 0)
    {
        LogInfo("PT: PtEngineStart failed on core %u (state=%d)\n", CurrentCore, Cpu->State);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief Stop PT tracing on the CURRENT CPU.
 *        Trace data accumulated in the per-CPU output buffer is left in
 *        place; PtSize / PtDump can read it later.
 */
VOID
PtStop()
{
    UINT32       CurrentCore;
    PT_PER_CPU * Cpu;

    if (g_PtStateList == NULL)
        return;

    CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    Cpu         = &g_PtStateList[CurrentCore];

    LogInfo("PT: stopping trace on core %d\n", CurrentCore);

    PtEngineStop(Cpu, NULL);
}

/**
 * @brief Pause PT tracing on the CURRENT CPU. Buffer state is preserved
 *        so a subsequent PtResume picks up where this left off.
 */
VOID
PtPause()
{
    UINT32       CurrentCore;
    PT_PER_CPU * Cpu;

    if (g_PtStateList == NULL)
        return;

    CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    Cpu         = &g_PtStateList[CurrentCore];

    LogInfo("PT: pausing trace on core %u\n", CurrentCore);

    PtEnginePause(Cpu);
}

/**
 * @brief Resume PT tracing on the CURRENT CPU after a prior PtPause.
 */
VOID
PtResume()
{
    UINT32       CurrentCore;
    PT_PER_CPU * Cpu;

    if (g_PtStateList == NULL)
        return;

    CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    Cpu         = &g_PtStateList[CurrentCore];

    LogInfo("PT: resuming trace on core %u\n", CurrentCore);

    PtEngineResume(Cpu);
}

/**
 * @brief Snapshot the current PT output position on the CURRENT CPU
 *        without disturbing tracing state. The returned value is the
 *        number of bytes of valid trace data sitting in this CPU's
 *        main + overflow buffer, i.e. the offset a decoder should stop
 *        at when reading from the user mapping.
 */
UINT64
PtSize()
{
    UINT32                       CurrentCore;
    PT_PER_CPU *                 Cpu;
    PT_OUTPUT_MASK_PTRS_REGISTER Mask;
    UINT32                       TopaIndex;
    UINT32                       BytesInEntry;

    if (g_PtStateList == NULL)
        return 0;

    CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    Cpu         = &g_PtStateList[CurrentCore];

    if (Cpu->State != PT_STATE_TRACING && Cpu->State != PT_STATE_PAUSED && Cpu->State != PT_STATE_STOPPED)
        return 0;

    //
    // Read MASK_PTRS without touching MSRs that would disturb tracing.
    // For an active trace this gives the live byte count; for stopped
    // traces it returns the position at the moment tracing was disabled.
    //
    Mask.Value   = __readmsr(MSR_IA32_RTIT_OUTPUT_MASK_PTRS);
    TopaIndex    = (UINT32)Mask.MaskOrTableOffset;
    BytesInEntry = (UINT32)Mask.OutputOffset;

    if (TopaIndex == 0)
        return BytesInEntry;

    return Cpu->Buffer.OutputSize + BytesInEntry;
}

/**
 * @brief Print PT trace summary for the CURRENT CPU.
 *
 *        PT packets are a compressed binary stream that requires a decoder
 *        (libipt or similar) to be human-readable, so this only emits the
 *        per-CPU statistics; bulk packet data is preserved in the output
 *        buffer for offline retrieval.
 */
VOID
PtDump()
{
    UINT32       CurrentCore;
    PT_PER_CPU * Cpu;

    if (g_PtStateList == NULL)
        return;

    CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    Cpu         = &g_PtStateList[CurrentCore];

    Log("PT trace summary for core %u\n", CurrentCore);
    Log("  State                : %d\n", Cpu->State);
    Log("  TotalBytesCaptured   : 0x%llx\n", Cpu->TotalBytesCaptured);
    Log("  OutputBufferSize     : 0x%llx\n", Cpu->Buffer.OutputSize);
    Log("  TopaPhysical         : 0x%llx\n", Cpu->Buffer.TopaPhysical);
    Log("  OutputPhysical       : 0x%llx\n", Cpu->Buffer.OutputPhysical);
    Log("  TraceUser/Kernel     : %u / %u\n", Cpu->Config.TraceUser, Cpu->Config.TraceKernel);
    Log("  TargetCr3            : 0x%llx\n", Cpu->Config.TargetCr3);
}

/**
 * @brief LBR-style filter wrapper: refresh tracing on the CURRENT CPU with
 *        a fresh PT_FILTER_OPTIONS.
 *
 *        Mirrors LbrFilter — the caller hands in only the fields a user is
 *        allowed to drive (TraceUser, TraceKernel, TargetCr3, BufferSize,
 *        NumAddrRanges, AddrRanges) and PtFilter writes them into the
 *        per-CPU PT_TRACE_CONFIG one at a time. Engine-managed options
 *        (BranchEn, TscEn, MtcEn, CycEn, RetCompression, *Freq) are left
 *        alone, so a filter call can never accidentally turn off the
 *        packet types the engine relies on.
 *
 *        BufferSize == 0 keeps the per-CPU value already in place, so
 *        pure filter changes (privilege bits, CR3, IP ranges) skip any
 *        reallocation of the ToPA / output / overflow buffers and can
 *        run entirely from a DPC. Genuine buffer-size changes still need
 *        a PASSIVE_LEVEL caller to free + reallocate; HyperTracePtFilter
 *        handles that case before broadcasting.
 */
VOID
PtFilter(const PT_FILTER_OPTIONS * FilterOptions)
{
    UINT32          CurrentCore;
    PT_PER_CPU *    Cpu;
    UINT32          i;
    PT_CAPABILITIES Caps = {0};

    if (g_PtStateList == NULL || FilterOptions == NULL)
        return;

    if (PtEngineQueryCapabilities(&Caps) != 0)
        return;

    CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    Cpu         = &g_PtStateList[CurrentCore];

    LogInfo("PT: applying filter on core %u\n", CurrentCore);

    //
    // Stop tracing on this CPU first so we can safely mutate Cpu->Config
    // and reprogram the RTIT_CTL bits.
    //
    if (Cpu->State == PT_STATE_TRACING || Cpu->State == PT_STATE_PAUSED)
    {
        PtEngineStop(Cpu, NULL);
    }

    //
    // Apply only the user-tunable fields to this CPU's per-CPU config.
    //
    Cpu->Config.TraceUser     = FilterOptions->TraceUser;
    Cpu->Config.TraceKernel   = FilterOptions->TraceKernel;

    if (FilterOptions->TargetCr3 != 0 && !Caps.Cr3Filtering)
    {
        LogInfo("PT: CR3 filtering requested but not supported by CPU\n");
        Cpu->Config.TargetCr3 = 0;
    }
    else
    {
        Cpu->Config.TargetCr3 = FilterOptions->TargetCr3;
    }

    if (FilterOptions->NumAddrRanges > Caps.NumAddrRanges)
    {
        LogInfo("PT: requested %u IP filter ranges, but CPU only supports %u\n", FilterOptions->NumAddrRanges, Caps.NumAddrRanges);
        Cpu->Config.NumAddrRanges = Caps.NumAddrRanges;
    }
    else if (FilterOptions->NumAddrRanges > 0 && !Caps.IpFiltering)
    {
        LogInfo("PT: IP filtering requested but not supported by CPU\n");
        Cpu->Config.NumAddrRanges = 0;
    }
    else
    {
        Cpu->Config.NumAddrRanges = FilterOptions->NumAddrRanges;
    }

    if (FilterOptions->BufferSize != 0)
    {
        Cpu->Config.BufferSize = FilterOptions->BufferSize;
    }

    for (i = 0; i < PT_MAX_ADDR_RANGES; i++)
    {
        Cpu->Config.AddrRanges[i] = FilterOptions->AddrRanges[i];
    }

    //
    // If the per-CPU buffers haven't been allocated yet, leave the slot
    // configured so the next PtStart picks it up — skip starting here
    // because PtEngineStart needs ToPA / output / overflow already
    // allocated at PASSIVE_LEVEL.
    //
    if (Cpu->State == PT_STATE_DISABLED)
        return;

    PtEngineStart(Cpu);
}

/**
 * @brief Flush PT trace state on the CURRENT CPU — disables tracing and
 *        clears the bytes-captured counter so the next PtStart begins from
 *        a fresh baseline. Buffer freeing happens at PASSIVE_LEVEL via
 *        PtFreeAllCpuBuffers; this is safe to call from a DPC.
 *
 *        Mirrors LbrFlush.
 */
VOID
PtFlush()
{
    UINT32       CurrentCore;
    PT_PER_CPU * Cpu;

    if (g_PtStateList == NULL)
        return;

    CurrentCore = KeGetCurrentProcessorNumberEx(NULL);
    Cpu         = &g_PtStateList[CurrentCore];

    LogInfo("PT: flush on core %u\n", CurrentCore);

    if (Cpu->State == PT_STATE_TRACING || Cpu->State == PT_STATE_PAUSED)
    {
        PtEngineStop(Cpu, NULL);
    }

    Cpu->TotalBytesCaptured = 0;
}
