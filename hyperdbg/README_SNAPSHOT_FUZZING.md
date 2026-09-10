# HyperDbg — Hypervisor-Assisted Snapshot Fuzzing & Stealth Platform

> **Date:** 2026-09-10  ·  **Status:** Development Preview  ·  **License:** GNU GPL v3

---

## Table of Contents

1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Hardware Requirements](#hardware-requirements)
4. [Changed & New Files](#changed--new-files)
5. [Public API Reference](#public-api-reference)
6. [IOCTL Interface](#ioctl-interface)
7. [Data Structures](#data-structures)
8. [Coverage Engine](#coverage-engine)
9. [Crash Triage Engine](#crash-triage-engine)
10. [Stealth & Evasion Subsystem](#stealth--evasion-subsystem)
11. [Unit Test Suite](#unit-test-suite)
12. [Applying the Patch](#applying-the-patch)
13. [Build Instructions](#build-instructions)
14. [Known Limitations](#known-limitations)

---

## Overview

This change-set adds a **complete in-hypervisor snapshot fuzzing platform** to HyperDbg. The engine
operates entirely within VMX root mode, eliminating the user-kernel round-trip overhead of
conventional fuzzers. A single fuzzing iteration is:

```
[Snapshot] -> [Inject testcase] -> [Execute guest] -> [Detect crash/timeout] -> [Rollback] -> repeat
```

All operations are driven by a thin IOCTL interface, making the engine drop-in compatible with
AFL++, libFuzzer, and custom harnesses.

**Key capabilities at a glance:**

| Feature | Implementation |
|---|---|
| Full vCPU context snapshot | GPRs, CRs, DRs, 6 segment descriptors, 10 MSRs, XSAVE (AVX-512) |
| Memory rollback | Hardware EPT PML (Page Modification Log) + software CoW shadow pages |
| Code coverage | In-kernel Intel PT ToPA packet decoder -> 64 KB AFL-compatible bitmap |
| Crash deduplication | 32-entry LBR callstack + FNV-1a crash hash, zero BSOD risk |
| Anti-detection | Synthetic TSC time-dilation, hardware MTF single-step, KTRAP_FRAME stealth |
| Batch execution | Autonomous in-kernel loop — thousands of iterations without kernel<->user transitions |

---

## Architecture

```
+-------------------------------------------------------------------------+
|                         User-Mode Harness                               |
|  (AFL++ / libFuzzer / custom)                                           |
|  DeviceIoControl -> IOCTL_SNAPSHOT_TAKE / IOCTL_FUZZ_RUN_BATCH         |
+--------------------------------+----------------------------------------+
                                 | IOCTL (METHOD_BUFFERED)
+--------------------------------v----------------------------------------+
|                      hyperkd  (Windows Driver)                          |
|  Ioctl.c -> DrvDispatchFuzzerIoControl()                                |
+--------------------------------+----------------------------------------+
                                 | Direct call (VMX Root)
+--------------------------------v----------------------------------------+
|             hyperhv  (Hypervisor / VMM -- Ring -1)                      |
|  +--------------------------------------------------------------------+  |
|  | SnapshotFuzzing.c -- Engine Core                                   |  |
|  |                                                                    |  |
|  | SnapshotTake()           <- Save vCPU + arm EPT PML + freeze TSC   |  |
|  | SnapshotFuzzIterate()    <- Inject -> Execute -> Check -> Rollback  |  |
|  | SnapshotRunBatch()       <- Autonomous in-kernel loop               |  |
|  | SnapshotCaptureCrash()   <- Pre-IDT exception + LBR capture        |  |
|  | SnapshotParsePtCoverage() <- Intel PT ToPA -> AFL bitmap            |  |
|  +--------------------------------------------------------------------+  |
|                                                                          |
|  EPT Page Table  -->  PML hardware dirty-page log                        |
|  Intel PT        -->  ToPA ring buffer                                   |
|  LBR MSRs        -->  32-entry last-branch ring                          |
+--------------------------------------------------------------------------+
```

---

## Hardware Requirements

| Requirement | Detail |
|---|---|
| Intel VT-x | **Mandatory** — VMX root / non-root transitions |
| EPT (Extended Page Tables) | **Mandatory** — memory isolation and dirty-page tracking |
| EPT A/D + PML | **Recommended** — hardware dirty-page log (`VMCS_GUEST_PML_INDEX`) |
| Intel PT (Processor Trace) | Optional — ToPA-based branch coverage collection |
| LBR (Last Branch Record) | Optional — crash callstack capture for deduplication |
| AVX / XSAVE | Optional — extended FPU state preservation in snapshot |

- Minimum tested: **Intel Haswell (4th gen)** or newer
- EPT PML support: **Intel Broadwell (5th gen)** or newer
- Intel PT: **Intel Broadwell** or newer

---

## Changed & New Files

### New Files

| File | Lines | Purpose |
|---|---|---|
| `hyperhv/code/features/SnapshotFuzzing.c` | 1,157 | Engine implementation — snapshot, rollback, coverage, crash triage, batch |
| `hyperhv/header/features/SnapshotFuzzing.h` | 87 | Internal VMM-exported function declarations |
| `include/SDK/headers/SnapshotFuzzing.h` | 366 | Public SDK — all shared structs, constants, and IOCTL codes |
| `hyperdbg-test/code/tests/fuzzing-test.cpp` | 400 | 7-test unit and integration test suite |

### Modified Files

| File | Change Summary |
|---|---|
| `hyperdbg-test/code/main.cpp` | Added `TEST_CASE_PARAMETER_FOR_SNAPSHOT_FUZZING` dispatch branch |
| `hyperdbg-test/header/testcases.h` | Added `TestSnapshotFuzzingEngine()` forward declaration |
| `hyperdbg-test/CMakeLists.txt` | Added `fuzzing-test.cpp` to CMake source list |
| `hyperdbg-test/hyperdbg-test.vcxproj` | Added `<ClCompile>` entry for `fuzzing-test.cpp` |
| `hyperdbg-test/hyperdbg-test.vcxproj.filters` | Added Visual Studio filter entries for `fuzzing-test.cpp` |
| `hyperhv/CMakeLists.txt` | Added `SnapshotFuzzing.c` and `SnapshotFuzzing.h` to CMake source list |
| `include/SDK/headers/Ioctls.h` | Added `IOCTL_FUZZER_BASE` offset and all eight fuzzer IOCTL definitions |

---

## Public API Reference

All functions are declared in `hyperhv/header/features/SnapshotFuzzing.h` and exported with
`IMPORT_EXPORT_VMM`.

### Lifecycle

```c
BOOLEAN  SnapshotInitialize();
VOID     SnapshotUninitialize();
BOOLEAN  SnapshotIsActive(VOID);
```

| Function | Description |
|---|---|
| `SnapshotInitialize` | Allocates the AFL coverage map and zeroes all engine state. Called automatically by `SnapshotTake`. |
| `SnapshotUninitialize` | Disarms EPT tracking, frees shadow pages, deallocates the AFL map. Call on driver unload. |
| `SnapshotIsActive` | Returns `TRUE` while a snapshot is armed and the fuzzer loop is running. |

### Snapshot Management

```c
NTSTATUS SnapshotTake(PDEBUGGER_SNAPSHOT_TAKE_REQUEST Request);
NTSTATUS SnapshotRestore(PDEBUGGER_SNAPSHOT_RESTORE_REQUEST Request);
NTSTATUS SnapshotClear(PUINT32 Status);
```

| Function | Description |
|---|---|
| `SnapshotTake` | Captures complete vCPU state (all registers, segments, MSRs, XSAVE), arms EPT PML dirty-page tracking, sets the exception bitmap for `#GP/#PF/#DE/#DF`, and freezes the baseline TSC. |
| `SnapshotRestore` | Rolls back all dirty physical frames via PML or software CoW, restores all vCPU registers, and re-freezes the TSC. |
| `SnapshotClear` | Disarms EPT and exception tracking, frees shadow frames, resets all counters. |

### Fuzzing Execution

```c
NTSTATUS SnapshotFuzzIterate(PDEBUGGER_FUZZ_ITERATE_REQUEST Request);
NTSTATUS SnapshotRunBatch(PDEBUGGER_FUZZ_RUN_BATCH_REQUEST Request);
NTSTATUS SnapshotRunAutonomousBatch(VIRTUAL_MACHINE_STATE *VCpu,
                                    UINT32 IterationCount,
                                    PFUZZ_CRASH_REPORT CrashReport);
```

| Function | Description |
|---|---|
| `SnapshotFuzzIterate` | Injects one mutated testcase, checks for crash, rolls back, increments execution counter. One IOCTL per iteration. |
| `SnapshotRunBatch` | Executes `IterationCount` iterations entirely in-kernel with havoc mutation. Breaks on first crash, reports full telemetry. |
| `SnapshotRunAutonomousBatch` | Runs directly in VMX root mode without IOCTL overhead — maximum throughput mode. |

### Coverage

```c
NTSTATUS SnapshotMapCoverage(PFUZZ_AFL_COVERAGE_MAP DestinationBuffer);
VOID     SnapshotParsePtCoverage(UINT8 *PtBuffer, SIZE_T PtSize,
                                  PFUZZ_AFL_COVERAGE_MAP AflMap);
```

| Function | Description |
|---|---|
| `SnapshotMapCoverage` | Copies the 64 KB AFL bitmap into a user-supplied buffer for harness consumption. |
| `SnapshotParsePtCoverage` | Decodes a raw Intel PT ToPA stream and updates the AFL bitmap using edge hash `(PrevIP ^ (CurIP >> 1)) & 0xFFFF`. |

### Crash Triage

```c
VOID     SnapshotCaptureCrash(VIRTUAL_MACHINE_STATE *VCpu,
                               UINT32 ExceptionVector,
                               PFUZZ_CRASH_REPORT Report);
NTSTATUS SnapshotGetCrashReport(PFUZZ_CRASH_REPORT Report);
NTSTATUS SnapshotClearCrashReport(VOID);
```

| Function | Description |
|---|---|
| `SnapshotCaptureCrash` | Called from the VMX exception handler. Records exception vector, CR2, faulting RIP, hardware error code, 32 LBR entries, and computes the deduplication hash. |
| `SnapshotGetCrashReport` | Returns the last crash report to a caller-supplied buffer. |
| `SnapshotClearCrashReport` | Zeroes the last crash report so the engine is ready for the next iteration. |

### Stealth / Evasion

```c
VOID   SnapshotFreezeTsc(VIRTUAL_MACHINE_STATE *VCpu, UINT64 FrozenTsc);
UINT64 SnapshotGetVirtualizedTsc(VIRTUAL_MACHINE_STATE *VCpu);
VOID   SnapshotSetEvasionMode(UINT32 Mode);
```

---

## IOCTL Interface

All codes are defined in `include/SDK/headers/SnapshotFuzzing.h`.
Base offset: `IOCTL_START_CODE + 0x400`.

| IOCTL | Offset | Transfer Direction | Description |
|---|---|---|---|
| `IOCTL_SNAPSHOT_TAKE` | `+0x01` | In: `DEBUGGER_SNAPSHOT_TAKE_REQUEST` | Create baseline vCPU + memory snapshot |
| `IOCTL_SNAPSHOT_RESTORE` | `+0x02` | Out: `DEBUGGER_SNAPSHOT_RESTORE_REQUEST` | Roll back to baseline |
| `IOCTL_SNAPSHOT_CLEAR` | `+0x03` | None | Release all snapshot resources |
| `IOCTL_FUZZ_ITERATE` | `+0x04` | In/Out: `DEBUGGER_FUZZ_ITERATE_REQUEST` | Execute one fuzzing iteration |
| `IOCTL_FUZZ_MAP_COVERAGE` | `+0x05` | Out: `FUZZ_AFL_COVERAGE_MAP` | Copy shared AFL bitmap to caller |
| `IOCTL_FUZZ_GET_CRASH_REPORT` | `+0x06` | Out: `FUZZ_CRASH_REPORT` | Retrieve last crash telemetry |
| `IOCTL_FUZZ_RUN_BATCH` | `+0x07` | In/Out: `DEBUGGER_FUZZ_RUN_BATCH_REQUEST` | In-kernel autonomous batch loop |
| `IOCTL_FUZZ_CLEAR_CRASH_REPORT` | `+0x08` | None | Reset crash telemetry buffer |

All IOCTLs use `METHOD_BUFFERED` and `FILE_ANY_ACCESS`.

---

## Data Structures

### `SNAPSHOT_VCPU_CONTEXT`

Complete architectural processor snapshot — saved and restored on every fuzzing iteration.

```c
typedef struct _SNAPSHOT_VCPU_CONTEXT {
    // 16x General-Purpose Registers
    UINT64 Rax, Rcx, Rdx, Rbx, Rsp, Rbp, Rsi, Rdi;
    UINT64 R8,  R9,  R10, R11, R12, R13, R14, R15;
    UINT64 Rip, Rflags;

    // Control Registers
    UINT64 Cr0, Cr2, Cr3, Cr4, Cr8;

    // Debug Registers
    UINT64 Dr0, Dr1, Dr2, Dr3, Dr6, Dr7;

    // Segment Registers (selector, base, limit, attributes x8)
    SNAPSHOT_SEGMENT_DESCRIPTOR Cs, Ss, Ds, Es, Fs, Gs, Tr, Ldtr;
    SNAPSHOT_DESCRIPTOR_TABLE   Gdtr, Idtr;

    // 10 Critical MSRs
    UINT64 MsrEfer, MsrStar, MsrLstar, MsrCstar;
    UINT64 MsrSysenterCs, MsrSysenterEsp, MsrSysenterEip;
    UINT64 MsrFsBase, MsrGsBase, MsrKernelGsBase;
    UINT64 MsrTsc, TscOffset;

    // Extended FPU / SSE / AVX / AVX-512 State (XSAVE)
    UINT8  XsaveArea[4096];
} SNAPSHOT_VCPU_CONTEXT;
```

### `SNAPSHOT_MEMORY_TRACKER`

Tracks 4 KB physical shadow page copies used for memory rollback.

```c
typedef struct _SNAPSHOT_MEMORY_TRACKER {
    UINT32                    DirtyCount;
    UINT32                    MaxCapacity;      // Up to 16,384 pages = 64 MB
    BOOLEAN                   AdBitsEnabled;   // TRUE when hardware PML is available
    SNAPSHOT_DIRTY_PAGE_ENTRY DirtyPages[16384];
} SNAPSHOT_MEMORY_TRACKER;
```

Each `SNAPSHOT_DIRTY_PAGE_ENTRY` records:

| Field | Type | Description |
|---|---|---|
| `PhysicalAddress` | `UINT64` | 4 KB-aligned Guest Physical Address (GPA) |
| `PristineShadowVa` | `PVOID` | Kernel VA of pristine 4 KB backup (NonPagedPool) |
| `Pml1EntryVa` | `PVOID` | Pointer to EPT PML1 entry for write-protection re-arming |
| `OriginalPml1Value` | `UINT64` | Original EPT PML1 descriptor bits |

### `FUZZ_AFL_COVERAGE_MAP`

```c
typedef struct _FUZZ_AFL_COVERAGE_MAP {
    UINT8  TraceBits[65536];    // 64 KB AFL-compatible edge bitmap
    UINT64 PreviousIp;          // Last decoded target IP (for edge hashing)
    UINT64 TotalEdgesCovered;   // Unique branch transitions observed
    UINT64 TotalExecutions;     // Total fuzzing iterations executed
} FUZZ_AFL_COVERAGE_MAP;
```

### `FUZZ_CRASH_REPORT`

```c
typedef struct _FUZZ_CRASH_REPORT {
    UINT32                ExceptionVector;    // 13=#GP, 14=#PF, 6=#UD, 8=#DF
    UINT32                HardwareErrorCode;  // CPU-pushed error code
    UINT64                FaultingAddress;    // CR2 value (#PF) or memory operand
    UINT64                FaultingRip;        // RIP at point of fault
    SNAPSHOT_VCPU_CONTEXT RegistersAtCrash;   // Full register dump at fault
    UINT32                LbrEntryCount;      // Number of valid LBR records
    LBR_ENTRY             LbrStack[32];       // Hardware Last-Branch ring
    UINT64                CrashHash;          // FNV-1a deduplication hash
} FUZZ_CRASH_REPORT;
```

### Key Constants

| Constant | Value | Meaning |
|---|---|---|
| `SNAPSHOT_AFL_MAP_SIZE` | `65536` | 64 KB AFL coverage bitmap |
| `SNAPSHOT_MAX_DIRTY_PAGES` | `16384` | Max tracked dirty pages (64 MB total) |
| `SNAPSHOT_MAX_LBR_DEPTH` | `32` | Intel LBR hardware ring depth |
| `SNAPSHOT_MAX_INPUT_SIZE` | `1048576` | 1 MB maximum testcase payload |
| `SNAPSHOT_XSAVE_AREA_SIZE` | `4096` | FPU/AVX/AVX-512 state save buffer |

### Fuzzing Status Codes

| Code | Value | Meaning |
|---|---|---|
| `FUZZ_STATUS_SUCCESS` | `0x00` | Iteration completed without fault |
| `FUZZ_STATUS_CRASH_EXCEPTION` | `0x01` | Hardware exception intercepted |
| `FUZZ_STATUS_TIMEOUT_EXCEEDED` | `0x02` | Iteration exceeded branch/cycle limit |
| `FUZZ_STATUS_MAX_BRANCHES_HIT` | `0x03` | Branch counter saturated |
| `FUZZ_STATUS_INVALID_STATE` | `0x04` | Engine not armed |
| `FUZZ_STATUS_RESTORE_FAILED` | `0x05` | Memory rollback failure |

---

## Coverage Engine

### Intel PT ToPA Packet Decoder

`SnapshotParsePtCoverage()` implements a zero-allocation inline decoder for the binary ToPA ring
buffer produced by Intel PT hardware:

| Packet Type | Byte Signature | Action |
|---|---|---|
| PSB (Packet Stream Boundary) | `0x02 0x82` x8 (16 bytes total) | Resync — skip 16 bytes |
| PAD | `0x00` | Skip |
| TIP / TIP.PGE / FUP | bits `[4:0]` = `0x0D / 0x11 / 0x1D` | Decode target IP, compute edge hash |
| TNT short | bit 0=0, bit 1=1 | Skip (taken/not-taken indicator) |
| TNT long | `0x02 0xA3` | Skip 8 bytes |
| OVF / PSBEND | `0x02 0x43` / `0x02 0x23` | Skip 2 bytes |

**AFL Canonical Edge Hash:**

```
EdgeIndex = (PrevIP ^ (CurrentIP >> 1)) & (AFL_MAP_SIZE - 1)
TraceBits[EdgeIndex]++
if newly seen: TotalEdgesCovered++
```

This 16-bit index into the 64 KB bitmap is identical to the AFL++ / LibAFL edge encoding,
enabling direct integration with existing coverage-guided mutation schedulers.

---

## Crash Triage Engine

### Pre-IDT Exception Interception

The engine arms the VMCS exception bitmap for four hardware faults:

| Vector | Exception | Common Cause |
|---|---|---|
| 0 | `#DE` | Integer divide by zero |
| 8 | `#DF` | Double fault (stack overflow, cascaded fault) |
| 13 | `#GP` | Invalid memory access, ring violation, bad segment |
| 14 | `#PF` | Unmapped page or write to read-only page |

Because the hypervisor intercepts these **before** the CPU delivers them to the IDT, the guest OS
never receives the exception and **never BSODs**. Recovery is immediate:

1. `SnapshotCaptureCrash()` records all state in VMX root mode.
2. Fuzzing loop rolls back to the pristine snapshot.
3. Next iteration begins within microseconds.

### Crash Hash Algorithm (FNV-1a Variant)

```
Hash = 0xCBF29CE484222325          (FNV-1a 64-bit offset basis)

Hash ^= FaultingRip
Hash *= 0x100000001B3              (FNV prime)

Hash ^= (UINT64)ExceptionVector
Hash *= 0x100000001B3

for i in 0 .. min(3, LbrEntryCount-1):
    Hash ^= LbrStack[i].To
    Hash *= 0x100000001B3

CrashReport.CrashHash = Hash
```

Two crashes are considered **distinct** if they differ in faulting `RIP` or in any of the top-4
LBR branch-record targets. This enables automatic deduplication across millions of iterations
without manual triage.

---

## Stealth & Evasion Subsystem

### Synthetic TSC Time Dilation

Many targets detect hypervisors by measuring `RDTSC` delta across a known-latency instruction
sequence. The engine virtualizes the TSC by writing `VMCS_CTRL_TSC_OFFSET`, which biases the
hardware counter without modifying any guest-visible MSR (`RDMSR IA32_TSC` is unaffected):

```c
UINT64 SnapshotGetVirtualizedTsc(VIRTUAL_MACHINE_STATE *VCpu)
{
    if (g_EvasionMode == EVASION_MODE_FIXED_DELTA)
        return g_SnapshotBaselineTsc + (g_VirtualTscDelta += 0x40);  // +64 cycles/query

    if (g_EvasionMode == EVASION_MODE_INSTRUCTION_DILATION)
        return g_SnapshotBaselineTsc + (g_VirtualTscDelta += 0x100); // +256 cycles/query

    return g_SnapshotBaselineTsc;  // Freeze mode: zero advance
}
```

| Mode | Constant | Behavior |
|---|---|---|
| Pass-through | `EVASION_MODE_NONE` | Raw hardware TSC |
| Fixed Delta | `EVASION_MODE_FIXED_DELTA` | +0x40 cycles per `RDTSC` |
| Instruction Dilation | `EVASION_MODE_INSTRUCTION_DILATION` | +0x100 cycles per `RDTSC` |

### Hardware MTF Stealth (KTRAP_FRAME)

Single-step interception uses the **Monitor Trap Flag** (VMX execution-control bit) instead of
the legacy `RFLAGS.TF` approach. This ensures the `TF` bit **never appears** in:

- `R11` saved by the `SYSCALL` instruction
- The `KTRAP_FRAME` structure populated by the NT kernel dispatcher
- ETW, WPP, or WMI trace streams inspected by anti-cheat or anti-debug software

---

## Unit Test Suite

Seven test cases in `hyperdbg-test/code/tests/fuzzing-test.cpp`:

| # | Test Function | What It Validates |
|---|---|---|
| 1 | `TestSnapshotContextLayout` | `SNAPSHOT_VCPU_CONTEXT` struct layout — GPR R/W, CR3, `XsaveArea` size constant |
| 2 | `TestAflEdgeCoverageHashing` | 64 KB bitmap sizing, 3 unique edges from 4 transitions, repeated-edge bucket increment |
| 3 | `TestCrashDeduplicationHashing` | FNV-1a determinism (identical crash -> identical hash), collision freedom (different RIP -> different hash) |
| 4 | `TestMutatorSafety` | 100 in-engine mutations remain non-null and stay within buffer bounds |
| 5 | `TestSyntheticTscEvasion` | Monotonic TSC advance, correct cumulative delta math, zero-advance after snapshot reset |
| 6 | `TestFastOpcodeDecoder` | 256-entry L1 direct-mapped cache — cold miss on first access, hot hit for SYSCALL and SYSRET RIPs |
| 7 | `TestMtfStealthInterception` | `TF` bit (bit 8) absent from `R11` / KTRAP_FRAME under MTF-based single-step |

### Running the Tests

```sh
# CMake build
cmake --build build --target hyperdbg-test

# Execute snapshot fuzzing test suite
hyperdbg-test.exe snapshot-fuzzing

# Expected console output
=== Running HyperDbg Snapshot Fuzzing Engine Tests ===
  [+] Sub-test: Validating SNAPSHOT_VCPU_CONTEXT memory layout...
  [+] Sub-test: SNAPSHOT_VCPU_CONTEXT passed.
  [+] Sub-test: Validating AFL 64KB coverage bitmap edge tracking...
  [+] Sub-test: AFL coverage edge hashing passed.
  [+] Sub-test: Validating crash deduplication FNV-1a hashing...
  [+] Sub-test: Crash deduplication hashing passed.
  [+] Sub-test: Validating input mutation safety...
  [+] Sub-test: Input mutator safety passed.
  [+] Sub-test: Validating synthetic TSC time dilation & freeze...
  [+] Sub-test: Synthetic TSC time dilation passed.
  [+] Sub-test: Validating L1 Opcode Call-Site Cache & Fast Decoding...
  [+] Sub-test: L1 Opcode Call-Site Cache passed.
  [+] Sub-test: Validating Hardware MTF Return Interception & Trap-Frame Stealth...
  [+] Sub-test: Hardware MTF Return Interception passed.
[*] All snapshot fuzzing engine unit tests passed successfully!

[*] The snapshot fuzzing engine test cases passed successfully
```

---

## Applying the Patch

The unified patch file `hyperdbg_unified_fuzzing_stealth_platform.patch` covers every file listed
in [Changed & New Files](#changed--new-files).

```sh
# From the repository root (d:\HyperDbg\hyperdbg)

# Dry-run: shows which files will be changed
git apply --stat hyperdbg_unified_fuzzing_stealth_platform.patch

# Verify it applies cleanly (no errors = good)
git apply --check hyperdbg_unified_fuzzing_stealth_platform.patch

# Apply
git apply hyperdbg_unified_fuzzing_stealth_platform.patch
```

The patch applies cleanly against the upstream `main` branch as of **2026-09-10** (validated
with `git apply --check`, 0 rejections, 0 conflicts).

---

## Build Instructions

### Windows — Visual Studio 2022

1. Open `hyperdbg.sln` in Visual Studio 2022.
2. Select **Release | x64**.
3. Build Solution (`Ctrl+Shift+B`).

Outputs:
- `hyperhv.sys` — hypervisor driver (VMM, Ring -1)
- `hyperkd.sys` — kernel debug driver
- `hyperdbg-test.exe` — test binary

### Windows / Linux — CMake

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)

# Build only the unit test binary (no WDK needed)
cmake --build build --target hyperdbg-test
```

> **Note:** `hyperhv` and `hyperkd` require the **Windows Driver Kit (WDK)**. The `hyperdbg-test`
> target builds on any x86-64 host (Linux, macOS, WSL) for unit-test-only validation without hardware.

---

## Known Limitations

| Limitation | Detail |
|---|---|
| Single-vCPU snapshots | The engine captures and restores the **current** physical CPU only. Full SMP support (per-core snapshot tables) is not yet implemented. |
| 64 MB dirty-page pool | `SNAPSHOT_MAX_DIRTY_PAGES = 16384` x 4 KB = 64 MB of `NonPagedPool`. Targets with large write working sets may exhaust the tracker. |
| Intel PT decoder completeness | Only TIP / TIP.PGE / FUP packets drive edge coverage. Conditional branch granularity (TNT) requires full `pt_insn` decode via libipt. |
| XSAVE area fixed at 4 KB | On CPUs with Intel AMX, the XSAVE area exceeds 8 KB. `SNAPSHOT_XSAVE_AREA_SIZE` must be increased and the buffer re-allocated. |
| No AFL shared-memory transport | Coverage is polled via `IOCTL_FUZZ_MAP_COVERAGE`. A `mmap`-based `__AFL_SHM_ID` zero-copy transport is a planned follow-up. |

---

*HyperDbg Dev Team — 2026-09-10*
