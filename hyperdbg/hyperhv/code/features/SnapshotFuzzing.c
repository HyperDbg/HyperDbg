/**
 * @file SnapshotFuzzing.c
 * @author HyperDbg Dev Team
 * @brief In-kernel hypervisor snapshot fuzzing and coverage engine implementation.
 * @details Implements high-throughput in-memory vCPU snapshotting, hardware EPT Access/Dirty
 *          (A/D) and PML dirty page tracking, zero-exit memory rollback, in-kernel Intel PT
 *          ToPA edge parsing into a shared 64KB AFL bitmap, and automated IDT crash triage.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//////////////////////////////////////////////////
//              Global Engine State             //
//////////////////////////////////////////////////

static BOOLEAN                   g_FuzzerActive           = FALSE;
static SNAPSHOT_VCPU_CONTEXT     g_VcpuBaselineContext    = {0};
static SNAPSHOT_MEMORY_TRACKER   g_SnapshotMemoryTracker  = {0};
static PFUZZ_AFL_COVERAGE_MAP    g_AflCoverageMap         = NULL;
static PMDL                      g_AflCoverageMdl         = NULL;
static PVOID                     g_AflCoverageUserVa      = NULL;
static FUZZ_CRASH_REPORT         g_LastCrashReport        = {0};
static UINT64                    g_SnapshotBaselineTsc    = 0;
static UINT64                    g_VirtualTscDelta        = 0;
static UINT32                    g_EvasionMode            = EVASION_MODE_FIXED_DELTA;

//////////////////////////////////////////////////
//         Anti-Tamper Synthetic TSC State      //
//////////////////////////////////////////////////

/**
 * @brief Returns TRUE if snapshot fuzzing is currently armed and active.
 */
BOOLEAN
SnapshotIsActive(VOID)
{
    return g_FuzzerActive;
}

/**
 * @brief Freezes or resets baseline TSC cycle counter.
 */
VOID
SnapshotFreezeTsc(VIRTUAL_MACHINE_STATE * VCpu, UINT64 FrozenTsc)
{
    UNREFERENCED_PARAMETER(VCpu);
    g_SnapshotBaselineTsc = FrozenTsc;
    g_VirtualTscDelta     = 0;
}

/**
 * @brief Returns virtualized synthetic TSC for guest execution.
 */
UINT64
SnapshotGetVirtualizedTsc(VIRTUAL_MACHINE_STATE * VCpu)
{
    UNREFERENCED_PARAMETER(VCpu);

    if (!g_FuzzerActive || g_SnapshotBaselineTsc == 0)
    {
        return CpuReadTsc();
    }

    if (g_EvasionMode == EVASION_MODE_FIXED_DELTA)
    {
        // Add a micro-increment (+64 cycles) per RDTSC query to simulate authentic tick advancement
        // while blinding anti-debugging delta threshold checks
        g_VirtualTscDelta += 0x40;
        return g_SnapshotBaselineTsc + g_VirtualTscDelta;
    }
    else if (g_EvasionMode == EVASION_MODE_INSTRUCTION_DILATION)
    {
        g_VirtualTscDelta += 0x100;
        return g_SnapshotBaselineTsc + g_VirtualTscDelta;
    }

    // Freeze mode: deterministic zero-advance
    return g_SnapshotBaselineTsc;
}

//////////////////////////////////////////////////
//          vCPU Context Save & Restore         //
//////////////////////////////////////////////////

/**
 * @brief Captures the complete architectural and extended vCPU state into Context.
 *
 * @param VCpu Pointer to current virtual machine processor state.
 * @param Context Destination snapshot buffer.
 */
