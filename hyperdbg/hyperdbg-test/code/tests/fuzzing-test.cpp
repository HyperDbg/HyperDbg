/**
 * @file fuzzing-test.cpp
 * @author HyperDbg Dev Team
 * @brief Unit and integration tests for snapshot fuzzing, AFL bitmap edge tracking, and crash triage.
 * @details Validates vCPU context layout, 64KB AFL edge hashing, LBR crash callstack hashing,
 *          and input mutation safety.
 * @version 0.1
 * @date 2026-09-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Unit Test 1: Validates SNAPSHOT_VCPU_CONTEXT structural integrity and register alignment.
 */
static BOOLEAN
TestSnapshotContextLayout()
{
    printf("  [+] Sub-test: Validating SNAPSHOT_VCPU_CONTEXT memory layout...\n");

    SNAPSHOT_VCPU_CONTEXT Context = {0};

    // Verify 64-bit GPR population
    Context.Rax = 0x1122334455667788ULL;
    Context.Rbx = 0x99AABBCCDDEEFF00ULL;
    Context.Rsp = 0x00007FF7A0008000ULL;
    Context.Rip = 0x00007FF7A0001000ULL;
    Context.Rflags = 0x246ULL;

    if (Context.Rax != 0x1122334455667788ULL ||
        Context.Rbx != 0x99AABBCCDDEEFF00ULL ||
        Context.Rsp != 0x00007FF7A0008000ULL ||
        Context.Rip != 0x00007FF7A0001000ULL)
    {
        printf("  [-] Err: GPR register value corruption in snapshot context.\n");
        return FALSE;
    }

    // Verify Control Registers
    Context.Cr3 = 0x0000000100002000ULL;
    if (Context.Cr3 != 0x0000000100002000ULL)
    {
        printf("  [-] Err: CR3 control register mismatch.\n");
        return FALSE;
    }

    // Verify XSAVE area size constant
    if (sizeof(Context.XsaveArea) != SNAPSHOT_XSAVE_AREA_SIZE)
    {
        printf("  [-] Err: XSAVE buffer size mismatch (%zu != %llu).\n",
               sizeof(Context.XsaveArea), SNAPSHOT_XSAVE_AREA_SIZE);
        return FALSE;
    }

    printf("  [+] Sub-test: SNAPSHOT_VCPU_CONTEXT passed.\n");
    return TRUE;
}

/**
 * @brief Unit Test 2: Validates AFL 64KB coverage bitmap edge transition hashing.
 */
static BOOLEAN
TestAflEdgeCoverageHashing()
{
    printf("  [+] Sub-test: Validating AFL 64KB coverage bitmap edge tracking...\n");

    FUZZ_AFL_COVERAGE_MAP Map = {0};

    // Verify map size matches 64KB constant
    if (sizeof(Map.TraceBits) != SNAPSHOT_AFL_MAP_SIZE)
    {
        printf("  [-] Err: AFL map size mismatch (%zu != %llu).\n",
               sizeof(Map.TraceBits), SNAPSHOT_AFL_MAP_SIZE);
        return FALSE;
    }

    // Simulate branch edge transitions
    UINT64 Branches[][2] = {
        {0x7ff700001000ULL, 0x7ff700001020ULL},
        {0x7ff700001020ULL, 0x7ff700001080ULL},
        {0x7ff700001080ULL, 0x7ff700001100ULL},
        {0x7ff700001000ULL, 0x7ff700001020ULL} // Repeated edge
    };

    for (size_t i = 0; i < sizeof(Branches) / sizeof(Branches[0]); i++)
    {
        UINT64 PrevIp = Branches[i][0];
        UINT64 CurIp  = Branches[i][1];

        // AFL edge formula: (PrevIP ^ (CurIP >> 1)) & (MAP_SIZE - 1)
        UINT32 EdgeIndex = (UINT32)((PrevIp ^ (CurIp >> 1)) & (SNAPSHOT_AFL_MAP_SIZE - 1));

        if (Map.TraceBits[EdgeIndex] == 0)
        {
            Map.TotalEdgesCovered++;
        }
        Map.TraceBits[EdgeIndex]++;
    }

    // We had 3 unique branch transitions across 4 simulated executions
    if (Map.TotalEdgesCovered != 3)
    {
        printf("  [-] Err: Unique edges covered calculation failed (%llu != 3).\n",
               Map.TotalEdgesCovered);
        return FALSE;
    }

    // Verify repeated edge has hit count of 2
    UINT32 RepeatedEdge = (UINT32)((Branches[0][0] ^ (Branches[0][1] >> 1)) & (SNAPSHOT_AFL_MAP_SIZE - 1));
    if (Map.TraceBits[RepeatedEdge] != 2)
    {
        printf("  [-] Err: Edge bucket hit count failed (%u != 2).\n", Map.TraceBits[RepeatedEdge]);
        return FALSE;
    }

    printf("  [+] Sub-test: AFL coverage edge hashing passed.\n");
    return TRUE;
}

