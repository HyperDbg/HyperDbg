---
name: hyperdbg-advanced-debugging
description: Omniscient expert guide, hardware reference, and battle-tested playbook for HyperDbg hypervisor-assisted debugging, Intel VT-x/EPT internals, stealth memory watchpoints, EFER-based syscall interception, privilege transitions, Intel PT/LBR tracing, transparent anti-detection evasion, in-kernel script automation, and high-performance snapshot fuzzing.
---

# HyperDbg: Omniscient Hypervisor-Assisted Debugging Reference & Playbook

HyperDbg operates in **Intel VT-x VMX Root Mode (Ring -1)**. It virtualizes an already-running operating system dynamically on bare metal without requiring reboot or system reconfiguration. By controlling execution below the operating system kernel, HyperDbg provides introspection, instrumentation, and control that completely bypasses Windows Kernel Patch Protection (PatchGuard / KPP), Hypervisor-Protected Code Integrity (HVCI), OS debugging subsystems (`Dbgk`), and commercial anti-cheat/anti-debug heuristics.

---

## 0. Hard Architectural Invariants & Non-Negotiables

### 1. The 4KB EPT Page Exclusivity Law
- **The Invariant**: You must **never** apply `!monitor`, `!epthook`, and `!epthook2` to addresses residing within the **same 4KB physical page frame**.
- **Hardware Mechanism**:
  - `!epthook` configures PML1 bits `Execute=1, Read=0, Write=0` pointing to a shadow physical page containing `0xCC`.
  - `!monitor r/w` clears `Read` or `Write` bits on the *original* physical frame.
  - If placed on the same 4KB page, read violations swap physical frames unexpectedly, corrupting MTF restoration tracking and causing triple faults or missed breaks.
- **Enforcement Formula**:
  ```text
  (PhysicalAddress_A & ~0xFFFULL) != (PhysicalAddress_B & ~0xFFFULL)
  ```

### 2. The Physical Page-In Invariant (GPA Resolution)
- **The Invariant**: EPT operates strictly on **Guest Physical Addresses (GPA)**.
- **The Failure Mode**: If target virtual memory belongs to pageable kernel pool or user space that has been paged to disk or trimmed from the working set, the guest page table walk fails (`PTE.Present = 0`). HyperDbg aborts with `DEBUGGER_ERROR_INVALID_ADDRESS` (0x5).
- **The Protocol**:
  1. In Kernel Debugger (KD) Mode: Force the page into RAM using `.pagein <VirtualAddress>` (injects a targeted `#PF` and sets `RFLAGS.TF` to re-intercept post-fault).
  2. In VMI Mode: Ensure the target routine or buffer has been touched within the target process context before arming the hook.

### 3. Dual-CR3 (KPTI / Meltdown) Isolation Rules
- **The Invariant**: On Windows systems with Kernel Page Table Isolation (KPTI / `nt!KiKvaShadow`), every user process maintains **two distinct CR3 values**:
  - `User CR3` (`CR3 & ~0x1000` or `CR3 | 0x1000`): Maps user-mode memory + minimal trampoline entry stubs (`KiKvaShadow`). System kernel functions cannot be resolved through this CR3.
  - `Kernel CR3`: Maps the full system and kernel address space.
- **The Resolution**: Always use `.process` (Kernel CR3) when translating kernel-mode virtual addresses, and `.process2` when inspecting user-mode memory layouts.

### 4. Calling Stage Laws for Event Mutation
- **The Invariant**:
  - `stage pre`: Fires **before** the instruction executes. Safe for argument inspection, register mutation, or suppressing execution (`event_sc();`).
  - `stage post`: Fires **after** the instruction completes. Safe for inspecting return values (`@rax`), output buffers, or memory side-effects.
- **Fatal Constraint**: Calling `event_sc();` in `stage post` is **invalid and rejected** by the kernel engine, as architectural CPU state has already committed.

### 5. Floating-Point Kernel Hygiene
- **The Invariant**: Event handlers execute in Ring 0 or VMX root mode. Hardware FPU, SSE, and AVX registers (`xmm0`–`xmm15`, `ymm`, `zmm`) must **never** be clobbered without saving and restoring the full `XSAVE`/`XRSTOR` frame.
- **Implementation**: HyperDbg implements a pure software IEEE-754 floating-point emulation engine (`ScriptEngineFormatFixedFloat`, `ScriptEngineFloatBigint`). All script-level float arithmetic is software-emulated and completely safe.

### 6. Interrupt & Spinlock Safety in VMX Root Mode
- **The Invariant**: Code executing in VMX root mode cannot acquire blocking locks, dispatch DPCs, or cause page faults.
- All logging passes through pre-allocated, lock-free, per-core non-paged ring buffers (`hyperlog`), returning packets via pending IRPs without blocking execution.

---

## 1. Intel VT-x Hardware Internals & VMCS Architecture

### 1.1. VMCS Field Classification
Intel VT-x VMCS fields are categorized by encoding width and function:
- **16-bit Control Fields**: `VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS`, `VMCS_CTRL_PRIMARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS`, `VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS`.
- **16-bit Guest/Host Selectors**: Guest `CS`, `SS`, `DS`, `ES`, `FS`, `GS`, `TR`, `LDTR` selectors; Host `CS`, `SS`, `DS`, `ES`, `FS`, `GS`, `TR` selectors.
- **64-bit Control Fields**: `VMCS_CTRL_EPT_POINTER` (EPTP), `VMCS_CTRL_PML_ADDRESS`, `VMCS_CTRL_TSC_OFFSET`, `VMCS_CTRL_MSR_BITMAP_ADDRESS`.
- **32-bit Control Fields**: `VMCS_CTRL_EXCEPTION_BITMAP`, `VMCS_CTRL_PAGE_FAULT_ERROR_CODE_MASK`, `VMCS_CTRL_PAGE_FAULT_ERROR_CODE_MATCH`, `VMCS_CTRL_VMEXIT_CONTROLS`, `VMCS_CTRL_VMENTRY_CONTROLS`.
- **Natural-Width Guest State**: `VMCS_GUEST_RIP`, `VMCS_GUEST_RSP`, `VMCS_GUEST_RFLAGS`, `VMCS_GUEST_CR0`, `VMCS_GUEST_CR3`, `VMCS_GUEST_CR4`, `VMCS_GUEST_DR7`.
- **Natural-Width Host State**: `VMCS_HOST_RIP`, `VMCS_HOST_RSP`, `VMCS_HOST_CR0`, `VMCS_HOST_CR3`, `VMCS_HOST_CR4`.