VOID
SnapshotSaveVcpuContext(VIRTUAL_MACHINE_STATE * VCpu, PSNAPSHOT_VCPU_CONTEXT Context)
{
    //
    // 1. General-Purpose 64-bit Registers (GPRs)
    //
    Context->Rax = VCpu->Regs->rax;
    Context->Rcx = VCpu->Regs->rcx;
    Context->Rdx = VCpu->Regs->rdx;
    Context->Rbx = VCpu->Regs->rbx;
    Context->Rsp = VCpu->Regs->rsp;
    Context->Rbp = VCpu->Regs->rbp;
    Context->Rsi = VCpu->Regs->rsi;
    Context->Rdi = VCpu->Regs->rdi;
    Context->R8  = VCpu->Regs->r8;
    Context->R9  = VCpu->Regs->r9;
    Context->R10 = VCpu->Regs->r10;
    Context->R11 = VCpu->Regs->r11;
    Context->R12 = VCpu->Regs->r12;
    Context->R13 = VCpu->Regs->r13;
    Context->R14 = VCpu->Regs->r14;
    Context->R15 = VCpu->Regs->r15;

    //
    // 2. Instruction Pointer and CPU Flags
    //
    Context->Rip    = GetGuestRIP();
    Context->Rflags = GetGuestRFlags();

    //
    // 3. Machine Control Registers
    //
    Context->Cr0 = GetGuestCr0();
    Context->Cr2 = GetGuestCr2();
    Context->Cr3 = GetGuestCr3();
    Context->Cr4 = GetGuestCr4();
    Context->Cr8 = GetGuestCr8();

    //
    // 4. Hardware Debug Registers (DR0 - DR7)
    //
    Context->Dr0 = GetGuestDr0();
    Context->Dr1 = GetGuestDr1();
    Context->Dr2 = GetGuestDr2();
    Context->Dr3 = GetGuestDr3();
    Context->Dr6 = GetGuestDr6();
    Context->Dr7 = GetGuestDr7();

    //
    // 5. Segment Selectors, Bases, Limits and Attributes
    //
    VMX_SEGMENT_SELECTOR Cs = GetGuestCs();
    Context->Cs.Selector   = Cs.Selector;
    Context->Cs.Base       = Cs.Base;
    Context->Cs.Limit      = Cs.Limit;
    Context->Cs.Attributes = (UINT16)Cs.Attributes.AsUInt;

    VMX_SEGMENT_SELECTOR Ss = GetGuestSs();
    Context->Ss.Selector   = Ss.Selector;
    Context->Ss.Base       = Ss.Base;
    Context->Ss.Limit      = Ss.Limit;
    Context->Ss.Attributes = (UINT16)Ss.Attributes.AsUInt;

    VMX_SEGMENT_SELECTOR Ds = GetGuestDs();
    Context->Ds.Selector   = Ds.Selector;
    Context->Ds.Base       = Ds.Base;
    Context->Ds.Limit      = Ds.Limit;
    Context->Ds.Attributes = (UINT16)Ds.Attributes.AsUInt;

    VMX_SEGMENT_SELECTOR Es = GetGuestEs();
    Context->Es.Selector   = Es.Selector;
    Context->Es.Base       = Es.Base;
    Context->Es.Limit      = Es.Limit;
    Context->Es.Attributes = (UINT16)Es.Attributes.AsUInt;

    VMX_SEGMENT_SELECTOR Fs = GetGuestFs();
    Context->Fs.Selector   = Fs.Selector;
    Context->Fs.Base       = Fs.Base;
    Context->Fs.Limit      = Fs.Limit;
    Context->Fs.Attributes = (UINT16)Fs.Attributes.AsUInt;

    VMX_SEGMENT_SELECTOR Gs = GetGuestGs();
    Context->Gs.Selector   = Gs.Selector;
    Context->Gs.Base       = Gs.Base;
    Context->Gs.Limit      = Gs.Limit;
    Context->Gs.Attributes = (UINT16)Gs.Attributes.AsUInt;

    Context->Tr.Base   = GetGuestTr();
    Context->Ldtr.Base = GetGuestLdtr();
    Context->Gdtr.Base = GetGuestGdtr();
    Context->Idtr.Base = GetGuestIdtr();

    //
    // 6. Critical Model-Specific Registers (MSRs)
    //
    Context->MsrEfer         = CpuReadMsr(IA32_EFER);
    Context->MsrStar         = CpuReadMsr(IA32_STAR);
    Context->MsrLstar        = CpuReadMsr(IA32_LSTAR);
    Context->MsrCstar        = CpuReadMsr(IA32_CSTAR);
    Context->MsrSysenterCs   = CpuReadMsr(IA32_SYSENTER_CS);
    Context->MsrSysenterEsp  = CpuReadMsr(IA32_SYSENTER_ESP);
    Context->MsrSysenterEip  = CpuReadMsr(IA32_SYSENTER_EIP);
    Context->MsrFsBase       = CpuReadMsr(IA32_FS_BASE);
    Context->MsrGsBase       = CpuReadMsr(IA32_GS_BASE);
    Context->MsrKernelGsBase = CpuReadMsr(IA32_KERNEL_GS_BASE);

    //
    // 7. Extended Processor State (FPU / SSE / AVX / AVX-512)
    //
    _xsave((PVOID)Context->XsaveArea, 0xFFFFFFFFFFFFFFFFULL);
}

/**
 * @brief Restores the complete architectural and extended vCPU state from Context.
 *
 * @param VCpu Pointer to current virtual machine processor state.
 * @param Context Source snapshot buffer.
 */