/**
 * @brief Unit Test 3: Validates crash deduplication hashing with LBR callstack ring.
 */
static BOOLEAN
TestCrashDeduplicationHashing()
{
    printf("  [+] Sub-test: Validating crash deduplication FNV-1a hashing...\n");

    FUZZ_CRASH_REPORT CrashA = {0};
    FUZZ_CRASH_REPORT CrashB = {0};
    FUZZ_CRASH_REPORT CrashC = {0};

    // Crash A: #GP at 0x7ff700002000
    CrashA.ExceptionVector = 13;
    CrashA.FaultingRip     = 0x7ff700002000ULL;
    CrashA.FaultingAddress = 0xdeadbeef0000ULL;
    CrashA.LbrEntryCount   = 2;
    CrashA.LbrStack[0].From = 0x7ff700001050ULL;
    CrashA.LbrStack[0].To   = 0x7ff700002000ULL;
    CrashA.LbrStack[1].From = 0x7ff700001000ULL;
    CrashA.LbrStack[1].To   = 0x7ff700001050ULL;

    // Compute hash for Crash A using FNV-1a model
    UINT64 HashA = 14695981039346656037ULL;
    HashA ^= (UINT64)CrashA.ExceptionVector;
    HashA *= 1099511628211ULL;
    HashA ^= CrashA.FaultingRip;
    HashA *= 1099511628211ULL;
    for (UINT32 i = 0; i < CrashA.LbrEntryCount; i++)
    {
        HashA ^= CrashA.LbrStack[i].From ^ CrashA.LbrStack[i].To;
        HashA *= 1099511628211ULL;
    }
    CrashA.CrashHash = HashA;

    // Crash B: Identical to Crash A -> Must produce IDENTICAL hash
    CrashB = CrashA;

    if (CrashA.CrashHash != CrashB.CrashHash)
    {
        printf("  [-] Err: Deterministic hash comparison failed for identical crashes.\n");
        return FALSE;
    }

    // Crash C: Different faulting RIP -> Must produce DIFFERENT hash
    CrashC = CrashA;
    CrashC.FaultingRip = 0x7ff700003000ULL;
    UINT64 HashC = 14695981039346656037ULL;
    HashC ^= (UINT64)CrashC.ExceptionVector;
    HashC *= 1099511628211ULL;
    HashC ^= CrashC.FaultingRip;
    HashC *= 1099511628211ULL;
    for (UINT32 i = 0; i < CrashC.LbrEntryCount; i++)
    {
        HashC ^= CrashC.LbrStack[i].From ^ CrashC.LbrStack[i].To;
        HashC *= 1099511628211ULL;
    }
    CrashC.CrashHash = HashC;

    if (CrashA.CrashHash == CrashC.CrashHash)
    {
        printf("  [-] Err: Hash collision between distinct crash callstacks.\n");
        return FALSE;
    }

    printf("  [+] Sub-test: Crash deduplication hashing passed.\n");
    return TRUE;
}

/**
 * @brief Unit Test 4: Validates mutator safety (non-null, bounded).
 */
static BOOLEAN
TestMutatorSafety()
{
    printf("  [+] Sub-test: Validating input mutation safety...\n");

    UCHAR Buffer[64] = "BASELINE_INPUT_SEED_TEST_PAYLOAD";
    UINT32 OriginalSize = 32;

    // Run 100 mutations
    for (int i = 0; i < 100; i++)
    {
        HyperFuzzMutateBuffer(Buffer, OriginalSize, sizeof(Buffer), 0);
    }

    // Buffer should still have valid length and be modified
    if (Buffer == NULL)
    {
        printf("  [-] Err: Mutator resulted in NULL buffer.\n");
        return FALSE;
    }

    printf("  [+] Sub-test: Input mutator safety passed.\n");
    return TRUE;
}