### 1.2. EPT Paging Bitfield Specification
HyperDbg constructs an identity-mapped 4-level Extended Page Table (PML4 $\to$ PDPT $\to$ PD $\to$ PT/PML1):

| Bit(s) | Field Name | Description in HyperDbg |
| :--- | :--- | :--- |
| **0** | `ReadAccess` | Enables guest read access. Cleared by `!monitor r` and `!epthook`. |
| **1** | `WriteAccess` | Enables guest write access. Cleared by `!monitor w` and `!epthook`. |
| **2** | `ExecuteAccess` | Enables supervisor-mode instruction fetch (or all fetch if MBEC disabled). Set to `1` on fake shadow pages in `!epthook`. |
| **5:3** | `MemoryType` | EPT Memory Type: `0` = Uncacheable (UC), `6` = Write-Back (WB). Derived from hardware MTRRs. |
| **6** | `IgnorePAT` | Ignores guest PAT and uses EPT memory type. |
| **7** | `LargePage` | Set to `1` in PML2 for 2MB pages; `1` in PML3 for 1GB pages. Must be `0` in PML1. |
| **8** | `Accessed` | Hardware set to `1` whenever page is read, written, or executed. |
| **9** | `Dirty` | Hardware set to `1` whenever page is written to (requires EPT A/D secondary control). Used for **snapshot dirty logging**. |
| **10** | `UserModeExecute` | Mode-Based Execution Control (MBEC). Enables user-mode (Ring 3) instruction fetch independently of supervisor fetch. Used by `!mode`. |
| **51:12** | `PageFrameNumber` | Physical address bits 51:12 (`PFN << 12`). Points to original or fake shadow page frame. |
| **63** | `SuppressVE` | Suppresses `#VE` (Virtualization Exception) on EPT violations when `#VE` control is enabled. |

### 1.3. The VM-Exit State Machine & Assembly Trampoline
When an intercepted event fires:
```text
Guest Non-Root Execution
         │
         ▼ (Interception: EPT Violation, #UD, CPUID, MSR, MTF, etc.)
CPU Hardware Context Save to VMCS Guest State Area
Host Context Loaded from VMCS Host State Area (CR0, CR3, CR4, RSP, RIP)
         │
         ▼
[hyperhv/code/assembly/AsmVmexitHandler.asm]
  1. Save GPRs to Host Stack: push rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi, r8-r15
  2. Save Guest XMM registers
  3. mov rcx, rsp (Pass pointer to GUEST_REGS as Arg 1)
  4. call VmxVmexitHandler
         │
         ▼
[hyperhv/code/vmm/vmx/Vmexit.c]
  1. Read VMCS_EXIT_REASON and VMCS_EXIT_QUALIFICATION
  2. Lookup and dispatch to specialized handler (EptHandleEptViolation, SyscallHook, etc.)
  3. Execute registered In-Kernel Scripts
  4. If VCpu->IncrementRip == TRUE:
       VMCS_GUEST_RIP += VMCS_VMEXIT_INSTRUCTION_LENGTH
         │
         ▼
[hyperhv/code/assembly/AsmResumeVm.asm]
  1. Restore Guest XMM registers
  2. Restore GPRs: pop r15-r8, rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax
  3. __vmx_vmresume
```

---

## 2. Core Operating Topologies & Multi-Core Freezing

### 2.1. Operating Modes
| Mode | Command | System State on Trigger | Intended Use Case |
| :--- | :--- | :--- | :--- |
| **VMI Mode** | `load vmm` | **Live / Non-Halting**. Target OS continues running at native speed. | Stealth telemetry, malware logging, un-pauseable production systems. |
| **KD Mode** | `load kd` | **Frozen / Synchronous**. Multi-core NMI halt stops the entire OS. | Interactive reversing, stepping (`p`, `t`, `i`), register edits, rootkit analysis. |

### 2.2. The Synchronous Multi-Core Freeze Protocol
When an event hits or `pause();` is called in KD Mode:
```text
[Triggering Core]                          [All Other Logical Cores]
      │                                                │
      ├─► Designate MainDebuggingCore                  │
      ├─► Acquire DbgState->Lock                       │
      ├─► VmFuncNmiBroadcastRequest() ────────────────►│ (Receive NMI)
      │                                                ├─► VM-Exit: EXCEPTION_VECTOR_NMI
      │                                                ├─► Acquire local DbgState->Lock
      │                                                └─► Spin in KdManageSystemHaltOnVmxRoot()
      ▼                                                                │
Interactive Prompt / Script Execution                                  │
      │                                                                │
[User issues 'g' or step]                                              ▼
      ├─► HaltedCoreBroadcastTaskAllCores() ──────────►│ (Acknowledge & Release Locks)
      ├─► Release DebuggerHandleBreakpointLock         │
      └─► VMRESUME ───────────────────────────────────►└─► VMRESUME
```

### 2.3. Remote Connectivity Matrix
```text
# TCP Network Debugging (Fast, Recommended for VMs)
Host (Debugger):        .listen 50000
Guest (Target Debuggee):.connect 192.168.1.100 50000

# Serial Hardware Debugging (Bare-Metal, Comms survive network stack crashes)
Host:                   .listen serial com1 baudrate 115200
Guest:                  .connect serial com1 baudrate 115200

# Named Pipe (VMware / VirtualBox)
Host:                   .listen namedpipe \\.\pipe\hyperdbg_pipe
Guest:                  .connect namedpipe \\.\pipe\hyperdbg_pipe
```