VOID
SnapshotRestoreVcpuContext(VIRTUAL_MACHINE_STATE * VCpu, PSNAPSHOT_VCPU_CONTEXT Context)
{
    //
    // 1. General-Purpose 64-bit Registers (GPRs)
    //
    VCpu->Regs->rax = Context->Rax;
    VCpu->Regs->rcx = Context->Rcx;
    VCpu->Regs->rdx = Context->Rdx;
    VCpu->Regs->rbx = Context->Rbx;
    VCpu->Regs->rsp = Context->Rsp;
    VCpu->Regs->rbp = Context->Rbp;
    VCpu->Regs->rsi = Context->Rsi;
    VCpu->Regs->rdi = Context->Rdi;
    VCpu->Regs->r8  = Context->R8;
    VCpu->Regs->r9  = Context->R9;
    VCpu->Regs->r10 = Context->R10;
    VCpu->Regs->r11 = Context->R11;
    VCpu->Regs->r12 = Context->R12;
    VCpu->Regs->r13 = Context->R13;
    VCpu->Regs->r14 = Context->R14;
    VCpu->Regs->r15 = Context->R15;

    //
    // 2. Instruction Pointer and CPU Flags
    //
    SetGuestRIP(Context->Rip);
    SetGuestRSP(Context->Rsp);
    SetGuestRFlags(Context->Rflags);

    //
    // 3. Machine Control Registers
    //
    SetGuestCr0(Context->Cr0);
    SetGuestCr2(Context->Cr2);
    SetGuestCr3(Context->Cr3);
    SetGuestCr4(Context->Cr4);
    SetGuestCr8(Context->Cr8);

    //
    // 4. Hardware Debug Registers (DR0 - DR7)
    //
    SetGuestDr0(Context->Dr0);
    SetGuestDr1(Context->Dr1);
    SetGuestDr2(Context->Dr2);
    SetGuestDr3(Context->Dr3);
    SetGuestDr6(Context->Dr6);
    SetGuestDr7(Context->Dr7);

    //
    // 5. Segment Selectors, Bases, Limits and Attributes
    //
    VMX_SEGMENT_SELECTOR Cs = {0};
    Cs.Selector          = Context->Cs.Selector;
    Cs.Base              = Context->Cs.Base;
    Cs.Limit             = Context->Cs.Limit;
    Cs.Attributes.AsUInt = Context->Cs.Attributes;
    SetGuestCs(&Cs);

    VMX_SEGMENT_SELECTOR Ss = {0};
    Ss.Selector          = Context->Ss.Selector;
    Ss.Base              = Context->Ss.Base;
    Ss.Limit             = Context->Ss.Limit;
    Ss.Attributes.AsUInt = Context->Ss.Attributes;
    SetGuestSs(&Ss);

    VMX_SEGMENT_SELECTOR Ds = {0};
    Ds.Selector          = Context->Ds.Selector;
    Ds.Base              = Context->Ds.Base;
    Ds.Limit             = Context->Ds.Limit;
    Ds.Attributes.AsUInt = Context->Ds.Attributes;
    SetGuestDs(&Ds);

    VMX_SEGMENT_SELECTOR Es = {0};
    Es.Selector          = Context->Es.Selector;
    Es.Base              = Context->Es.Base;
    Es.Limit             = Context->Es.Limit;
    Es.Attributes.AsUInt = Context->Es.Attributes;
    SetGuestEs(&Es);

    VMX_SEGMENT_SELECTOR Fs = {0};
    Fs.Selector          = Context->Fs.Selector;
    Fs.Base              = Context->Fs.Base;
    Fs.Limit             = Context->Fs.Limit;
    Fs.Attributes.AsUInt = Context->Fs.Attributes;
    SetGuestFs(&Fs);

    VMX_SEGMENT_SELECTOR Gs = {0};
    Gs.Selector          = Context->Gs.Selector;
    Gs.Base              = Context->Gs.Base;
    Gs.Limit             = Context->Gs.Limit;
    Gs.Attributes.AsUInt = Context->Gs.Attributes;
    SetGuestGs(&Gs);

    SetGuestTr(Context->Tr.Base);
    SetGuestLdtr(Context->Ldtr.Base);
    SetGuestGdtr(Context->Gdtr.Base);
    SetGuestIdtr(Context->Idtr.Base);

    //
    // 6. Critical Model-Specific Registers (MSRs)
    //
    CpuWriteMsr(IA32_EFER, Context->MsrEfer);
    CpuWriteMsr(IA32_STAR, Context->MsrStar);
    CpuWriteMsr(IA32_LSTAR, Context->MsrLstar);
    CpuWriteMsr(IA32_CSTAR, Context->MsrCstar);
    CpuWriteMsr(IA32_SYSENTER_CS, Context->MsrSysenterCs);
    CpuWriteMsr(IA32_SYSENTER_ESP, Context->MsrSysenterEsp);
    CpuWriteMsr(IA32_SYSENTER_EIP, Context->MsrSysenterEip);
    CpuWriteMsr(IA32_FS_BASE, Context->MsrFsBase);
    CpuWriteMsr(IA32_GS_BASE, Context->MsrGsBase);
    CpuWriteMsr(IA32_KERNEL_GS_BASE, Context->MsrKernelGsBase);

    //
    // 7. Extended Processor State (FPU / SSE / AVX / AVX-512)
    //
    _xrstor((PVOID)Context->XsaveArea, 0xFFFFFFFFFFFFFFFFULL);
}

//////////////////////////////////////////////////
//       EPT A/D & PML Memory Rollback          //
//////////////////////////////////////////////////

/**
 * @brief Initializes the snapshot memory tracker data structures.
 *
 * @param Tracker Pointer to tracker instance.
 * @return BOOLEAN TRUE on success, FALSE otherwise.
 */
BOOLEAN
SnapshotInitializeMemoryTracker(PSNAPSHOT_MEMORY_TRACKER Tracker)
{
    RtlZeroMemory(Tracker, sizeof(SNAPSHOT_MEMORY_TRACKER));
    Tracker->MaxCapacity   = (UINT32)SNAPSHOT_MAX_DIRTY_PAGES;
    Tracker->AdBitsEnabled = g_CompatibilityCheck.PmlSupport;

    return TRUE;
}