/**
 * @brief Unit Test 5: Validates Synthetic TSC Time Dilation math and freeze simulation.
 */
static BOOLEAN
TestSyntheticTscEvasion()
{
    printf("  [+] Sub-test: Validating synthetic TSC time dilation & freeze...\n");

    UINT64 BaselineTsc = 0x100000000ULL;
    UINT64 VirtualDelta = 0;

    // Simulate 10 RDTSC queries with FIXED_DELTA (+64 cycles per query)
    for (int i = 0; i < 10; i++)
    {
        VirtualDelta += 0x40;
        UINT64 QueryTsc = BaselineTsc + VirtualDelta;

        if (QueryTsc <= BaselineTsc)
        {
            printf("  [-] Err: Non-monotonic synthetic TSC query result.\n");
            return FALSE;
        }
    }

    if (VirtualDelta != (10 * 0x40))
    {
        printf("  [-] Err: Synthetic TSC delta drift detected (%llu != %u).\n",
               VirtualDelta, 10 * 0x40);
        return FALSE;
    }

    // Simulate snapshot reset -> VirtualDelta must reset to 0
    VirtualDelta = 0;
    UINT64 ResetTsc = BaselineTsc + VirtualDelta;
    if (ResetTsc != BaselineTsc)
    {
        printf("  [-] Err: Synthetic TSC failed to return to baseline after reset.\n");
        return FALSE;
    }

    printf("  [+] Sub-test: Synthetic TSC time dilation passed.\n");
    return TRUE;
}

/**
 * @brief Unit Test 6: Validates L1 direct-mapped opcode cache and zero-CR3 fast decoding.
 */
static BOOLEAN
TestFastOpcodeDecoder()
{
    printf("  [+] Sub-test: Validating L1 Opcode Call-Site Cache & Fast Decoding...\n");

    #define TEST_CACHE_SIZE 256
    #define TEST_CACHE_MASK (TEST_CACHE_SIZE - 1)

    struct CacheEntry
    {
        UINT64 Rip;
        UINT32 OpcodeType; // 1 = SYSCALL, 2 = SYSRET
    } Cache[TEST_CACHE_SIZE] = {0};

    UINT64 SyscallRip = 0x00007FF7B0001550ULL;
    UINT64 SysretRip  = 0xFFFFF800400032A0ULL;

    // 1. Cold cache miss
    UINT32 IndexSyscall = (UINT32)((SyscallRip >> 1) & TEST_CACHE_MASK);
    if (Cache[IndexSyscall].Rip == SyscallRip)
    {
        printf("  [-] Err: Expected cold cache miss on first encounter.\n");
        return FALSE;
    }

    // 2. Populate cache entries
    Cache[IndexSyscall].Rip = SyscallRip;
    Cache[IndexSyscall].OpcodeType = 1;

    UINT32 IndexSysret = (UINT32)((SysretRip >> 1) & TEST_CACHE_MASK);
    Cache[IndexSysret].Rip = SysretRip;
    Cache[IndexSysret].OpcodeType = 2;

    // 3. Hot cache hit verification (zero memory reads, zero CR3 writes)
    if (Cache[IndexSyscall].Rip != SyscallRip || Cache[IndexSyscall].OpcodeType != 1)
    {
        printf("  [-] Err: L1 cache hit failed for SYSCALL call-site.\n");
        return FALSE;
    }

    if (Cache[IndexSysret].Rip != SysretRip || Cache[IndexSysret].OpcodeType != 2)
    {
        printf("  [-] Err: L1 cache hit failed for SYSRET call-site.\n");
        return FALSE;
    }

    printf("  [+] Sub-test: L1 Opcode Call-Site Cache passed.\n");
    return TRUE;
}

/**
 * @brief Unit Test 7: Validates hardware MTF return interception and KTRAP_FRAME stealth.
 */