---

## 3. Universal Event Command Grammar & Parameter Matrix

Every event command conforms to the universal syntax:
```text
!<command> [target] [qualifiers] [calling-options] [condition { ... }] [script { ... }]
```

### Qualifiers
- `pid <HexPID>`: Applies hook only when current process matches `HexPID` (e.g., `pid 0x1f4`).
- `core <CoreID>`: Restricts trigger to a specific logical processor core index (e.g., `core 0`).
- `imm <0|1>`: Immediate message delivery (`imm 1` bypasses message batching queues).
- `stage <pre|post>`: Execution phase (`stage pre` before instruction; `stage post` on return).
- `buffer <HexAddr>`: Custom memory pointer accessible inside the script via `$buffer`.
- `condition { <bool_expr> }`: Fast VMX-root predicate. If false, skips script evaluation immediately without logging overhead.
- `script { <script_block> }`: In-kernel procedural bytecode executed on match.

---

## 4. The Hypervisor Event Arsenal

### 4.1. Hidden Breakpoints: `!epthook` & `!epthook2`

#### Classic Hidden Hook (`!epthook`)
Decouples Execution from Read/Write access via EPT PML1 bits. Scanners reading the page see original bytes; the CPU executing the page runs fake bytes with `0xCC`.
```text
!epthook <AddressOrSymbol> [pid <pid>] [script { ... }]
```
*How MTF Restores Page Integrity on Read/Write:*
```text
1. Guest performs memory read on hooked address (e.g. integrity checksum / GetModuleHandle).
2. EPT Violation fires (Read permission missing on fake page).
3. Hypervisor intercepts violation -> swaps EPT PML1 back to Original Clean Page.
4. Sets VMCS Monitor Trap Flag (MTF = 1) -> issues VMRESUME.
5. CPU executes exactly ONE read instruction -> reads authentic unmodified byte.
6. MTF VM-Exit fires immediately -> Hypervisor restores Fake Shadow Page -> clears MTF -> issues VMRESUME.
Result: Scanner observes 0x00% modified bytes. Zero detection.
```

#### Inline Fast Trampoline Detour (`!epthook2`)
Installs an in-guest jmp detour on the shadow page. Executes a detour handler in VMX non-root mode.
```text
!epthook2 <TargetAddress> <DetourTrampolineAddress>
```
*Selection Rule*: Use `!epthook` for stealth inspection; use `!epthook2` when monitoring functions called $>10,000$ times per second (e.g., `RtlAllocateHeap`).

---

### 4.2. Unlimited Memory Watchpoints: `!monitor`

Emulates hardware watchpoints across arbitrary address spans and struct boundaries without debug registers ($DR0$-$DR3$).

```text
!monitor [r|w|rw|x] <Address> [size <Bytes>] [pid <pid>] [script { ... }]
```
- `r`: Read watchpoint (PML1 `Read=0`).
- `w`: Write watchpoint (PML1 `Write=0`).
- `rw`: Access watchpoint (PML1 `Read=0, Write=0`).
- `x`: Execution watchpoint (PML1 `Execute=0`).
- `size <Bytes>`: Size of region in bytes. If region spans multiple 4KB pages, HyperDbg automatically hooks all intersecting physical frames.

*Physical Memory Variant*:
```text
!monitor [r|w|rw|x] phys <PhysicalAddress> [size <Bytes>] [script { ... }]
```

---

### 4.3. EFER-Based System Call Interception: `!syscall` & `!sysret`

Does **not** hook SSDT, SSDT Shadow, or MSR `IA32_LSTAR`. It clears bit 0 (`SyscallEnable` / SCE) in `GUEST_IA32_EFER`.
- Whenever user-mode executes `syscall` (`0F 05`), the CPU encounters `EFER.SCE = 0` and raises an Undefined Opcode exception (`#UD`).
- VMCS Exception Bitmap intercepts `#UD`.
- HyperDbg checks the opcode: if `0F 05`, it runs scripts, then architecturally emulates the syscall entry (`RCX` $\leftarrow$ `RIP`, `R11` $\leftarrow$ `RFLAGS`, `RIP` $\leftarrow$ `LSTAR`).

```text
!syscall [pid <pid>] [script { ... }]
!sysret [pid <pid>] [script { ... }]
```
*Syscall Number Mapping*: The System Service Number (SSN) is stored in `@rax`.

---

### 4.4. Privilege Boundary & Heaven's Gate: `!mode`

Intercepts transitions between User Mode (Ring 3) and Kernel Mode (Ring 0).
- Utilizes hardware **Mode-Based Execution Control (MBEC)**:
  - Disabling User-Mode Execute (bit 10 of EPT PML4) traps User $\to$ Kernel transitions (`uk`).
  - Disabling Supervisor-Mode Execute (bit 2 of EPT PML4) traps Kernel $\to$ User transitions (`ku`).
- In KD Mode, you **must run `preactivate`** prior to running `!mode`.

```text
preactivate
!mode [uk|ku] [pid <pid>] [script { ... }]
```

*Tracking Heaven's Gate (WoW64 32-bit to 64-bit transition)*:
```text
!mode uk pid 0x420 script {
    if (@cs == 0x33) {
        printf("[*] Heaven's Gate entered! 64-bit RIP: %p, Return RSP: %p\n", @rip, @rsp);
    }
}
```

---

### 4.5. Low-Level Hardware & Peripherals Interception