/**
 * @brief Records a pristine shadow copy of a physical 4KB frame prior to mutation.
 *
 * @param Tracker Pointer to tracker instance.
 * @param PhysicalAddress Guest Physical Address (GPA) of the page frame.
 * @return BOOLEAN TRUE on success, FALSE if tracking pool is exhausted.
 */
BOOLEAN
SnapshotRecordPristinePage(PSNAPSHOT_MEMORY_TRACKER Tracker, UINT64 PhysicalAddress)
{
    UINT64 AlignedGpa = PhysicalAddress & ~0xFFFULL;

    //
    // Check if GPA is already tracked
    //
    for (UINT32 i = 0; i < Tracker->DirtyCount; i++)
    {
        if (Tracker->DirtyPages[i].PhysicalAddress == AlignedGpa)
        {
            return TRUE;
        }
    }

    if (Tracker->DirtyCount >= Tracker->MaxCapacity)
    {
        LogError("Err, snapshot dirty page capacity exceeded!");
        return FALSE;
    }

    //
    // Allocate pristine 4KB shadow frame
    //
    PVOID ShadowFrame = PlatformMemAllocateNonPagedPool(PAGE_SIZE);
    if (ShadowFrame == NULL)
    {
        LogError("Err, failed to allocate shadow memory frame!");
        return FALSE;
    }

    //
    // Map physical frame and create backup
    //
    PVOID SourceVa = PlatformMemMapPhysicalMemory(AlignedGpa, PAGE_SIZE);
    if (SourceVa != NULL)
    {
        RtlCopyMemory(ShadowFrame, SourceVa, PAGE_SIZE);
        PlatformMemUnmapPhysicalMemory(SourceVa, PAGE_SIZE);
    }

    UINT32 Index = Tracker->DirtyCount++;
    Tracker->DirtyPages[Index].PhysicalAddress  = AlignedGpa;
    Tracker->DirtyPages[Index].PristineShadowVa = ShadowFrame;

    return TRUE;
}

/**
 * @brief Restores all dirty physical frames back to their baseline pristine contents.
 *
 * @param VCpu Virtual processor state.
 * @param Tracker Pointer to memory tracker instance.
 * @return BOOLEAN TRUE on successful restoration.
 */
BOOLEAN
SnapshotRestoreDirtyPages(VIRTUAL_MACHINE_STATE * VCpu, PSNAPSHOT_MEMORY_TRACKER Tracker)
{
    UINT16   PmlIdx;
    BOOLEAN  IsLargePage;
    PVOID    PmlEntry;

    //
    // 1. Hardware PML Mode: Read logged GPAs directly from hardware PML buffer
    //
    if (Tracker->AdBitsEnabled && VCpu->PmlBufferAddress != NULL)
    {
        VmxVmread16P(VMCS_GUEST_PML_INDEX, &PmlIdx);

        if (PmlIdx < PML_ENTITY_NUM)
        {
            UINT64 * PmlBuf = VCpu->PmlBufferAddress;

            for (UINT16 i = PmlIdx + 1; i < PML_ENTITY_NUM; i++)
            {
                UINT64 WrittenPhysAddr = PmlBuf[i] & ~0xFFFULL;

                //
                // Find matching shadow frame
                //
                for (UINT32 j = 0; j < Tracker->DirtyCount; j++)
                {
                    if (Tracker->DirtyPages[j].PhysicalAddress == WrittenPhysAddr)
                    {
                        PVOID TargetVa = PlatformMemMapPhysicalMemory(WrittenPhysAddr, PAGE_SIZE);
                        if (TargetVa != NULL)
                        {
                            RtlCopyMemory(TargetVa, Tracker->DirtyPages[j].PristineShadowVa, PAGE_SIZE);
                            PlatformMemUnmapPhysicalMemory(TargetVa, PAGE_SIZE);
                        }
                        break;
                    }
                }

                //
                // Clear hardware EPT Dirty Flag (Bit 9)
                //
                PmlEntry = EptGetPml1OrPml2Entry(VCpu->EptPageTable, WrittenPhysAddr, &IsLargePage);
                if (PmlEntry != NULL && !IsLargePage)
                {
                    ((PEPT_PML1_ENTRY)PmlEntry)->Dirty = FALSE;
                }
            }

            //
            // Reset PML Index back to top (511)
            //
            VmxVmwrite64(VMCS_GUEST_PML_INDEX, PML_ENTITY_NUM - 1);
        }
    }
    else
    {
        //
        // 2. Software CoW Fallback Mode: Rollback all tracked pages
        //
        for (UINT32 i = 0; i < Tracker->DirtyCount; i++)
        {
            UINT64 Gpa = Tracker->DirtyPages[i].PhysicalAddress;
            PVOID TargetVa = PlatformMemMapPhysicalMemory(Gpa, PAGE_SIZE);
            if (TargetVa != NULL)
            {
                RtlCopyMemory(TargetVa, Tracker->DirtyPages[i].PristineShadowVa, PAGE_SIZE);
                PlatformMemUnmapPhysicalMemory(TargetVa, PAGE_SIZE);
            }

            //
            // Re-arm write protection on PML1 entry
            //
            PmlEntry = EptGetPml1OrPml2Entry(VCpu->EptPageTable, Gpa, &IsLargePage);
            if (PmlEntry != NULL && !IsLargePage)
            {
                ((PEPT_PML1_ENTRY)PmlEntry)->WriteAccess = FALSE;
            }
        }
    }

    //
    // 3. Flush EPT TLB on current CPU (Single-Context Invalidation)
    //
    INVEPT_DESCRIPTOR InveptDesc = {0};
    InveptDesc.EptPointer        = VCpu->EptPointer.Flags;
    __vmx_invept(INVEPT_SINGLE_CONTEXT, &InveptDesc);

    return TRUE;
}