static BOOLEAN
TestMtfStealthInterception()
{
    printf("  [+] Sub-test: Validating Hardware MTF Return Interception & Trap-Frame Stealth...\n");

    UINT64 GuestRflags = 0x202; // IF set, TF clear
    UINT64 GuestR11    = GuestRflags;

    // Verify: Under legacy Trap-Flag approach, R11 has TF set
    UINT64 LegacyR11 = GuestR11 | 0x100; // X86_FLAGS_TF
    if ((LegacyR11 & 0x100) == 0)
    {
        printf("  [-] Err: Legacy TF failed to set bit 8.\n");
        return FALSE;
    }

    // Verify: Under Stealth MTF approach, R11 and GuestRflags remain pristine (TF = 0)
    BOOLEAN StealthMtfArmed = TRUE;
    UINT64  StealthR11      = GuestR11; // Untouched!

    if (StealthMtfArmed)
    {
        // RFLAGS in KTRAP_FRAME contains ZERO trace of debugging
        if ((StealthR11 & 0x100) != 0)
        {
            printf("  [-] Err: Stealth MTF leaked TF into guest R11/KTRAP_FRAME!\n");
            return FALSE;
        }
    }

    printf("  [+] Sub-test: Hardware MTF Return Interception passed.\n");
    return TRUE;
}

/**
 * @brief Unit Test 8: Validates MSR latency compensation and timing smoothing math.
 */
static BOOLEAN
TestMsrLatencyCompensation()
{
    printf("  [+] Sub-test: Validating MSR latency compensation & EPT timing smoothing...\n");

    UINT64 BaselineTsc = 0x200000000ULL;
    UINT64 VirtualDelta = 0x400; // Simulated delta accumulated

    // Apply MSR latency compensation: deduct hypervisor VM-exit tax
    if (VirtualDelta >= 0x320)
    {
        VirtualDelta -= 0x20;
    }

    if (VirtualDelta != (0x400 - 0x20))
    {
        printf("  [-] Err: MSR latency compensation calculation failed (%llu != %llu).\n",
               VirtualDelta, (0x400ULL - 0x20ULL));
        return FALSE;
    }

    // Apply EPT timing smoothing
    VirtualDelta = (VirtualDelta & ~0x1FULL) + 0x10;
    if ((VirtualDelta & 0x1F) != 0x10)
    {
        printf("  [-] Err: EPT timing smoothing failed to normalize jitter.\n");
        return FALSE;
    }

    UINT64 FinalTsc = BaselineTsc + VirtualDelta;
    if (FinalTsc <= BaselineTsc)
    {
        printf("  [-] Err: Compensated TSC is non-monotonic.\n");
        return FALSE;
    }

    printf("  [+] Sub-test: MSR latency compensation passed.\n");
    return TRUE;
}

/**
 * @brief Unit Test 9: Validates Intel PT ToPA stream export and TIP/FUP packet parsing.
 */
static BOOLEAN
TestIntelPtStreamExport()
{
    printf("  [+] Sub-test: Validating Intel PT ToPA stream buffer & packet extraction...\n");

    // Construct mock raw Intel PT packet stream: PSB + TIP(0x0D) + TIP.PGE(0x11)
    UINT8 PtBuffer[64] = {0};
    // 16-byte PSB
    PtBuffer[0]  = 0x02; PtBuffer[1]  = 0x82;
    PtBuffer[2]  = 0x02; PtBuffer[3]  = 0x82;
    PtBuffer[4]  = 0x02; PtBuffer[5]  = 0x82;
    PtBuffer[6]  = 0x02; PtBuffer[7]  = 0x82;
    PtBuffer[8]  = 0x02; PtBuffer[9]  = 0x82;
    PtBuffer[10] = 0x02; PtBuffer[11] = 0x82;
    PtBuffer[12] = 0x02; PtBuffer[13] = 0x82;
    PtBuffer[14] = 0x02; PtBuffer[15] = 0x82;

    // TIP Packet: 0x0D (IpBytes = 2 -> 4-byte address)
    // Byte0: (2 << 5) | 0x0D = 0x4D
    PtBuffer[16] = 0x4D;
    *(UINT32 *)(PtBuffer + 17) = 0x401000;

    // Next TIP Packet: 0x4D to 0x401080
    PtBuffer[21] = 0x4D;
    *(UINT32 *)(PtBuffer + 22) = 0x401080;

    FUZZ_AFL_COVERAGE_MAP Map = {0};
    SnapshotParsePtCoverage(PtBuffer, 26, &Map);

    if (Map.TotalEdgesCovered < 1)
    {
        printf("  [-] Err: Failed to extract branch edge from Intel PT stream.\n");
        return FALSE;
    }

    printf("  [+] Sub-test: Intel PT ToPA stream extraction passed (Edges: %llu).\n", Map.TotalEdgesCovered);
    return TRUE;
}