```text
# 1. CPUID Interception (Spoof features or hypervisor presence)
!cpuid [function <LeafHex>] [pid <pid>] [script { ... }]

# 2. Model-Specific Registers (RDMSR / WRMSR)
!msrread <MSR_Address_Hex> [script { ... }]
!msrwrite <MSR_Address_Hex> [script { ... }]
# Examples: 0xC0000082 (LSTAR), 0xC0000080 (EFER), 0xC0000100 (FS_BASE), 0xC0000101 (GS_BASE)

# 3. Timing & Profiling Attacks (RDTSC / RDTSCP / RDPMC)
!tsc [script { ... }]
!pmc <CounterIndex> [script { ... }]

# 4. Debug Register Access (MOV DRx, reg)
!dr [read|write] [register <dr0..dr7>] [script { ... }]

# 5. Port-Mapped I/O (PMIO)
!ioin [port <PortHex>] [script { ... }]
!ioout [port <PortHex>] [script { ... }]

# 6. Interrupts & Exceptions (IDT Interception)
!exception [vector <0..31>] [script { ... }]
!interrupt [vector <32..255>] [script { ... }]

# 7. Extended State Mutation
!xsetbv [script { ... }]

# 8. SMM Interception
!smi [script { ... }]
```

---

## 5. Hardware Tracing: Intel PT & LBR

### 5.1. Last Branch Record (LBR)
Captures the last 16 to 32 taken branches in dedicated hardware MSRs (`MSR_LASTBRANCH_x_FROM_IP` / `TO_IP`):
```text
# Enable LBR filtering (0x1 = User branches, 0x2 = Kernel branches)
!lbr 0x1

# Dump recent branch frames
!lbrdump
```

*Invoking LBR inside Event Scripts*:
```text
lbr_save();                  # Freezes current hardware branch ring
lbr_print();                 # Emits branch list (From-IP -> To-IP)
lbr_check();                 # Validates if LBR entries exist
lbr_restore();               # Restores hardware LBR state
lbr_restore_by_filter(0x1);  # Restores state applying filter mask
```

### 5.2. Intel Processor Trace (Intel PT)
Streams binary-encoded execution packets (TNT, TIP, FUP) directly to ToPA (Table of Physical Addresses) memory pages:
```text
# 1. Initialize ToPA ring buffers for target process
!pt enable pid 0x4a0

# 2. Start hardware logging
!pt start

# 3. Stop logging
!pt stop

# 4. Dump buffer to file for offline decoding via libipt / Ghidra
!pt dump C:\traces\run.pt
```

---

## 6. HyperEvade: Anti-Detection Engineering (`!hide`)

Activating transparent mode (`!hide`) counters all state-of-the-art hypervisor and debugger detection methods:

```text
!hide
```

### 1. RDTSC VM-Exit Latency Compensation
- **The Threat**: Detection loops execute consecutive `rdtsc` instructions. A native gap is ~20 cycles; a VM-exit forces a latency of 500–1500 cycles.
- **The Mitigation**: HyperEvade subtracts the hypervisor VM-exit cycle overhead from the TSC value returned to the guest:
  $$\text{TSC}_{\text{spoofed}} = \text{TSC}_{\text{hardware}} - \Delta\text{Cycles}_{\text{VM-Exit}}$$

### 2. CPUID Hypervisor Signature Scrubbing
- **The Threat**: Applications query CPUID Leaf `0x1` (ECX bit 31 = Hypervisor Present) and Leaf `0x40000000` (Vendor signature).
- **The Mitigation**: HyperEvade clears ECX bit 31 and overwrites hypervisor signatures with legitimate Intel/AMD CPU vendor strings.

### 3. System Call Query Sanitization
- Intercepts `NtQuerySystemInformation` queries for:
  - `SystemKernelDebuggerInformation`: Forces `DebuggerNotPresent = TRUE`, `DebuggerEnabled = FALSE`.
  - `SystemCodeIntegrityInformation`: Clears test-signing / driver signing bypass flags.
  - Device/Driver Object Walks: Strips `\Device\HyperDbgDriver` from directory queries.

---

## 7. Complete In-Kernel Scripting Engine Manual

### 7.1. Registers & Contextual Pseudo-Registers
| Register | Representation | Description |
| :--- | :--- | :--- |
| **GPRs** | `@rax`, `@rbx`, `@rcx`, `@rdx`, `@rsi`, `@rdi`, `@rsp`, `@rbp`, `@r8`-`@r15` | Architectural General-Purpose 64-bit registers. |
| **Sub-Regs**| `@eax`, `@ax`, `@al`, `@ah`, `@r8d`, `@r8w`, `@r8b` | 32-bit, 16-bit, and 8-bit sub-register views. |
| **Instruction** | `@rip` | Current Instruction Pointer. |
| **Flags** | `@rflags`, `@eflags` | CPU status and control flags register. |
| **Segments** | `@cs`, `@ds`, `@es`, `@fs`, `@gs`, `@ss` | 16-bit Segment selectors. |
| **Control Regs** | `@cr0`, `@cr2`, `@cr3`, `@cr4`, `@cr8` | Machine state & virtual memory root registers. |
| **`$pid`** | Pseudo-register | Current Process ID (from `EPROCESS.UniqueProcessId`). |
| **`$tid`** | Pseudo-register | Current Thread ID (from `ETHREAD.Cid.UniqueThread`). |
| **`$pname`** | Pseudo-register | ASCII string of the running process image name. |
| **`$proc`** | Pseudo-register | Pointer to current NT `EPROCESS` block. |
| **`$thread`** | Pseudo-register | Pointer to current NT `ETHREAD` block. |
| **`$peb`** | Pseudo-register | Pointer to current Process Environment Block (PEB). |
| **`$teb`** | Pseudo-register | Pointer to current Thread Environment Block (TEB). |
| **`$ip`** | Pseudo-register | Synonymous with `@rip`. |
| **`$core`** | Pseudo-register | Numerical index of the currently executing logical processor. |
| **`$buffer`** | Pseudo-register | Pointer passed via the `buffer <addr>` command qualifier. |
| **`$context`**| Pseudo-register | Context tag identifier associated with the event. |

---

### 7.2. Memory Access & Mutation Functions