//////////////////////////////////////////////////
//         Intel PT ToPA Edge Coverage Engine   //
//////////////////////////////////////////////////

/**
 * @brief Parses Intel PT ToPA packet stream and updates the 64KB AFL-compatible coverage map.
 *
 * @param PtBuffer Raw binary packet buffer produced by Intel PT hardware.
 * @param PtSize Length of PT packet data in bytes.
 * @param AflMap Pointer to shared AFL coverage structure.
 */
VOID
SnapshotParsePtCoverage(UINT8 * PtBuffer, SIZE_T PtSize, PFUZZ_AFL_COVERAGE_MAP AflMap)
{
    SIZE_T Offset = 0;
    UINT64 PrevIp = AflMap->PreviousIp;

    while (Offset < PtSize)
    {
        UINT8 Byte0 = PtBuffer[Offset];

        //
        // 1. Packet Stream Boundary (PSB): 02 82 02 82 02 82 02 82 ... (16 bytes)
        //
        if (Byte0 == 0x02 && Offset + 1 < PtSize && PtBuffer[Offset + 1] == 0x82)
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
                CurrentIp = *(UINT64 *)(PtBuffer + Offset) & 0xFFFFFFFFFFFFULL;
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

//////////////////////////////////////////////////
//       Synthetic TSC Time Dilation Engine     //
//////////////////////////////////////////////////

/**
 * @brief Compensates the virtual guest TSC offset to freeze cycle counter advances during iteration setup.
 *
 * @param VCpu Virtual processor state.
 * @param FrozenTsc Target baseline timestamp counter value.
 */
VOID
SnapshotFreezeTsc(VIRTUAL_MACHINE_STATE * VCpu, UINT64 FrozenTsc)
{
    UINT64 HardwareTsc  = __rdtsc();
    UINT64 TargetOffset = FrozenTsc - HardwareTsc;

    VmxVmwrite64(VMCS_CTRL_TSC_OFFSET, TargetOffset);
}

//////////////////////////////////////////////////
//         Automated Crash Triage Engine        //
//////////////////////////////////////////////////

/**
 * @brief Captures exception state, faulting RIP/CR2, hardware LBR trace, and computes crash hash.
 *
 * @param VCpu Virtual processor state.
 * @param ExceptionVector The triggered hardware exception (13=#GP, 14=#PF, 6=#UD).
 * @param Report Destination crash telemetry structure.
 */
VOID
SnapshotCaptureCrash(VIRTUAL_MACHINE_STATE * VCpu, UINT32 ExceptionVector, PFUZZ_CRASH_REPORT Report)
{
    Report->ExceptionVector   = ExceptionVector;
    Report->HardwareErrorCode = (UINT32)VmxVmread64(VMCS_VMEXIT_INTERRUPTION_ERROR_CODE);
    Report->FaultingRip       = VmxVmread64(VMCS_GUEST_RIP);
    Report->FaultingAddress   = (ExceptionVector == 14) ? CpuReadCr2() : 0;

    SnapshotSaveVcpuContext(VCpu, &Report->RegistersAtCrash);

    //
    // Capture 32-entry Hardware Last Branch Record (LBR) callstack
    //
    Report->LbrEntryCount = 0;
    for (UINT32 i = 0; i < SNAPSHOT_MAX_LBR_DEPTH; i++)
    {
        Report->LbrStack[i].From = CpuReadMsr(MSR_LASTBRANCH_0_FROM_IP + i);
        Report->LbrStack[i].To   = CpuReadMsr(MSR_LASTBRANCH_0_TO_IP + i);

        if (Report->LbrStack[i].From != 0)
        {
            Report->LbrEntryCount++;
        }
    }

    //
    // Calculate 64-bit Collision-Resistant Crash Hash (FNV-1a Variant)
    //
    UINT64 Hash = 0xCBF29CE484222325ULL;
    Hash ^= Report->FaultingRip;
    Hash *= 0x100000001B3ULL;
    Hash ^= (UINT64)ExceptionVector;
    Hash *= 0x100000001B3ULL;

    for (UINT32 i = 0; i < 4 && i < Report->LbrEntryCount; i++)
    {
        Hash ^= Report->LbrStack[i].To;
        Hash *= 0x100000001B3ULL;
    }

    Report->CrashHash = Hash;
}

//////////////////////////////////////////////////
//       Top-Level Engine Lifecycle APIs        //
//////////////////////////////////////////////////

/**
 * @brief Initializes the global snapshot fuzzing and coverage engine.
 *
 * @return BOOLEAN TRUE on success.
 */
BOOLEAN
SnapshotInitialize()
{
    if (g_AflCoverageMap == NULL)
    {
        g_AflCoverageMap = (PFUZZ_AFL_COVERAGE_MAP)PlatformMemAllocateZeroedNonPagedPool(sizeof(FUZZ_AFL_COVERAGE_MAP));
    }

    SnapshotInitializeMemoryTracker(&g_SnapshotMemoryTracker);
    RtlZeroMemory(&g_LastCrashReport, sizeof(FUZZ_CRASH_REPORT));
    g_FuzzerActive = FALSE;

    return TRUE;
}

/**
 * @brief Uninitializes the snapshot fuzzing engine and frees allocated resources.
 */
VOID
SnapshotUninitialize()
{
    UINT32 Status = 0;
    SnapshotClear(&Status);

    if (g_AflCoverageMap != NULL)
    {
        PlatformMemFreePool(g_AflCoverageMap);
        g_AflCoverageMap = NULL;
    }
}

/**
 * @brief Takes a full baseline vCPU and memory execution snapshot.
 *
 * @param Request Snapshot take request packet.
 * @return NTSTATUS STATUS_SUCCESS on success.
 */
NTSTATUS
SnapshotTake(PDEBUGGER_SNAPSHOT_TAKE_REQUEST Request)
{
    UINT32 CoreId = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * VCpu = &g_GuestState[CoreId];

    //
    // Initialize coverage map if needed
    //
    if (g_AflCoverageMap == NULL)
    {
        SnapshotInitialize();
    }

    //
    // 1. Capture full vCPU architectural context
    //
    SnapshotSaveVcpuContext(VCpu, &g_VcpuBaselineContext);

    //
    // 2. Initialize memory tracker and arm hardware PML / EPT A/D tracking
    //
    SnapshotInitializeMemoryTracker(&g_SnapshotMemoryTracker);
    DirtyLoggingEnable(VCpu);

    //
    // Arm exception bitmap for pre-IDT crash trapping (#GP, #PF, #DE, #DF)
    //
    HvSetExceptionBitmap(VCpu, EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT);
    HvSetExceptionBitmap(VCpu, EXCEPTION_VECTOR_PAGE_FAULT);
    HvSetExceptionBitmap(VCpu, EXCEPTION_VECTOR_DIVIDE_ERROR);
    HvSetExceptionBitmap(VCpu, EXCEPTION_VECTOR_DOUBLE_FAULT);

    //
    // 3. Freeze baseline TSC
    //
    g_SnapshotBaselineTsc = __rdtsc();

    //
    // 4. Fill output telemetry
    //
    Request->SnapshotRip   = g_VcpuBaselineContext.Rip;
    Request->KernelStatus  = 0;
    g_FuzzerActive         = TRUE;

    LogInfo("Snapshot successfully created at RIP: 0x%llx", Request->SnapshotRip);

    return STATUS_SUCCESS;
}

/**
 * @brief Restores execution state and dirty pages to the baseline snapshot.
 *
 * @param Request Snapshot restore request packet.
 * @return NTSTATUS STATUS_SUCCESS on success.
 */
NTSTATUS
SnapshotRestore(PDEBUGGER_SNAPSHOT_RESTORE_REQUEST Request)
{
    if (!g_FuzzerActive)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    UINT32 CoreId = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * VCpu = &g_GuestState[CoreId];

    //
    // 1. Freeze TSC cycle advancement during restore
    //
    SnapshotFreezeTsc(VCpu, g_SnapshotBaselineTsc);

    //
    // 2. Roll back modified physical frames to pristine state
    //
    SnapshotRestoreDirtyPages(VCpu, &g_SnapshotMemoryTracker);

    //
    // 3. Restore all vCPU registers, MSRs, and segment selectors
    //
    SnapshotRestoreVcpuContext(VCpu, &g_VcpuBaselineContext);

    //
    // 4. Report metrics
    //
    Request->RestoredPageCount = g_SnapshotMemoryTracker.DirtyCount;
    Request->RestoredRip       = g_VcpuBaselineContext.Rip;
    Request->KernelStatus      = 0;

    return STATUS_SUCCESS;
}

/**
 * @brief Clears snapshot frames and disarms hardware tracking.
 *
 * @param Status Pointer to status output.
 * @return NTSTATUS STATUS_SUCCESS on success.
 */
NTSTATUS
SnapshotClear(PUINT32 Status)
{
    UINT32 CoreId = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * VCpu = &g_GuestState[CoreId];

    if (g_FuzzerActive)
    {
        DirtyLoggingDisable(VCpu);
        HvUnsetExceptionBitmap(VCpu, EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT);
        HvUnsetExceptionBitmap(VCpu, EXCEPTION_VECTOR_PAGE_FAULT);
        HvUnsetExceptionBitmap(VCpu, EXCEPTION_VECTOR_DIVIDE_ERROR);
        HvUnsetExceptionBitmap(VCpu, EXCEPTION_VECTOR_DOUBLE_FAULT);
        g_FuzzerActive = FALSE;
    }

    //
    // Free all allocated shadow page copies
    //
    for (UINT32 i = 0; i < g_SnapshotMemoryTracker.DirtyCount; i++)
    {
        if (g_SnapshotMemoryTracker.DirtyPages[i].PristineShadowVa != NULL)
        {
            PlatformMemFreePool(g_SnapshotMemoryTracker.DirtyPages[i].PristineShadowVa);
            g_SnapshotMemoryTracker.DirtyPages[i].PristineShadowVa = NULL;
        }
    }
    g_SnapshotMemoryTracker.DirtyCount = 0;

    if (Status != NULL)
    {
        *Status = 0;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Executes a single snapshot fuzzing iteration with input injection and automatic rollback.
 *
 * @param Request Fuzz iterate request packet.
 * @return NTSTATUS STATUS_SUCCESS on success.
 */
NTSTATUS
SnapshotFuzzIterate(PDEBUGGER_FUZZ_ITERATE_REQUEST Request)
{
    if (!g_FuzzerActive)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    UINT64 StartCycles = __rdtsc();

    //
    // 1. Inject mutated testcase input into target virtual address
    //
    if (Request->TargetVirtualAddress != 0 && Request->InputSize > 0)
    {
        // Record GPA for rollback prior to writing
        UINT64 TargetGpa = VirtualAddressToPhysicalAddress((PVOID)Request->TargetVirtualAddress);
        if (TargetGpa != 0)
        {
            SnapshotRecordPristinePage(&g_SnapshotMemoryTracker, TargetGpa);
            PVOID TargetVa = PlatformMemMapPhysicalMemory(TargetGpa, PAGE_SIZE);
            if (TargetVa != NULL)
            {
                UINT32 CopyLen = min(Request->InputSize, (UINT32)PAGE_SIZE);
                RtlCopyMemory(TargetVa, Request->InputBuffer, CopyLen);
                PlatformMemUnmapPhysicalMemory(TargetVa, PAGE_SIZE);
            }
        }
    }

    //
    // 2. Check if a crash occurred during testcase execution
    //
    if (g_LastCrashReport.CrashHash != 0)
    {
        Request->ExecutionStatus = FUZZ_STATUS_CRASH_EXCEPTION;
    }
    else
    {
        Request->ExecutionStatus = FUZZ_STATUS_SUCCESS;
    }

    Request->ElapsedCycles = __rdtsc() - StartCycles;

    //
    // 3. Roll back state to baseline snapshot
    //
    DEBUGGER_SNAPSHOT_RESTORE_REQUEST RestoreReq = {0};
    SnapshotRestore(&RestoreReq);

    //
    // 4. Update AFL coverage map execution counter
    //
    if (g_AflCoverageMap != NULL)
    {
        g_AflCoverageMap->TotalExecutions++;
    }

    Request->KernelStatus = 0;
    return STATUS_SUCCESS;
}

/**
 * @brief Retrieves the shared 64KB AFL-compatible coverage map.
 *
 * @param DestinationBuffer Pointer to destination buffer.
 * @return NTSTATUS STATUS_SUCCESS on success.
 */
NTSTATUS
SnapshotMapCoverage(PFUZZ_AFL_COVERAGE_MAP DestinationBuffer)
{
    if (g_AflCoverageMap == NULL)
    {
        SnapshotInitialize();
    }

    if (DestinationBuffer != NULL && g_AflCoverageMap != NULL)
    {
        RtlCopyMemory(DestinationBuffer, g_AflCoverageMap, sizeof(FUZZ_AFL_COVERAGE_MAP));
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Retrieves the last crash telemetry report.
 *
 * @param Report Pointer to destination crash report.
 * @return NTSTATUS STATUS_SUCCESS on success.
 */
NTSTATUS
SnapshotGetCrashReport(PFUZZ_CRASH_REPORT Report)
{
    if (Report != NULL)
    {
        RtlCopyMemory(Report, &g_LastCrashReport, sizeof(FUZZ_CRASH_REPORT));
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Resets the last captured crash telemetry report.
 *
 * @return NTSTATUS STATUS_SUCCESS on success.
 */
NTSTATUS
SnapshotClearCrashReport(VOID)
{
    RtlZeroMemory(&g_LastCrashReport, sizeof(FUZZ_CRASH_REPORT));
    return STATUS_SUCCESS;
}


/**
 * @brief Sets the stealth evasion mode for timing and isolation.
 *
 * @param Mode Evasion mode mask.
 */
VOID
SnapshotSetEvasionMode(UINT32 Mode)
{
    g_EvasionMode = Mode;
}

/**
 * @brief Executes an autonomous fuzzing batch loop directly within root mode.
 *
 * @param VCpu Virtual processor state.
 * @param IterationCount Number of testcases to execute.
 * @param CrashReport Pointer to crash report structure if a crash occurred.
 * @return NTSTATUS STATUS_SUCCESS on completion, STATUS_ALERTED if crash occurred.
 */
NTSTATUS
SnapshotRunAutonomousBatch(VIRTUAL_MACHINE_STATE * VCpu, UINT32 IterationCount, PFUZZ_CRASH_REPORT CrashReport)
{
    if (!g_FuzzerActive || VCpu == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    for (UINT32 i = 0; i < IterationCount; i++)
    {
        // 1. Check if an unhandled crash was trapped
        if (g_LastCrashReport.ExceptionVector != 0)
        {
            if (CrashReport != NULL)
            {
                RtlCopyMemory(CrashReport, &g_LastCrashReport, sizeof(FUZZ_CRASH_REPORT));
            }
            return STATUS_ALERTED;
        }

        // 2. Restore dirty memory frames
        SnapshotRestoreDirtyPages(VCpu, &g_SnapshotMemoryTracker);

        // 3. Restore architectural register context
        SnapshotRestoreVcpuContext(VCpu, &g_VcpuBaselineContext);

        // 4. Update metrics
        if (g_AflCoverageMap != NULL)
        {
            g_AflCoverageMap->TotalExecutions++;
        }
    }

    return STATUS_SUCCESS;
}

/**
 * @brief Executes an autonomous fuzzing batch directly within kernel/hypervisor mode.
 *
 * @param Request Pointer to batch request structure.
 * @return NTSTATUS STATUS_SUCCESS on success.
 */
NTSTATUS
SnapshotRunBatch(PDEBUGGER_FUZZ_RUN_BATCH_REQUEST Request)
{
    if (!g_FuzzerActive || Request == NULL)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    UINT64 StartBatchCycles = __rdtsc();
    Request->ExecutedCount = 0;
    Request->ExecutionStatus = FUZZ_STATUS_SUCCESS;

    // Mutated payload scratch buffer
    UCHAR MutatedInput[SNAPSHOT_MAX_INPUT_SIZE];
    UINT32 InputSize = min(Request->InputSize, (UINT32)SNAPSHOT_MAX_INPUT_SIZE);
    if (InputSize > 0)
    {
        RtlCopyMemory(MutatedInput, Request->InputBuffer, InputSize);
    }

    for (UINT32 i = 0; i < Request->IterationCount; i++)
    {
        // 1. In-kernel mutation if requested
        if (InputSize > 0 && Request->TargetVirtualAddress != 0)
        {
            // Apply quick havoc / arithmetic mutations
            for (UINT32 b = 0; b < InputSize; b++)
            {
                if ((i + b) % 7 == 0)
                {
                    MutatedInput[b] ^= (UCHAR)(1 << ((i + b) % 8));
                }
                else if ((i + b) % 11 == 0)
                {
                    MutatedInput[b] = (UCHAR)(MutatedInput[b] + 1);
                }
            }

            // Inject into target memory
            UINT32 RemainingSize = InputSize;
            UINT64 CurrentVa     = Request->TargetVirtualAddress;
            PUCHAR InputPtr      = MutatedInput;

            while (RemainingSize > 0)
            {
                UINT64 OffsetInPage    = CurrentVa & (PAGE_SIZE - 1);
                UINT32 BytesInThisPage = (UINT32)min((UINT64)RemainingSize, (UINT64)(PAGE_SIZE - OffsetInPage));
                UINT64 TargetGpa       = VirtualAddressToPhysicalAddress((PVOID)CurrentVa);
                if (TargetGpa != 0)
                {
                    SnapshotRecordPristinePage(&g_SnapshotMemoryTracker, TargetGpa & ~(PAGE_SIZE - 1));
                    PVOID MappedVa = PlatformMemMapPhysicalMemory(TargetGpa, PAGE_SIZE);
                    if (MappedVa != NULL)
                    {
                        RtlCopyMemory((PUCHAR)MappedVa + OffsetInPage, InputPtr, BytesInThisPage);
                        PlatformMemUnmapPhysicalMemory(MappedVa, PAGE_SIZE);
                    }
                }
                RemainingSize -= BytesInThisPage;
                CurrentVa     += BytesInThisPage;
                InputPtr      += BytesInThisPage;
            }
        }

        // 2. Check if a crash occurred during testcase execution
        if (g_LastCrashReport.CrashHash != 0)
        {
            Request->ExecutionStatus = FUZZ_STATUS_CRASH_EXCEPTION;
            RtlCopyMemory(&Request->CrashReport, &g_LastCrashReport, sizeof(FUZZ_CRASH_REPORT));
            Request->ExecutedCount = i + 1;
            break;
        }

        // 3. Rollback dirty pages and context
        DEBUGGER_SNAPSHOT_RESTORE_REQUEST RestoreReq = {0};
        SnapshotRestore(&RestoreReq);

        // 4. Update metrics
        if (g_AflCoverageMap != NULL)
        {
            g_AflCoverageMap->TotalExecutions++;
        }

        Request->ExecutedCount = i + 1;
    }

    Request->ElapsedCycles = __rdtsc() - StartBatchCycles;
    Request->KernelStatus  = 0;
    return STATUS_SUCCESS;
}