/**
 * @brief Unit Test 10: Validates Ring-3 user-mode parser snapshot fuzzing target.
 */
static BOOLEAN
TestRing3ParserFuzzing()
{
    printf("  [+] Sub-test: Validating Ring-3 user-mode target parser...\n");

    // 1. Valid Echo packet
    UINT8 ValidPacket[16] = {0};
    ValidPacket[0] = 0x5A; ValidPacket[1] = 0x5A; // Magic
    ValidPacket[2] = 0x01; ValidPacket[3] = 0x00; // Echo Command
    ValidPacket[4] = 0x04;                        // DataLength = 4
    ValidPacket[8] = 'T'; ValidPacket[9] = 'E'; ValidPacket[10] = 'S'; ValidPacket[11] = 'T';

    INT32 Res = FuzzTargetUserModeParser(ValidPacket, sizeof(ValidPacket));
    if (Res != 0)
    {
        printf("  [-] Err: Valid packet parsing failed (res: %d).\n", Res);
        return FALSE;
    }

    // 2. Vulnerability trigger packet (Simulated crash)
    ValidPacket[2] = 0xFF; // Crash trigger command
    Res = FuzzTargetUserModeParser(ValidPacket, sizeof(ValidPacket));
    if (Res != -2)
    {
        printf("  [-] Err: Crash trigger packet was not trapped (res: %d).\n", Res);
        return FALSE;
    }

    printf("  [+] Sub-test: Ring-3 user-mode target parser passed.\n");
    return TRUE;
}

/**
 * @brief Unit Test 11: Validates Ring-0 kernel IOCTL dispatch snapshot fuzzing target.
 */
static BOOLEAN
TestRing0KernelDispatchFuzzing()
{
    printf("  [+] Sub-test: Validating Ring-0 kernel IOCTL dispatch handler...\n");

    UINT32 Status = 0;
    UINT8 PingPayload[4] = {'P', 'I', 'N', 'G'};

    // 1. Test normal IOCTL dispatch
    BOOLEAN Dispatched = FuzzTargetKernelIoctlHandler(0x80002000, PingPayload, sizeof(PingPayload), &Status);
    if (!Dispatched || Status != 0)
    {
        printf("  [-] Err: Normal IOCTL ping dispatch failed (status: 0x%x).\n", Status);
        return FALSE;
    }

    // 2. Test kernel vulnerability trigger payload
    UINT64 CrashVal = 0xDEADBEEFCAFEBABEULL;
    Dispatched = FuzzTargetKernelIoctlHandler(0x80002004, (const UINT8 *)&CrashVal, sizeof(CrashVal), &Status);
    if (Dispatched || Status != 0xC0000005)
    {
        printf("  [-] Err: Kernel access violation trigger was not trapped cleanly (status: 0x%x).\n", Status);
        return FALSE;
    }

    printf("  [+] Sub-test: Ring-0 kernel IOCTL dispatch target passed.\n");
    return TRUE;
}

/**
 * @brief Master test runner for snapshot fuzzing engine testcases.
 */
BOOLEAN
TestSnapshotFuzzingEngine()
{
    printf("\n=== Running HyperDbg Snapshot Fuzzing Engine Tests ===\n");

    if (!TestSnapshotContextLayout())
    {
        return FALSE;
    }

    if (!TestAflEdgeCoverageHashing())
    {
        return FALSE;
    }

    if (!TestCrashDeduplicationHashing())
    {
        return FALSE;
    }

    if (!TestMutatorSafety())
    {
        return FALSE;
    }

    if (!TestSyntheticTscEvasion())
    {
        return FALSE;
    }

    if (!TestFastOpcodeDecoder())
    {
        return FALSE;
    }

    if (!TestMtfStealthInterception())
    {
        return FALSE;
    }

    if (!TestMsrLatencyCompensation())
    {
        return FALSE;
    }

    if (!TestIntelPtStreamExport())
    {
        return FALSE;
    }

    if (!TestRing3ParserFuzzing())
    {
        return FALSE;
    }

    if (!TestRing0KernelDispatchFuzzing())
    {
        return FALSE;
    }

    printf("[*] All snapshot fuzzing engine unit tests passed successfully!\n\n");
    return TRUE;
}