```text
# Virtual Memory Reads (Target Process Context)
poi(addr)                 # Dereference 64-bit pointer: *(UINT64*)(addr)
db(addr)                  # Read 1 byte (UINT8)
dw(addr)                  # Read 2 bytes (UINT16)
dd(addr)                  # Read 4 bytes (UINT32)
dq(addr)                  # Read 8 bytes (UINT64)

# Virtual Memory Writes (Target Process Context)
eb(addr, val)             # Write 1 byte
ew(addr, val)             # Write 2 bytes
ed(addr, val)             # Write 4 bytes
eq(addr, val)             # Write 8 bytes

# Physical Memory Reads/Writes (Direct RAM Access)
db_pa(paddr)              # Read 1 byte from physical address
dq_pa(paddr)              # Read 8 bytes from physical address
eb_pa(paddr, val)         # Write 1 byte to physical address
eq_pa(paddr, val)         # Write 8 bytes to physical address

# Address Translation & Range Validation
check_address(addr, len)  # Returns 1 if virtual address range is mapped & safe; 0 otherwise
va2pa(vaddr)              # Translates Virtual Address to Physical Address (via current CR3)
pa2va(paddr)              # Translates Physical Address to Virtual Address
memcpy(dst, src, len)     # Safely copies bytes between virtual addresses
memcpy_pa(dst, src, len)  # Safely copies bytes between physical addresses
```

---

### 7.3. Concurrency, Execution & Event Controls

```text
# Console Logging & Formatting
printf("Format string: %llx, %p, %d, %s, %ws\n", arg1, arg2, ...);
print(expr);                                  # Evaluates and dumps expression value in hex

# Instruction Manipulation
event_sc();                                   # Short-circuit triggering event. Suppresses original instruction.
                                              # MUST be called in stage pre!
pause();                                      # Halts all CPU cores and drops into interactive KD prompt.
disassemble_len(addr);                        # Returns length in bytes of instruction at addr (64-bit)
disassemble_len32(addr);                      # Returns length in bytes of instruction at addr (32-bit)

# Interrupt / Exception Injection
event_inject(type, vector);                   # Injects interrupt/exception into guest
event_inject_error_code(type, vector, err);   # Injects exception with explicit hardware error code

# Dynamic Event State Management
event_enable(tag_id);                         # Arms an inactive event by ID
event_disable(tag_id);                        # Disarms an active event by ID
event_clear(tag_id);                          # Unregisters and deallocates event resources

# Concurrency & Interlocked Operations
interlocked_increment(&val);                  # Atomic 64-bit increment
interlocked_decrement(&val);                  # Atomic 64-bit decrement
interlocked_exchange(&dst, new_val);          # Atomic 64-bit swap
interlocked_compare_exchange(&dst, ex, comp); # Atomic 64-bit CAS
spinlock_lock(&lock);                         # Acquires spinlock in kernel memory
spinlock_unlock(&lock);                       # Releases spinlock in kernel memory

# Timing & Delays
rdtsc();                                      # Returns raw 64-bit cycle counter
rdtscp();                                     # Returns serialized cycle counter
microsleep(us);                               # Delays core execution for specified microseconds

# String Operations (VMX Root Safe)
strlen(addr);                                 # Length of ASCII string
wcslen(addr);                                 # Length of Unicode string
strcmp(s1, s2);                               # Case-sensitive ASCII comparison
strncmp(s1, s2, max_len);                     # Bounded ASCII comparison
wcscmp(s1, s2);                               # Unicode comparison
wcsncmp(s1, s2, max_len);                     # Bounded Unicode comparison
memcmp(s1, s2, count);                        # Raw memory byte comparison
```

---

## 8. Masterclass Hypervisor Snapshot Fuzzing Engine

HyperDbg's snapshot fuzzing engine enables high-throughput (**1,500 – 10,000 executions/second**) fuzzing of ring 0 drivers, anti-cheat modules, and protected ring 3 processes directly on bare-metal hardware.

```text
               ┌────────────────────────────────────────────────────────┐
               │              LibAFL / Python Host Engine               │
               └───────────────────────────┬────────────────────────────┘
                                           │ IOCTL_FUZZ_ITERATE
                                           ▼
┌──────────────────────────────────────────────────────────────────────────────────────┐
│ HyperDbg Kernel Driver (Ring 0 / VMX Root)                                           │
│                                                                                      │
│ 1. INJECT INPUT        Write mutated testcase directly to TargetVirtualAddress       │
│ 2. VMENTRY             Resume guest non-root execution at target function entry      │
│ 3. HARDWARE TRACKING   • EPT A/D (Bit 9) / PML marks modified pages with ZERO exits  │
│                        • Intel PT streams branch packets directly to ToPA buffers    │
│ 4. LOOP TERMINATION    Hit Exit Breakpoint OR Exception Trap (#GP/#PF/#UD)           │
│ 5. REVERT STATE        • Roll back vCPU context (GPRs, RSP, RIP, CR3, MSRs, XSAVE)   │
│                        • Copy back dirty 4KB frames from shadow pool & clear Bit 9   │
│                        • Invalidate EPT TLB via INVEPT                               │
│ 6. EXTRACT COVERAGE    Parse ToPA packets -> update shared 64KB AFL trace_bits map   │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 8.1. High-Speed vCPU Context Snapshotting & Restoration

To create and restore execution snapshots with microsecond latency, the hypervisor captures and restores every architectural register, control register, segment descriptor, debug register, MSR, and extended `XSAVE` frame using dedicated routines:

```c
/**
 * @brief Captures the complete architectural and extended vCPU state into Context.
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
```

---

### 8.2. Hardware EPT Access/Dirty Tracking & Zero-Exit Memory Rollback

When Page Modification Logging (PML) and EPT Access/Dirty bits are enabled (`EPTP` bit 6 = 1), guest writes automatically set **Bit 9 (Dirty flag)** in the PML1 entry without causing VM-exits.

```c
/**
 * @brief Records a pristine shadow copy of a physical 4KB frame prior to mutation.
 */
BOOLEAN
SnapshotRecordPristinePage(PSNAPSHOT_MEMORY_TRACKER Tracker, UINT64 PhysicalAddress)
{
    UINT64 AlignedGpa = PhysicalAddress & ~0xFFFULL;

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

    PVOID ShadowFrame = PlatformMemAllocateNonPagedPool(PAGE_SIZE);
    if (ShadowFrame == NULL)
    {
        LogError("Err, failed to allocate shadow memory frame!");
        return FALSE;
    }

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

            PmlEntry = EptGetPml1OrPml2Entry(VCpu->EptPageTable, Gpa, &IsLargePage);
            if (PmlEntry != NULL && !IsLargePage)
            {
                ((PEPT_PML1_ENTRY)PmlEntry)->WriteAccess = FALSE;
            }
        }
    }

    //
    // 3. Flush EPT TLB via INVEPT
    //
    INVEPT_DESCRIPTOR InveptDesc = {0};
    InveptDesc.EptPointer        = VCpu->EptPointer.Flags;
    __vmx_invept(INVEPT_SINGLE_CONTEXT, &InveptDesc);

    return TRUE;
}
```

---

### 8.3. In-Kernel Intel PT ToPA Edge Coverage Engine

HyperDbg maps a 64KB AFL-compatible coverage bitmap into user space via `IOCTL_FUZZ_MAP_COVERAGE`. Hardware IP filtering (`IA32_RTIT_ADDR0_A`/`B`) eliminates Windows kernel noise, capturing only branches inside the fuzz target:

```c
/**
 * @brief Parses Intel PT ToPA packet stream and updates the 64KB AFL-compatible coverage map.
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
                Offset += 8;
                continue;
            }
            else if (Byte1 == 0x43 || Byte1 == 0x23)
            {
                Offset += 2;
                continue;
            }
        }

        Offset++;
    }

    AflMap->PreviousIp = PrevIp;
}
```

---

### 8.4. Synthetic TSC Time Dilation & Latency Compensation

To prevent target code and anti-cheat drivers from detecting execution stalls during fuzz iterations, HyperDbg freezes the virtual TSC:

$$\text{TSC}_{\text{Guest}} = \text{TSC}_{\text{SnapshotBase}} + (\text{IterationCycles} \times \text{DilationFactor})$$

In VMX root mode, the hypervisor modulates `VMCS_CTRL_TSC_OFFSET`:
```c
VOID
SnapshotFreezeTsc(VIRTUAL_MACHINE_STATE * VCpu, UINT64 FrozenTsc)
{
    UINT64 HardwareTsc  = __rdtsc();
    UINT64 TargetOffset = FrozenTsc - HardwareTsc;
    VmxVmwrite64(VMCS_CTRL_TSC_OFFSET, TargetOffset);
}
```

---

### 8.5. Automated IDT Crash Triage & Deduplication Engine

Fatal hardware exceptions (`#GP` vector 13, `#PF` vector 14, `#UD` vector 6, `#DF` vector 8) are trapped by the VMCS Exception Bitmap:

```c
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
```

---

### 8.6. Kernel IOCTL Dispatcher Implementation

```c
NTSTATUS
SnapshotFuzzerIoctlDispatcher(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION IrpStack  = IoGetCurrentIrpStackLocation(Irp);
    ULONG              IoctlCode = IrpStack->Parameters.DeviceIoControl.IoControlCode;
    PVOID              Buffer    = Irp->AssociatedIrp.SystemBuffer;
    ULONG              InLength  = IrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG              OutLength = IrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    NTSTATUS           Status    = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(DeviceObject);

    switch (IoctlCode)
    {
    case IOCTL_SNAPSHOT_TAKE:
    {
        // Broadcast VMCALL to all cores to freeze execution and capture baseline
        BroadcastSnapshotTakeAllCores();
        Irp->IoStatus.Information = 0;
        break;
    }
    case IOCTL_SNAPSHOT_RESTORE:
    {
        // Broadcast VMCALL to rollback vCPU registers and dirty memory pages
        BroadcastSnapshotRestoreAllCores();
        Irp->IoStatus.Information = 0;
        break;
    }
    case IOCTL_FUZZ_ITERATE:
    {
        PDEBUGGER_FUZZ_ITERATE_REQUEST Req = (PDEBUGGER_FUZZ_ITERATE_REQUEST)Buffer;
        if (InLength < sizeof(DEBUGGER_FUZZ_ITERATE_REQUEST))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        // Execute single iteration with input injection & hardware trace
        Status = FuzzerExecuteSingleIteration(Req);
        Irp->IoStatus.Information = sizeof(DEBUGGER_FUZZ_ITERATE_REQUEST);
        break;
    }
    case IOCTL_FUZZ_MAP_COVERAGE:
    {
        // Map AFL 64KB shared memory bitmap to user space via MDL
        Status = FuzzerMapCoverageToUser(Irp, Buffer, OutLength);
        break;
    }
    case IOCTL_FUZZ_GET_CRASH_REPORT:
    {
        if (OutLength < sizeof(FUZZ_CRASH_REPORT))
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        RtlCopyMemory(Buffer, &g_LastCrashReport, sizeof(FUZZ_CRASH_REPORT));
        Irp->IoStatus.Information = sizeof(FUZZ_CRASH_REPORT);
        break;
    }
    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return Status;
}
```

---

### 8.7. Production Python LibAFL Harness Bridge

```python
import ctypes
from ctypes import wintypes
import os
import sys
import time

FILE_DEVICE_UNKNOWN = 0x00000022
METHOD_BUFFERED     = 0
FILE_ANY_ACCESS     = 0

def CTL_CODE(DeviceType, Function, Method, Access):
    return (DeviceType << 16) | (Access << 14) | (Function << 2) | Method

IOCTL_FUZZER_BASE   = 0x800 + 0x400
IOCTL_SNAPSHOT_TAKE = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x01, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_SNAPSHOT_RESTORE = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x02, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_FUZZ_ITERATE  = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x04, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_MAP_COVERAGE  = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x05, METHOD_BUFFERED, FILE_ANY_ACCESS)
IOCTL_GET_CRASH     = CTL_CODE(FILE_DEVICE_UNKNOWN, IOCTL_FUZZER_BASE + 0x06, METHOD_BUFFERED, FILE_ANY_ACCESS)

class FUZZ_ITERATE_REQUEST(ctypes.Structure):
    _fields_ = [
        ("TargetVirtualAddress", ctypes.c_uint64),
        ("InputSize", ctypes.c_uint32),
        ("InputBuffer", ctypes.c_uint8 * (1024 * 1024)),
        ("MaxInstructionCount", ctypes.c_uint64),
        ("ExitBreakpointAddress", ctypes.c_uint64),
        ("ExecutionStatus", ctypes.c_uint32),
        ("ElapsedCycles", ctypes.c_uint64),
        ("KernelStatus", ctypes.c_uint32)
    ]

class FUZZ_CRASH_REPORT(ctypes.Structure):
    _fields_ = [
        ("ExceptionVector", ctypes.c_uint32),
        ("HardwareErrorCode", ctypes.c_uint32),
        ("FaultingAddress", ctypes.c_uint64),
        ("FaultingRip", ctypes.c_uint64),
        ("RegistersPadding", ctypes.c_uint8 * 512),
        ("LbrEntryCount", ctypes.c_uint32),
        ("LbrStack", (ctypes.c_uint64 * 2) * 32),
        ("CrashHash", ctypes.c_uint64)
    ]

# 1. Open Handle to HyperDbg Kernel Driver
hDevice = ctypes.windll.kernel32.CreateFileW(
    r"\\.\HyperDbgDriverDevice",
    0xC0000000, 0, None, 3, 0, None
)

if hDevice == -1:
    print("[-] Failed to open HyperDbg driver device handle!")
    sys.exit(1)

# 2. Arm Baseline Snapshot
bytesRet = wintypes.DWORD(0)
ctypes.windll.kernel32.DeviceIoControl(hDevice, IOCTL_SNAPSHOT_TAKE, None, 0, None, 0, ctypes.byref(bytesRet), None)
print("[+] Hardware baseline snapshot armed in memory.")

# 3. Fast High-Throughput Fuzzing Loop
req = FUZZ_ITERATE_REQUEST()
req.TargetVirtualAddress = 0x7FF72001A000   # Target buffer VA
req.ExitBreakpointAddress = 0x7FF72001A320  # Return address
req.MaxInstructionCount = 1000000

total_execs = 0
start_time = time.time()
unique_crashes = set()

for iteration in range(50000):
    # Mutate testcase
    testcase = b"\x41" * 64 + int.to_bytes(iteration, 4, "little")
    req.InputSize = len(testcase)
    ctypes.memmove(req.InputBuffer, testcase, len(testcase))

    # Execute single iteration inside hypervisor
    ctypes.windll.kernel32.DeviceIoControl(
        hDevice, IOCTL_FUZZ_ITERATE,
        ctypes.byref(req), ctypes.sizeof(req),
        ctypes.byref(req), ctypes.sizeof(req),
        ctypes.byref(bytesRet), None
    )
    total_execs += 1

    # Check for crash exception (#GP, #PF, #UD)
    if req.ExecutionStatus == 1:
        crash = FUZZ_CRASH_REPORT()
        ctypes.windll.kernel32.DeviceIoControl(
            hDevice, IOCTL_GET_CRASH, None, 0,
            ctypes.byref(crash), ctypes.sizeof(crash),
            ctypes.byref(bytesRet), None
        )
        if crash.CrashHash not in unique_crashes:
            unique_crashes.add(crash.CrashHash)
            print(f"[!] NEW UNIQUE CRASH DETECTED! Hash: 0x{crash.CrashHash:016X} | RIP: 0x{crash.FaultingRip:016X} | Vector: {crash.ExceptionVector}")

    if total_execs % 5000 == 0:
        elapsed = time.time() - start_time
        print(f"[*] Executions: {total_execs} | Speed: {total_execs / elapsed:.2f} exec/s | Unique Crashes: {len(unique_crashes)}")

ctypes.windll.kernel32.CloseHandle(hDevice)
```

---

## 9. Battle-Tested Reverse Engineering Recipes

### Recipe 1: Unpacking VMProtect 3 / Themida (OEP Hook + LBR Tracing)
*Problem*: Binary uses heavy virtualization, anti-debug checks, and memory checksums. Code is decrypted on heap and jumped to at the Original Entry Point (OEP).
```text
# Step 1: Engage transparent mode
!hide

# Step 2: Configure LBR to capture user-mode jumps
!lbr 0x1

# Step 3: Monitor main .text section for Execution (Read/Write allowed without events)
!monitor x 0x00007ff7`20011000 size 0x40000 pid 0x14f0 script {
    printf("[+] OEP Execution Detected! Target RIP: %p\n", @rip);
    
    # Save hardware branch trace leading to OEP
    lbr_save();
    lbr_print();
    
    # Freeze all cores for memory dump
    pause();
}
```

---

### Recipe 2: Subverting Malware API Hashing & Dynamic Imports
*Problem*: Malware resolves APIs via custom hashing over exported DLL names.
```text
# Place hidden EPT hook on LdrGetProcedureAddress
!epthook ntdll!LdrGetProcedureAddress pid 0x12a0 stage post script {
    # In stage post:
    # R9 points to PVOID* ProcedureAddressOut
    # R8 points to PANSI_STRING ProcedureNameOrOrdinal
    
    if (@rax == 0x0) { # STATUS_SUCCESS
        printf("[*] API Resolved: Ptr=%p, ResolvedAt=%p\n", poi(@r9), @rip);
    }
}
```

---

### Recipe 3: Defeating `NtQueryInformationProcess(ProcessDebugPort)` Anti-Debug
*Problem*: Malware detects debuggers by checking if `ProcessDebugPort` returns non-zero.
```text
!syscall pid 0x18e0 script {
    # RAX = SSN for NtQueryInformationProcess (0x19 on Win10 21H2/22H2)
    # RDX = ProcessInformationClass (7 = ProcessDebugPort)
    # R8  = ProcessInformation Buffer Pointer
    
    if (@rax == 0x19 && @rdx == 0x7) {
        printf("[!] Anti-Debug Intercepted: ProcessDebugPort query by Thread: %x\n", $tid);
        
        # Suppress the syscall from reaching the kernel
        event_sc();
        
        # Zero out the return buffer (0 = No debugger attached)
        eq(@r8, 0);
        
        # If ReturnLength (R10/5th arg) provided, write 8 bytes
        if (@r9 != 0) {
            eq(@r9, 8);
        }
        
        # Return STATUS_SUCCESS in RAX
        @rax = 0x0;
    }
}
```

---

### Recipe 4: Rootkit Hunter: Zero-Trace Kernel Hook on `nt!ExAllocatePoolWithTag`
*Problem*: Trace allocations made by a suspected rootkit without triggering PatchGuard.
```text
!epthook nt!ExAllocatePoolWithTag script {
    # RDX = NumberOfBytes
    # R8  = PoolTag (4 ASCII bytes)
    
    if (@r8 == 'enoN' || @r8 == 'kceH') { # 'None' or 'Heck'
        printf("[!] Suspicious Pool Allocation: Tag=%c%c%c%c, Size=0x%x, Caller=%p\n",
               db(@r8), db(@r8+1), db(@r8+2), db(@r8+3), @rdx, poi(@rsp));
    }
}
```

---

### Recipe 5: Defeating Anti-Cheat RDTSC Timing Loops
*Problem*: Protected anti-cheat binary loops `rdtsc` to calculate cycle jitter.
```text
!tsc script {
    # Advance cycle counter by a fixed increment (0x30 cycles) per interrogation
    # defeating any measurement of hypervisor exit latency
    @rax = @rax + 0x30;
    @rdx = @rdx;
}
```

---

### Recipe 6: Monitoring Direct Memory-Mapped I/O (MMIO) Hardware Accesses
*Problem*: Reverse engineer an undocumented PCIe hardware peripheral interacting via physical MMIO space.
```text
# Physical base address of PCIe device BAR0: 0xFED00000
!monitor rw phys 0xFED00000 size 0x1000 script {
    printf("[*] MMIO Hardware Access: PhysAddr: %p, RIP: %p, WriteValue: %llx\n", 
           @rax, @rip, poi(@rax));
}
```

---

### Recipe 7: Automated Snapshot Fuzzing of a Kernel IOCTL Dispatcher
*Problem*: Fuzz an undocumented driver IOCTL dispatcher (`DeviceControl`) without BSOD crashes or reboots.
```text
# Arm snapshot on the driver's IRP_MJ_DEVICE_CONTROL entry point
!epthook MyVulnerableDriver!DriverDispatchDeviceControl script {
    printf("[*] Target Dispatch Hit! Arming snapshot.\n");
    
    # Take baseline snapshot in memory
    snapshot_take();
    
    # Set exit breakpoint on IofCompleteRequest
    !epthook nt!IofCompleteRequest script {
        # Execution succeeded without fault: restore state for next input
        snapshot_restore();
    }
}

# Trap fatal memory access faults in the driver
!exception vector 14 script {
    if (@rip >= 0xFFFFF800`00000000) {
        printf("[!] KERNEL CRASH DETECTED at RIP: %p, FaultingAddr: %p\n", @rip, @cr2);
        lbr_save();
        lbr_print();
        snapshot_restore();
    }
}
```

---

## 10. Failure Modes & Systematic Diagnostic Tree

```text
[HyperDbg Event / Command Failure]
  │
  ├──► Error: "DEBUGGER_ERROR_INVALID_ADDRESS" (0x5)
  │      ├─ Cause: Target Virtual Address is paged out (PTE.Present = 0).
  │      └─ Fix: Run `.pagein <Address>` in KD Mode, or touch address in process context.
  │
  ├──► Error: "DEBUGGER_ERROR_EPT_COULD_NOT_SPLIT_THE_LARGE_PAGE_TO_4KB_PAGES"
  │      ├─ Cause: Pre-allocated pool for 2MB -> 4KB dynamic page splitting is exhausted.
  │      └─ Fix: Clear inactive events using `events` and `event_clear <id>`. Increase
  │               allocation pool limit in driver configurations.
  │
  ├──► Error: "Address already hooked" / EPT Access Conflict
  │      ├─ Cause: 4KB Page Exclusivity Violation! Two hooks placed on the same 4KB boundary.
  │      └─ Fix: Verify `(Addr1 & ~0xFFF) != (Addr2 & ~0xFFF)`. If hooking adjacent functions
  │               in the same page, hook only the page base or use `!epthook2`.
  │
  ├──► Error: "Preactivate required"
  │      ├─ Cause: Setting high-overhead transition events (`!mode`) in KD mode without arming.
  │      └─ Fix: Run `preactivate` command once prior to registering `!mode`.
  │
  ├──► Error: "event_sc() failed / unexpected behavior"
  │      ├─ Cause: `event_sc()` was called in `stage post`.
  │      └─ Fix: Re-register event with explicit `stage pre` qualifier.
  │
  └──► Error: System Hangs / NMI Watchdog Timeout
         ├─ Cause: Infinite loop inside script, or calling `pause()` inside an ultra-high
         │         frequency event without a restrictive `condition { ... }`.
         └─ Fix: Always add tight conditions: `condition { @rax == 0x1234 }` before scripts.
```
