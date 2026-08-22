# HyperDbg Linux Port — Status & TODO Ledger

This file tracks **what has been changed** for the Linux port and **what is still
stubbed / deferred**, so that when the port compiles end-to-end we have a single
list of the shortcuts that must be revisited before Linux is actually functional.

It complements [`README.md`](README.md) (the contributor how-to). This file is
the *state* of the work; the README is the *method*.

> Status in one line: the userspace library (`libhyperdbg`) compiles file-by-file
> on Linux. Many Windows-only paths are **stubbed to compile+link**, not yet
> implemented. See the TODO ledger below. Work has now also started on the
> **kernel module** (`HyperDbg.ko`) — see its section near the end.

---

## Conventions used in this port

Two patterns, applied consistently:

1. **In-body `#ifdef _WIN32`** — for a *few* Windows-only functions inside an
   otherwise shared file. The Windows body stays; the Linux `#else` returns a
   safe default with a `TODO(Linux)` comment.
   Examples: `DebuggerGetNtoskrnlBase`, the `$peb` pseudo-register, the two
   test harnesses in `script-engine-wrapper.cpp`.

2. **Separate `*-linux.cpp` file + CMake swap** — for a *whole* translation unit
   that is entirely Windows-specific with only a few public entry points. The
   original Windows `.cpp` is left 100% untouched; a Linux stub file implements
   the same public functions; `libhyperdbg/CMakeLists.txt`'s `if(UNIX)` block
   does `list(REMOVE_ITEM ...)` + `list(APPEND ...)` to swap them.
   Examples: `symbol.cpp` → `symbol-linux.cpp`, `pe-parser.cpp` → `pe-parser-linux.cpp`.

Guard style:
- Windows-only code → `#ifdef _WIN32`
- Linux-only additions → `#ifdef __linux__`
- Cross-platform wrappers → `#if defined(_WIN32) / #elif defined(__linux__)` internally

**Golden rule (see README):** don't scatter `#ifdef` through program logic — route
Windows API calls through the platform interface so both OSes share the call site.

---

## Platform interface layer (new)

User-mode abstractions in `include/platform/user/` (`header/` = interface,
`code/` = implementation):

| File | Abstracts | Linux status |
|------|-----------|--------------|
| `platform-lib-calls.{h,c}` | OS lib calls: events, handles, threads, sprintf/vsnprintf, perf counters, get-last-error, process/thread ids & names, OS version, `strnlen`, `DebugBreak`, zero-memory | Mostly implemented; a few stubbed (see TODO) |
| `platform-intrinsics.{h,c}` | CPU ops: `rdtsc`/`rdtscp`, interlocked 64-bit ops, bit-test-and-set | Implemented (GCC builtins) |
| `platform-serial.{h,c}` | Serial byte transport for remote kernel debugging | **Stub** — Linux branch returns false; termios impl TODO |
| `platform-ioctl.{h,c}` | Local kernel-driver IOCTL interface (`PlatformDeviceIoControl`) + device open (`PlatformOpenDevice`) | **Stub** — no Linux kernel module yet; `PlatformOpenDevice` returns `INVALID_HANDLE_VALUE` |
| `platform-signal.{h,c}` | Console control handler (Ctrl-C / Ctrl-Break) | Implemented (blocks signals + `sigwait` thread) |
| `platform-socket.{h,c}` | TCP remote-debugging transport: the few Winsock ops that diverge from POSIX (`WSAStartup`/`WSACleanup` lifecycle, `closesocket`, `SD_SEND` shutdown, `WSAGetLastError`) + the `accept()` length-type (`PLATFORM_SOCKLEN`). Also owns the Linux POSIX socket-header includes | Implemented (BSD sockets) — the portable socket calls stay at the tcpclient/tcpserver call sites |

Kernel-mode equivalents live in `include/platform/kernel/`. Two were extended for
the port because the shared `script-eval/` code compiles in both user and kernel
builds: `PlatformIntrinsics.c` (interlocked 64-bit ops) and `PlatformMem.c`
(`PlatformSprintf`).

Shared, OS-neutral headers:
- `include/platform/general/header/nt-list.h` (new) — NT doubly-linked-list helpers
  (`InitializeListHead`, `InsertHeadList`, `CONTAINING_RECORD`, …) as `static inline`
  for Linux; inert on Windows.
- `include/platform/general/header/Environment.h` — SAL annotations, string typedefs,
  `CTRL_*_EVENT`, `Sleep`, `INFINITE`/`WAIT_OBJECT_0`, `NTAPI`/`WINAPI`, `SOCKET`, etc.
- `include/SDK/headers/BasicTypes.h` — Linux compat typedefs: `WCHAR` (as `UINT16`),
  `LARGE_INTEGER`, `PSIZE_T`, `LONGLONG`, and pointer aliases (`PLONG`, `PULONG`,
  `PDWORD`, `PUCHAR`, …).

---

## Linux-only replacement files (stubs)

| File | Replaces | What it stubs | TODO to make real |
|------|----------|---------------|-------------------|
| `.../script-engine/symbol-linux.cpp` | `symbol.cpp` (DbgHelp + PDB) | All `Symbol*` functions. Only `SymbolConvertNameOrExprToAddress` does real work: parses a plain hex/decimal literal so numeric addresses work. | Real ELF/DWARF symbol parser (libdw / libelf / libbfd). |
| `.../user-level/pe-parser-linux.cpp` | `pe-parser.cpp` (Windows PE format) | The 3 public fns: `PeShowSectionInformationAndDump`, `PeIsPE32BitOr64Bit` (→ FALSE), `PeGetSyscallNumber` (→ 0). | Recreate the Windows `IMAGE_*` headers for Linux, then port `pe-parser.cpp`. Only needed for Windows-target debugging on Linux. |
| `.../driver-loader/install-linux.cpp` | `install.cpp` (SCM driver loader) | Only `ManageDriver` (→ FALSE) is still a stub; `SetupPathForFileName` is **implemented** (`readlink("/proc/self/exe")` + strip + append + `access()`). The 4 `SC_HANDLE` helpers (`InstallDriver`/`RemoveDriver`/`StartDriver`/`StopDriver`) are guarded out of `install.h` on Linux (never referenced there). | `ManageDriver`: load/unload a future HyperDbg Linux kernel module via `finit_module`/`delete_module` (needs CAP_SYS_MODULE). |
| `.../communication/namedpipe-linux.cpp` | `namedpipe.cpp` (Win32 named-pipe IPC) | All 10 public `NamedPipeServer*`/`NamedPipeClient*` fns. `Create*` → `INVALID_HANDLE_VALUE` (print); send/read → 0/FALSE; close → no-op (quiet, unreachable once Create fails). The two internal `*Example()` demos are not in the Linux TU. | Back with a filesystem FIFO (`mkfifo`) or, better for framed bidirectional messages, an `AF_UNIX` socket derived from the `\\.\pipe\NAME` string; overlapped/event I/O collapses to blocking `read`/`write`. |

All four self-guard with `#ifdef __linux__`, and every function still stubbed in
them prints `"... is not supported on Linux yet"` at runtime (named-pipe: only in
the `Create*` entry points, to avoid per-loop spam).

---

## Changes so far (files that build on Linux)

Swept from the port markers (`Platform*` calls, `_WIN32` / `__linux__` guards).
"Wrapper sweep" = mechanical rename of a raw Win32 call to its `Platform*`
equivalent, behavior-preserving.

### Build / precompiled header
- `libhyperdbg/pch.h` — the big one: `#ifdef _WIN32` guards around Windows-only
  headers (`dbghelp.h`, SCM, etc.); unconditional includes of the new
  platform headers + `nt-list.h`; include-order fixes. `install.h` is now included
  unconditionally (it was Windows-only) since its Linux-unsafe `SC_HANDLE` decls are
  self-guarded — see below.
- `header/debugger/driver-loader/install.h` — the 4 `SC_HANDLE` driver helpers
  (`InstallDriver`/`RemoveDriver`/`StartDriver`/`StopDriver`) guarded `#ifdef _WIN32`
  (they use the Windows-only `SC_HANDLE` type and have no callers outside install.cpp);
  `ManageDriver`, `SetupPathForFileName` and the `DRIVER_FUNC_*` macros stay visible on
  both so the Linux callers (`libhyperdbg.cpp`, hwdbg, export.cpp) compile.
- `CMakeLists.txt` (top-level) — `if(LINUX)` branch builds only `script-engine`,
  `libhyperdbg`, `hyperdbg-cli`; links `Threads::Threads`. (CMake is **Linux-only**;
  Windows builds from the `.vcxproj` / MSBuild.)
- `libhyperdbg/CMakeLists.txt` — `if(UNIX)` swaps `symbol.cpp`→`symbol-linux.cpp`,
  `pe-parser.cpp`→`pe-parser-linux.cpp`, and `install.cpp`→`install-linux.cpp`; header
  entries point at the real nested paths.

### script-engine subproject
- GCC-compatibility fixes across `script-engine/` (`pch.h`, `type.h`, `scanner.h`,
  `globals.{h,c}`, `script-engine.c`, its `CMakeLists.txt`) so the shared
  script-engine builds as a Linux `.so`.
- SDK import/interface headers (`include/SDK/HyperDbgSdk.h`,
  `include/SDK/imports/user/HyperDbg*Imports.h`) adjusted for the Linux build.
- Upstream `d44c726d` ("add float type in script engine") re-broke the Linux
  build; fixed with two mechanical swaps:
  - `pch.h` — define `_GNU_SOURCE` on Linux ahead of every libc header. The
    float-literal parser's `#else` branch calls `strtof_l`/`strtod_l`, which
    glibc declares in `<stdlib.h>` only under `__USE_GNU`. `newlocale`/
    `freelocale` in the same branch are plain POSIX-2008 and already resolved.
  - `script-engine.c:1381,1400` — 2× `_snprintf_s(Buf, sizeof(Buf), _TRUNCATE,
    "%d", …)` → `PlatformSprintf(Buf, sizeof(Buf), "%d", …)`. Both buffers are
    `CHAR[32]` formatting a single `%d`, so the dropped `_TRUNCATE` truncation
    semantics are unreachable.
- `code/hardware.c` (+ `header/hardware.h`) — added to script-engine CMake
  `SourceFiles` (was in the vcxproj, missing from the Linux build). Provides the
  `HardwareScriptInterpreter*` family the hwdbg TUs call. One bucket-1 swap to
  compile: `RtlZeroMemory`→`PlatformZeroMemory` ×2 (lines 499, 564). Resolves the
  `HardwareScriptInterpreter*` link errors. NOTE: two more files are still in the
  vcxproj but missing from script-engine's CMake — `code/script_include.c`
  (undefined `ResolveIncludePath`/`ParseIncludeFile`/`FileExists`/`InsertStrNew`)
  and `include/platform/user/code/platform-lib-calls.c` (undefined `Platform*` in
  libscript-engine.so). Adding both is the next script-engine build step.
- `code/script_include.c` + `platform-lib-calls.c` — DONE (2026-07-24).
  `libscript-engine.so` is now fully self-contained (zero undefined refs):
  - Added `../include/platform/user/code/platform-lib-calls.c` to the base
    `SourceFiles` — it's compiled into libhyperdbg too, but each `.so` needs its
    own copy of the `Platform*` symbols (script-engine calls `PlatformSnprintf`/
    `PlatformStrDup`/`PlatformVsnprintf`/`PlatformZeroMemory`). No swap: it builds
    on both OSes. Needed one root-cause fix — added `#include <time.h>` to its
    Linux include block (`clock_gettime`/`CLOCK_MONOTONIC` in
    `PlatformQueryPerformanceCounter`); it previously only compiled because
    libhyperdbg's pch pulled `<time.h>` in transitively, but script-engine's pch
    doesn't.
  - Added `code/script_include.c` to base `SourceFiles`, then swapped it for a new
    empty stub `code/script_include-linux.c` under `if(UNIX)` (user chose stubs
    over porting the Win32 path logic for now). The stub implements
    `ResolveIncludePath`/`FileExists`/`ParseIncludeFile`/`InsertStrNew` as
    no-op/failure; script `#include` resolution is unsupported on Linux until a
    real resolver (`readlink("/proc/self/exe")` + `stat`) lands. `script_include.c`
    left pristine (Windows path uses `GetModuleFileNameA`/`GetFileAttributesA`).

  Result: every remaining CLI-link undefined ref (23) now belongs to
  `liblibhyperdbg.so` alone — keystone (`ks_*`, 5), the excluded `pt.cpp`
  (`CommandPt*`/`HyperDbgPt*`, 4), and missing libhyperdbg TUs behind the
  PCI-ID/Vendor, Stepping, text-callback, `ShowMessages`, `IrpBasedBufferThread`
  symbols (14).

### Kernel-level debugger (remote protocol)
- `kd.cpp` — largest sweep (~46 `Platform*`): serial open/configure/close via
  `PlatformSerial*`; events/threads/handles via `Platform*`; `RtlZeroMemory`,
  `DeviceIoControl`, `GetLastError`, `GetCurrentProcessId` sweeps; the raw Win32
  serial data-path functions kept under `#ifdef _WIN32` with a Linux interface `#else`.
- `kernel-listening.cpp` — `RtlZeroMemory`, `strnlen_s`→`PlatformStrnlen`, serial
  wait/read via `PlatformSerial*`, `DebugBreak`→`PlatformDebugBreak`.
- `readmem.cpp` — `ZeroMemory` / `DeviceIoControl` / `GetLastError` sweep.

### common
- `common.cpp` — multi-category port:
  - `_stricmp`→`PlatformStrCaseCmp` (new wrapper, see platform-lib-calls).
  - `CpuIdEx`→`CpuCpuIdEx`. **This also fixes an upstream typo, not just a Linux
    shim.** Commit `85843494` ("add CPU intrinsics for user mode") swept the MSVC
    intrinsics onto the new `Cpu*` wrappers but wrote `CpuIdEx` where the wrapper is
    actually named `CpuCpuIdEx` (cf. `__cpuid`→`CpuCpuId` done correctly alongside).
    `CpuIdEx` is defined **nowhere** in the tree, so this line does not compile on
    Windows either — it was just never rebuilt there. Same bug also hit
    `cpu.cpp:148` / `cpu.cpp:200` — **now fixed there too** (user-confirmed the swap);
    still present on `master`.
  - `IsFileExistA` — kept as-is (POSIX `struct stat`/`stat()`); added `#include
    <sys/stat.h>` under `__linux__`.
  - Whole Windows-only bodies guarded `#ifdef _WIN32` with a Linux stub + TODO:
    `SetPrivilege` (token/LUID; no Linux callers → returns FALSE),
    `IsFileExistW` (`_wstat`; wide-char deferred → FALSE),
    `GetConfigFilePath` (`GetModuleFileNameW`/shlwapi; wide-char deferred → empties path).
  - `ListDirectory` — **implemented** (2026-08-02), see the file-location section below.
  - `CheckAddressValidityUsingTsx` — TSX `_xbegin`/`_xend`/`_XBEGIN_STARTED` kept
    verbatim; they resolve on GCC via `<immintrin.h>` (included under `__linux__`)
    once `-mrtm` is set. Added `target_compile_options(libhyperdbg PRIVATE -mrtm)`
    in `libhyperdbg/CMakeLists.txt` (UNIX block). Real 1:1 mapping, not a stub — the
    path is gated by `g_RtmSupport`, which is live on Linux now that cpuid works.
- `platform-lib-calls.{h,c}` — added `PlatformStrCaseCmp(Str1, Str2)`: Windows
  `_stricmp`; Linux `strcasecmp` (added `<strings.h>` to the Linux includes).

### Core debugger
- `debugger.cpp` — `DeviceIoControl` / `GetLastError` / `RtlZeroMemory` sweep;
  `DebuggerGetNtoskrnlBase` body guarded `#ifdef _WIN32` (Linux returns NULL).
- `interpreter.cpp` — `SetConsoleCtrlHandler`→`PlatformInstallCtrlHandler`;
  script-engine message-callback cast.
- `break-control.cpp` — console-control handler routed through `platform-signal`.

### App / export layer
- `export.cpp` — `strcpy_s`×2 → `PlatformStrCpy` (new bounded-copy wrapper; also
  unblocked once `SetupPathForFileName` became visible via the install-linux swap).
- `platform-lib-calls.{h,c}` — added `PlatformStrCpy(Dest, DestSize, Src)`: Windows
  `strcpy_s`; Linux does the same bounds check (empty-string + non-zero on overflow)
  since glibc has no `strcpy_s`. ⚠️ Linux branch **not yet tested** against the exact
  `strcpy_s` semantics — verify before relying on it.
- `platform-lib-calls.{h,c}` — added `PlatformCopyMemory(Destination, Source, Size)`:
  Windows `RtlCopyMemory`; Linux `memcpy` (same arg order/signature).

### hwdbg
- `hwdbg-interpreter.cpp` — `RtlCopyMemory`→`PlatformCopyMemory`, `RtlZeroMemory`→`PlatformZeroMemory`.
- `hwdbg-scripts.cpp` + `hwdbg-commands/hw.cpp` — added to libhyperdbg CMake
  `SourceFiles` (were missing from the Linux build; present in the vcxproj all
  along), plus the `header/hwdbg/hwdbg-scripts.h` list entry. `hw.cpp` built
  clean; `hwdbg-scripts.cpp` needed one bucket-1 swap: `RtlZeroMemory`→
  `PlatformZeroMemory` (line 415). Both compile on Linux now. NOTE: their
  `HardwareScriptInterpreter*` callees live in `script-engine/code/hardware.c`,
  now added to script-engine's CMake (see the script-engine subproject section).

### objects
- `objects.cpp` — wrapper sweep: `RtlCopyMemory`×2→`PlatformCopyMemory`,
  `RtlZeroMemory`→`PlatformZeroMemory`, `DeviceIoControl`×4→`PlatformDeviceIoControl`,
  `GetLastError`×4 (the `"ioctl failed"` sites)→`PlatformGetLastError`; plus the two
  enum-first-member `= {0}`→`= {}` value-init fixes (lines 30/31; line 145's struct
  isn't enum-first, left as `= {0}`).

### rev
- `rev-ctrl.cpp` — `DeviceIoControl`→`PlatformDeviceIoControl`, `GetLastError`→`PlatformGetLastError`.

### App
- `dllmain.cpp` — whole `DllMain` body guarded `#ifdef _WIN32` (Windows DLL loader
  entry point; no Linux equivalent, no callers in our code, body was a no-op). Linux
  TU is intentionally empty.
- `libhyperdbg.cpp` — the main app (load/unload driver, open device, event loop).
  Wrapper sweeps: `DeviceIoControl`×5→`PlatformDeviceIoControl`, `GetLastError`×7→`PlatformGetLastError`,
  `CloseHandle`×4→`PlatformCloseHandle`, `WaitForSingleObject`→`PlatformWaitForSingleObject`,
  `CreateEvent(NULL,FALSE,FALSE,NULL)`→`PlatformCreateEvent(FALSE,FALSE)`,
  `CreateThread(...)`→`PlatformCreateThread(fn,NULL)`, and the 2-arg `strcpy_s(g_DriverName, ...)`
  template form→`PlatformStrCpy(g_DriverName, sizeof(g_DriverName), ...)`. The local-driver
  device open (`CreateFileA("\\.\HyperDbgDebuggerDevice", ...)`) → new `PlatformOpenDevice`
  wrapper (see platform-ioctl); the surrounding error-handling block stays at the call site
  (`ERROR_ACCESS_DENIED`/`ERROR_GEN_FAILURE` added to `Environment.h` so it compiles on Linux).
  `WindowsSetDebugPrivilege` now resolves via `windows-privilege.c` (see below).
- `windows-only/windows-privilege.{c,h}` — `WindowsSetDebugPrivilege` was already ported
  (Windows: token/SeDebugPrivilege; Linux branch: `return TRUE`), just not wired into the
  Linux build. Added `windows-privilege.c` to `libhyperdbg/CMakeLists.txt` (both the source
  list and the `LANGUAGE CXX` block); un-guarded its header include in `pch.h` (was
  `#ifdef _WIN32`, header is Linux-safe); fixed the header's `#ifdef __linux__` SDK include
  path (`../../../../` → `../../../../../`, it sits one dir deeper in `windows-only/`).

### User-level debugger
- `ud.cpp` — wrapper sweep (bucket 1): `DeviceIoControl`→`PlatformDeviceIoControl`,
  the `"ioctl failed"` `GetLastError`→`PlatformGetLastError`, `RtlZeroMemory`→`PlatformZeroMemory`,
  the event-handle `CloseHandle`→`PlatformCloseHandle`, `CreateEvent(NULL,x,y,NULL)`→`PlatformCreateEvent(x,y)`.
  Win32 process/thread-management (bucket 2): 5 new `Platform*` process wrappers (Group A)
  + whole-body `#ifdef _WIN32` guards on the Toolhelp walkers / `UdPrintError` (Group B).
  See the Process-control section of the TODO ledger for details.
- `platform-lib-calls.{h,c}` — added `PlatformCreateProcess`/`PlatformOpenProcess`/
  `PlatformTerminateProcess`/`PlatformResumeThread`/`PlatformGetExitCodeProcess`
  (Windows real, Linux stub).

### Script engine
- `script-engine-wrapper.cpp` — 6× `RtlZeroMemory`→`PlatformZeroMemory`; the two
  wide-char test harnesses (`AllocateStructForCasting`, `ScriptEngineWrapperTestParser`)
  guarded out on Linux (see wide-char TODO).
- `script-eval/Functions.c` — `sprintf_s`, `__rdtsc(p)`, `Interlocked*`,
  `RtlZeroMemory`, `QueryPerformance*` → `Platform*`/`Cpu*` wrappers.
- `script-eval/PseudoRegisters.c` — `$peb` guarded `#ifdef _WIN32` (returns 0);
  `$tid`/`$pid`/`$core`/`$pname` → new `Platform*` wrappers.

### Commands & app (wrapper sweeps + wide-char casts)
- Meta: `dump.cpp`, `pagein.cpp`, `pe.cpp`, `start.cpp`, `restart.cpp`
  (the last two carry `(WCHAR *)` wide-char shim casts).
- Debugging/extension: `a.cpp`, `dt-struct.cpp`, `k.cpp`, `preactivate.cpp`,
  `prealloc.cpp`, `sleep.cpp`, `track.cpp`, `pci-id.cpp`, `pcicam.cpp`, `pcitree.cpp`.
- App: `messaging.cpp`, `packets.cpp` (`vsprintf_s`→`PlatformVsnprintf`),
  `spinlock.cpp` (`_interlockedbittestandset`→`CpuInterlockedBitTestAndSet`).
  (`libhyperdbg.cpp` itself has its own entry under **App** above.)

### Commands batch sweep — DONE (2026-07-20)

Mechanical bucket-1 sweep across 23 files in `libhyperdbg/code/debugger/commands/`.
Behaviour-preserving 1:1 substitutions only, no logic touched:

| Swap | Count |
|------|-------|
| `DeviceIoControl` → `PlatformDeviceIoControl` | 19 |
| `GetLastError` → `PlatformGetLastError` | 20 |
| `RtlZeroMemory` / `ZeroMemory` → `PlatformZeroMemory` | 15 |
| `GetCurrentProcessId` → `PlatformGetCurrentProcessId` | 7 |
| `CloseHandle` → `PlatformCloseHandle` | 2 |
| `TerminateThread` → `PlatformTerminateThread` | 1 |

- Debugging: `flush.cpp`, `lm.cpp`, `load.cpp`, `output.cpp`, `rdmsr.cpp`, `s.cpp`,
  `test.cpp`, `wrmsr.cpp`
- Extension: `apic.cpp`, `hide.cpp`, `idt.cpp`, `ioapic.cpp`, `lbr.cpp`,
  `lbrdump.cpp`, `pa2va.cpp`, `pcicam.cpp`, `pcitree.cpp`, `pte.cpp`, `smi.cpp`,
  `unhide.cpp`, `va2pa.cpp`
- Meta: `sym.cpp`, `disconnect.cpp`

**Pure addition:** `PlatformTerminateThread` in `platform-lib-calls.{h,c}` — real
`TerminateThread` on Windows, Linux stub returning TRUE (see TODO below).

After this sweep `pt.cpp` is the only file left in `commands/` holding raw Win32
calls. Note `lm.cpp` still does not compile, but for unrelated pre-existing
reasons (`RTL_PROCESS_MODULES` / `RTL_PROCESS_MODULE_INFORMATION` undeclared, and
`WCHAR *` vs `wchar_t *` — the wide-char item below); none of those are on lines
this sweep touched.

### Command files wired into Linux CMake — DONE (2026-07-24)

The 2026-07-20 sweep ported these files' bucket-1 calls but never added them to
`libhyperdbg/CMakeLists.txt`, so they were compiled on Windows only and their
`Command*` symbols were unresolved at the Linux CLI link. Added the 12 missing
command TUs to `SourceFiles`:
- Debugging: `continue.cpp`, `gg.cpp`
- Extension: `apic.cpp`, `idt.cpp`, `ioapic.cpp`, `lbr.cpp`, `lbrdump.cpp`,
  `pcicam.cpp`, `pcitree.cpp`, `smi.cpp`, `xsetbv.cpp`
- Plus top-level `ucpuid.cpp` (defines `CommandUserCpuid` / `CommandUserCpuidHelp`
  / `CommandCpuidRequestCpuid` / `CommandShowUserCpuidMessage`; lives at
  `libhyperdbg/ucpuid.cpp`, not under `code/`, which is why the earlier diff
  missed it).

Stragglers the 07-20 sweep didn't cover, fixed to compile (all mechanical):
- `apic.cpp` — 2× `RtlCopyMemory`→`PlatformCopyMemory` (sweep only did the
  ZeroMemory family).
- Enum-first aggregate init `= {0}`→`= {}` (GCC rejects `int`→enum in `{0}`;
  `{}` value-inits identically): `lbr.cpp:332`, `lbrdump.cpp:242`,
  `pcicam.cpp:51`, `pcitree.cpp:49`, `smi.cpp:125`. (apic's `LAPIC_PAGE {0}` and
  lbrdump's `CHAR[] {0}` are scalar-first and compile fine, left as-is.)
- `ucpuid.cpp` — `DeviceIoControl`→`PlatformDeviceIoControl`,
  `GetLastError`→`PlatformGetLastError` (1 each; same drop-in as the sweep).
- `Environment.h` — added the two missing generic Win32 aliases `ucpuid.cpp`
  needs: `#define CONST const` and `typedef float FLOAT;` (winnt.h spellings;
  benefits any future file too).

**`pt.cpp` deliberately excluded from the Linux build** via an `if(UNIX)`
`REMOVE_ITEM` (like namedpipe/symbol/pe-parser). It's the un-started
process-control port (`OpenProcess(PROCESS_ALL_ACCESS)`,
`CreateToolhelp32Snapshot`, `CreateThread`, `WaitForMultipleObjects`, Win32
process/thread handles) — see the `pt.cpp` TODO below. `CommandPt`/`CommandPtHelp`
stay unresolved, same as before it was added to the list.

Result: every `Command*` link error is resolved except the two `CommandPt*`.
Remaining CLI-link buckets are unrelated: `Sym*` (symbol-linux stub, 15), `ks_*`
(keystone Linux lib, 5), `Platform*` + include-family (script_include.c /
platform-lib-calls.c missing from script-engine's CMake, 8).

### rdmsr.cpp core-count — DONE (2026-07-22)

Follow-up to the bucket-1 sweep of `rdmsr.cpp` above (this is a separate bucket-2
change, not part of the mechanical batch). The command needs the online logical-CPU
count to size its per-core transfer buffer; on Windows that came from two static
helpers (`GetWindowsCompatibleNumberOfCores` via `GetSystemInfo`, and
`GetWindowsNumaNumberOfCores` via `GetLogicalProcessorInformationEx` — the latter
`GetProcAddress`-loaded from `kernel32.dll`, so entirely Win32).

- Both static helpers + their `glpie_t` typedef guarded `#ifdef _WIN32` (rdmsr.cpp
  lines 36–111). Windows bodies untouched.
- Call site (`CommandRdmsr`, ~line 199): Windows path keeps the NUMA-then-fallback
  logic; Linux `#else` calls the new `PlatformGetActiveProcessorCount()`.
- **Pure addition:** `PlatformGetActiveProcessorCount(VOID)` in
  `platform-lib-calls.{h,c}` — Windows `GetSystemInfo`→`dwNumberOfProcessors`;
  Linux `sysconf(_SC_NPROCESSORS_ONLN)` (returns 0 if unknown). ⚠️ Linux branch
  marked "Not yet tested!!" in the source — verify before relying on it.

### settings.cpp config-file I/O — DONE (2026-07-22)

`debugging-commands/settings.cpp` reads/writes the settings INI via a wide-char
(`WCHAR[MAX_PATH]`) path and `std::ifstream`/`std::ofstream`. Two Linux-only
blockers, both the deferred wide-char item: (1) `GetConfigFilePath(PWCHAR)` — on
Linux `WCHAR` is `unsigned short` but `PWCHAR` is `short *` (signedness mismatch,
`-fpermissive`); (2) libstdc++ has **no** `basic_ifstream`/`basic_ofstream`
constructor taking a 2-byte `WCHAR*`, and no cast can bridge wide→`char*`.

Both `CommandSettingsGetValueFromConfigFile` and `CommandSettingsSetValueFromConfigFile`
are already dead on Linux (`GetConfigFilePath` empties the path,
`IsFileExistW` returns FALSE), so their whole bodies were guarded `#ifdef _WIN32`
(Windows verbatim) with a Linux `#else` stub (`return FALSE` / no-op +
`UNREFERENCED_PARAMETER` + TODO(Linux)). Same pattern-1 convention already used
for `GetConfigFilePath`/`IsFileExistW`/`ListDirectory` in common.cpp. This also
moves the `GetConfigFilePath` call sites into the Windows branch, resolving the
`PWCHAR` signedness error without touching the shared typedef.
`CommandSettingsLoadDefaultValuesFromConfigFile` only calls the guarded getter —
no wide-char of its own, left as-is.

### debug.cpp serial connect — DONE (2026-07-22)

`meta-commands/debug.cpp` (the `.debug` command — connect to a remote debuggee
over serial/namedpipe). Two mechanical fixes:
- `_stricmp`×4 (COM-port name compare in `CommandDebugCheckComPort`) →
  `PlatformStrCaseCmp` (the existing common.cpp wrapper; Windows `_stricmp`,
  Linux `strcasecmp`).
- `CBR_*` baud-rate constants (`CommandDebugCheckBaudrate` validation) — **pure
  addition** to `Environment.h` Linux block: the 15 winbase.h `CBR_110`…`CBR_256000`
  `#define`s kept at their canonical Windows values (each equals its baud rate).
  Matches the CTRL_*/PROCESS_*/ERROR_* constant blocks already there. Actual Linux
  serial I/O is still the platform-serial termios TODO.

### formats.cpp DECIMAL_DIG — DONE (2026-07-22)

`meta-commands/formats.cpp:94` uses `DECIMAL_DIG` (the ISO C99 `<float.h>` macro,
widest-float round-trip digit count) in a `.formats` output format string. MSVC
exposes it transitively via its CRT/pch; glibc needs the explicit include.
**Pure addition:** `#include <float.h>` in the `Environment.h` Linux block
(next to `<wchar.h>`/`<unistd.h>`). Standard header, cross-platform-safe.

### forwarding.cpp output-event forwarding — DONE (2026-07-22)

`communication/forwarding.cpp` is the debug-output forwarding subsystem (sinks:
file / TCP / named-pipe / loadable module). Bucket-2, multi-category. User chose
**new Platform\* wrappers** for both non-trivial subsystems (not guards).

- **Clean swap:** `WriteFile`→`PlatformWriteFile` (exact match; the original
  assigns the result then unconditionally `return TRUE`, so the error-check below
  was already dead code — `BytesWritten` out-param dropped, still referenced by
  that dead code so no unused-var). `CloseHandle` (FILE source)→`PlatformCloseFile`
  (fclose on Linux — matches the FILE\* the new open returns).
- **File sink** (`CreateFileA`, narrow path + `OPEN_ALWAYS`): existing
  `PlatformOpenFileForWriting` did NOT fit (it is wide + `CREATE_ALWAYS`/truncate),
  so **pure addition** `PlatformOpenFileForWritingNarrow(const CHAR *)` — Windows
  `CreateFileA(...OPEN_ALWAYS...)`; Linux `fopen("r+b")` then `fopen("w+b")`
  (open-existing-no-truncate, else create) returning the `FILE*` as the HANDLE.
  Named `...Narrow` (user preference) to flag the char-width difference vs the
  wide variant. Because the path is already a narrow `std::string`, this sink
  actually works on Linux — no wide-char blocker.
- **Module/plugin sink** (`LoadLibraryA`/`GetProcAddress`/`FreeLibrary`): **pure
  additions** `PlatformLoadLibrary`/`PlatformGetProcAddress`/`PlatformFreeLibrary`
  in platform-lib-calls — Windows real; Linux `dlopen(RTLD_NOW|RTLD_LOCAL)`/
  `dlsym`/`dlclose` (dlclose return inverted to keep "non-zero == success").
  `PlatformGetProcAddress` returns `PVOID` (no `FARPROC` on Linux); caller casts.
- **Build:** added `#include <dlfcn.h>` to the platform-lib-calls Linux includes;
  added `${CMAKE_DL_LIBS}` to the `libhyperdbg` link (top-level CMakeLists) — the
  portable dl link (empty where dl is in libc). Only libhyperdbg compiles
  platform-lib-calls.c on Linux, so no other target needed it.
- ⚠️ All four new Linux branches marked `NOT YET TESTED!!` in source.

### namedpipe.cpp — DONE via namedpipe-linux.cpp + CMake swap (2026-07-22)

`communication/namedpipe.cpp` is a whole Windows-only TU (Win32 named-pipe IPC:
server `CreateNamedPipe`/`ConnectNamedPipe`, client `CreateFileA` on `\\.\pipe\`
+ overlapped `ReadFile`/`WriteFile` via `g_OverlappedIoStructureFor*Debugger`).
Followed pattern-2 (like symbol/pe-parser/install): new `namedpipe-linux.cpp`
`#ifdef __linux__` stubs of the 10 public `NamedPipe{Server,Client}*` fns;
`namedpipe.cpp` left 100% untouched; CMake `if(UNIX)` REMOVE_ITEM + APPEND swap.
6 callers link the stubs transparently (forwarding/kd/debug/export/tests/test).
See the Linux-replacement-files table above for the FIFO/AF_UNIX TODO.

### TCP transport (tcpclient/tcpserver/remote-connection) — DONE (2026-07-22)

The TCP remote-debugging path. Unlike namedpipe, the socket code is genuinely
cross-platform — Winsock and POSIX share the BSD-socket API almost verbatim — so
it stays as shared `.cpp` (no `-linux.cpp` fork). A new **`platform-socket.{h,c}`
module** (sibling to platform-serial/-signal; wired into pch.h, top-level +
libhyperdbg CMake, and the Windows `.vcxproj`/`.filters`) isolates the handful of
things that actually diverge. User chose the platform-API route over Winsock-name
shims in `Environment.h`.

- **`communication/tcpclient.cpp` / `tcpserver.cpp`** — the portable calls
  (`socket`/`connect`/`bind`/`listen`/`accept`/`send`/`recv`/`shutdown`/
  `getaddrinfo`) stay at the call sites unchanged. Swapped only the divergent
  ones to `Platform*`: `WSAStartup(MAKEWORD(2,2),&wsaData)`→`PlatformSocketInitialize()`
  (WSADATA local + MAKEWORD dropped; keeps the `IResult != 0` shape — wrapper
  returns 0 on success), `WSACleanup()`→`PlatformSocketCleanup()`,
  `closesocket`→`PlatformCloseSocket`, `shutdown(...,SD_SEND)`→`PlatformShutdownSocketSend`,
  `WSAGetLastError()`→`PlatformGetSocketError()`, plus the bucket-1
  `ZeroMemory`→`PlatformZeroMemory`. tcpserver's `accept()` length out-param
  `INT AddrLen`→`PLATFORM_SOCKLEN AddrLen` — the one genuine type incompatibility
  (Winsock `int*` vs POSIX `socklen_t*`).
- **`communication/remote-connection.cpp`** — bucket-1 sweep (`.listen`/`.connect`
  command layer over the sockets): `RtlZeroMemory`×3→`PlatformZeroMemory`,
  `SetEvent`→`PlatformSetEvent`, `CreateEvent(NULL,FALSE,FALSE,NULL)`→`PlatformCreateEvent(FALSE,FALSE)`,
  `CreateThread(...)`→`PlatformCreateThread(fn,NULL)` (unused `DWORD ThreadId` local
  dropped), `WaitForSingleObject`→`PlatformWaitForSingleObject`.
- **`platform-socket.{h,c}`** (pure addition): `PlatformSocketInitialize`/
  `PlatformSocketCleanup`/`PlatformCloseSocket`/`PlatformShutdownSocketSend`/
  `PlatformGetSocketError` + the `PLATFORM_SOCKLEN` typedef. The `.c` maps the
  divergent primitives (close / shutdown-flag / last-error) via small per-OS
  macros so each wrapper body is written once; only the WSAStartup vs no-op
  lifecycle keeps a small in-body guard. The `.h` also owns the Linux POSIX
  socket-header includes (`<sys/socket.h>`/`<netdb.h>`/`<netinet/in.h>`/
  `<arpa/inet.h>`/`<unistd.h>`), so any TU using sockets gets them via pch.
  ⚠️ Linux branches not yet exercised at runtime (compile-verified only).

Note the pre-existing latent teardown-ordering issue is unchanged; see the
`PlatformTerminateThread` TODO below (remote-connection's listening thread).

### asm-vmx-checks — DONE via GAS port + CMake swap (2026-07-24)

`code/assembly/asm-vmx-checks-masm-windows.asm` (MASM, `AsmVmxSupportDetection`:
CPUID.1 → `bt ecx,5` → return 1/0 for VMX support) only assembles with ml64.
Ported to a new GAS/AT&T-syntax `code/assembly/asm-vmx-checks-gas-unix.s` —
instruction-for-instruction equivalent, `.globl AsmVmxSupportDetection`, plus a
`.note.GNU-stack` non-exec-stack marker. No logic change. The Windows `.asm` is
left untouched. CMake: base `SourceFiles` entry renamed to `-masm-windows.asm`,
and the `if(UNIX)` block REMOVE_ITEMs it, APPENDs the `.s`, and calls
`enable_language(ASM)` so CMake assembles it with the system assembler. Windows
`libhyperdbg.vcxproj` + `.filters` `<MASM Include=...>` updated to the renamed
`-masm-windows.asm`. Assemble-verified with `as` (exports `AsmVmxSupportDetection`).

### Remaining libhyperdbg TUs wired into Linux CMake — DONE (2026-07-24)

Same gap class as the 2026-07-24 command-file batch: four TUs present in
`libhyperdbg.vcxproj` all along but never added to `libhyperdbg/CMakeLists.txt`,
so they were compiled on Windows only and their symbols were unresolved at the
Linux CLI link. Added to `SourceFiles` (plus their four header entries):

| TU | Symbols it was missing |
|----|------------------------|
| `code/app/messaging.cpp` | `ShowMessages`, `SetTextMessageCallback`, `SetTextMessageCallbackUsingSharedBuffer`, `UnsetTextMessageCallback` |
| `code/app/packets.cpp` | `IrpBasedBufferThread` |
| `code/debugger/core/steppings.cpp` | `SteppingStepOver`, `SteppingStepOverForGu`, `SteppingRegularStepIn`, `SteppingInstrumentationStepIn`, `SteppingInstrumentationStepInForTracking` |
| `code/debugger/misc/pci-id.cpp` | `GetVendorById`, `GetDeviceFromVendor`, `FreeVendor`, `FreePciIdDatabase` |

`steppings.cpp` compiled with no changes at all. The others needed:

- `messaging.cpp` — 1 bucket-1 swap: `RtlZeroMemory`→`PlatformZeroMemory` (line 57).
- `packets.cpp` — bucket-1 sweep: `ZeroMemory`→`PlatformZeroMemory`,
  `DeviceIoControl`→`PlatformDeviceIoControl`, `SetEvent`→`PlatformSetEvent`,
  `CloseHandle`→`PlatformCloseHandle`, `GetLastError`×2→`PlatformGetLastError`.
  Plus the packet-reader's dedicated device handle: the same
  `CreateFileA("\\.\HyperDbgDebuggerDevice", GENERIC_READ|GENERIC_WRITE, …)`
  block already ported in `libhyperdbg.cpp` → `PlatformOpenDevice(...)`, with the
  surrounding `ERROR_ACCESS_DENIED`/`ERROR_GEN_FAILURE` handling left at the call
  site (identical shape to the libhyperdbg.cpp call site).
- `pci-id.cpp` — 4× `strncpy_s`→ new `PlatformStrNCpy` (below), and
  `GetVendorById` body guarded `#ifdef _WIN32` (below).

**Pure addition: `PlatformStrNCpy(Dest, DestSize, Src, Count)`** in
`platform-lib-calls.{h,c}` — Windows `strncpy_s` verbatim; Linux reproduces the
documented rules: copies D = min(Count, strlen(Src)) chars and null-terminates,
or empties Dest + returns non-zero if D doesn't fit; `Count == _TRUNCATE` instead
copies as much as fits and returns `STRUNCATE`. Sibling of the existing
`PlatformStrCpy`; a plain `PlatformStrCpy` could not be reused because
`ReadLine` (pci-id.cpp:76) copies a *substring* out of a longer stream buffer.
Also **pure addition** to the `Environment.h` Linux block: `_TRUNCATE`
(`((SIZE_T)-1)`) and `STRUNCATE` (`80`) at their canonical MSVC values, matching
the existing `CBR_*`/`ERROR_*`/`PROCESS_*` constant blocks.
⚠️ Linux branch marked `NOT YET TESTED!!` in source, like `PlatformStrCpy`.

**`GetVendorById` body guarded `#ifdef _WIN32`** (pattern 1; user chose the stub
over porting). It resolves the PCI ID database *relative to the executable*:
`GetModuleHandle`/`GetModuleFileName`, then `strrchr(Path, '\\')` to strip the
exe name and append `PCI_ID_DATABASE_PATH`. The two Win32 calls would wrap
cleanly (`readlink("/proc/self/exe")`), but the surrounding logic is
Windows-path-shaped in two places — the `'\\'` separator and the constant itself
(`pci-id.h:44`, `"constants\\pci.ids"`) — so wrapping only the calls would leave
`strrchr` returning NULL, silently overwriting the whole path and resolving
against the cwd. That is a behaviour change, not a port, so the whole body is
Windows-only and Linux returns NULL.
**Resolved (2026-08-02)** — the Linux `#else` now calls `SetupPathForFileName`,
which does the `readlink` walk *and* the separator normalization in one place, so
neither of the two path-shape problems is left in `pci-id.cpp`. See the
file-location section below. `GetVendorByIdStr`, `GetDeviceFromVendor`,
`FreeVendor` and `FreePciIdDatabase` are plain C and compile unchanged.
Consequence: `!pcitree` / `!pcicam` now show vendor and device names on Linux
too, as long as `constants/pci.ids` is deployed next to the binary.

**Result: the CLI link is down from 23 undefined refs to 9**, and every
"missing TU" bucket is now closed. What is left is both known and deliberate:
`ks_*` (5 — keystone, no Linux lib linked) and `CommandPt*`/`HyperDbgPt*`
(4 — `pt.cpp` excluded from the Linux build, port not started).

### keystone assembler stubbed on Linux — DONE (2026-07-24)

The 5 `ks_*` link errors (`ks_open`/`ks_option`/`ks_asm`/`ks_errno`/`ks_close`).
Only a **Windows** `keystone.lib` is vendored (`libraries/keystone/release-lib/`,
PE/COFF) and `dependencies/keystone/` ships **headers only** — no Linux library,
no source, not a git submodule. The `link_directories(...keystone...)` and the
`keystone` entry in `target_link_libraries` were previously commented out in the
top-level `CMakeLists.txt` to get past `cannot find -lkeystone`, which is what
left the 5 symbols unresolved.

Because `dependencies/keystone/include/keystone/keystone.h` *is* present (and
included unconditionally from `pch.h:138`), every `ks_*` **type and constant**
(`ks_engine`, `ks_err`, `ks_arch`, `KS_ARCH_X86`, `KS_MODE_64`,
`KS_OPT_SYNTAX_INTEL`, …) resolves fine on Linux — only the 5 *functions* are
missing. That means no header surgery and no `-linux.cpp` fork were needed:
`assembler.h`'s class declaration (which has `ks_err KsErr` as a member and
`ks_arch`/`KS_*` as default arguments) compiles untouched.

All 5 calls are confined to one method, so this is pattern 1 — the body of
`AssembleData::Assemble` (`assembler.cpp:119`) is guarded `#ifdef _WIN32`
(Windows verbatim) with a Linux `#else` that emits
`"err, the assembler is not supported on Linux yet"` and returns `-1`, plus
`UNREFERENCED_PARAMETER` ×4 and a TODO(Linux). No call-site changes were needed:
both callers (`HyperDbgAssembleGetLength`, `HyperDbgAssemble`) already treat a
non-zero `Assemble()` return as failure and return `FALSE`, so the stub flows
through the existing error paths. The rest of the TU stays live on Linux —
notably `ParseAssemblyData`, which does the `<symbol>` resolution.

Affects the `a` (assemble) command and anything calling `HyperDbgAssemble`.

- [ ] Real fix: build upstream Keystone for Linux → `libkeystone.a`/`.so`, then
  restore the two commented-out lines in the top-level `CMakeLists.txt` and drop
  the guard.

**Result: the CLI link is down to 4 undefined refs**, all `pt.cpp`
(`CommandPt`, `CommandPtHelp`, `HyperDbgPtMmapSendRequest`,
`HyperDbgPerformPtOperation`) — the one remaining deliberate exclusion.

### pt.cpp stubbed on Linux — DONE (2026-07-24) — **THE LINK NOW SUCCEEDS**

The last 4 undefined refs (`CommandPt`, `CommandPtHelp`,
`HyperDbgPerformPtOperation`, `HyperDbgPtMmapSendRequest`). `pt.cpp` was already
`REMOVE_ITEM`'d from the Linux build; it now gets a replacement stub instead of
leaving the symbols dangling, following the same pattern as symbol.cpp,
pe-parser.cpp, install.cpp and namedpipe.cpp.

New `code/debugger/commands/extension-commands/pt-linux.cpp` (`#ifdef __linux__`)
implements only the 4 externally visible functions — the two command entry
points reached from the dispatch table (`CommandPt`, `CommandPtHelp`) and the two
kernel-request helpers declared in `debugger.h`. Each prints a "not supported on
Linux yet" note; the `BOOLEAN` pair returns FALSE. Everything else in `pt.cpp` is
helper code reached only through those entry points, so it does not exist in the
Linux TU. `pt.cpp` is still left 100% untouched. CMake `if(UNIX)` now does the
usual REMOVE_ITEM + APPEND pair.

**Result: `hyperdbg-cli` links and runs on Linux for the first time.** With the
symbol-visibility fix below also in place, the binary starts, reaches the
`HyperDbg>` prompt and executes host-side commands correctly — verified with
`.help`, `.help !monitor`, `.formats 0x1337` (full hex/decimal/octal/binary/char/
time/float/double output) and `? 5 * 8` (script engine) — then exits cleanly on
`.exit`. The port is out of the compile/link phase and into the runtime phase.

Run it with:
```bash
LD_LIBRARY_PATH=$PWD/libhyperdbg:$PWD/script-engine ./hyperdbg-cli/hyperdbg-cli
```

- [ ] Port `pt.cpp` for real — see the process-control entry in the TODO ledger.

### Symbol visibility: honour the existing IMPORT_EXPORT_* model — DONE (2026-07-24)

Build-system change in the top-level `CMakeLists.txt`, two parts:

```cmake
target_compile_definitions(script-engine PRIVATE HYPERDBG_SCRIPT_ENGINE)
target_compile_definitions(libhyperdbg  PRIVATE HYPERDBG_LIBHYPERDBG)

set_target_properties(script-engine libhyperdbg PROPERTIES
    C_VISIBILITY_PRESET   hidden
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
```

**Why.** `include/SDK/imports/user/HyperDbg*Imports.h` already carries a Linux
branch for each library's export macro — `IMPORT_EXPORT_LIBHYPERDBG` and friends
expand to `__attribute__((visibility("default")))` when the library's own
`HYPERDBG_*` macro is defined, and to nothing otherwise, mirroring the
`__declspec(dllexport)`/`dllimport` pair used on Windows. 77 symbols are annotated
for libhyperdbg and 29 for script-engine. Neither half was active on Linux: the
`HYPERDBG_*` defines were never set by CMake, and `visibility("default")` is a
no-op unless the compiler's baseline visibility is `hidden` (it can only raise a
symbol above the baseline, and the baseline was already default). So the whole
export model existed in the headers but did nothing, and every symbol in both
libraries was exported.

That matters because ELF merges same-named exported symbols across shared objects,
whereas a Windows DLL's non-exported globals are private to it. Three globals were
defined independently in both libraries and were being silently collapsed into one
object at load time:

| Symbol | libhyperdbg | script-engine |
|--------|-------------|---------------|
| `g_MessageHandler` | `header/globals/globals.h:460` | `code/globals.c:24` |
| `g_HwdbgInstanceInfo` | ” | ” |
| `g_HwdbgInstanceInfoIsValid` | ” | ” |

Turning both halves on restores the Windows semantics (private unless explicitly
exported). Exported data symbols drop to **0** in both libraries; total exports go
from everything to 264 (libhyperdbg) and 25 (script-engine). The link stays clean
— **0 undefined references** — so nothing was relying on an unannotated symbol
crossing a library boundary. No source file was touched.

⚠️ `VISIBILITY_INLINES_HIDDEN` is included to match the usual CMake pairing; if a
future change takes the address of an inline member across a library boundary and
compares it, that flag is the first thing to re-check.

Grouped by subsystem. These are the shortcuts taken to reach compilation.

### Wide characters (the big deferred item)
- [ ] **`wchar_t` 2-vs-4-byte / `WCHAR` / `UNICODE_STRING`.** On Linux `WCHAR` is
  2 bytes but native `wchar_t` is 4. Current state: bogus 2-byte reinterpret casts
  at call sites (each marked `TEMPORARY LINUX SHIM / TODO(Linux)`), and the
  `script-engine-wrapper.cpp` test harnesses are guarded out on Linux. Needs a
  real `std::wstring` → 2-byte-`WCHAR`/UTF-16 conversion helper before Linux file
  I/O and the user-debugger path can actually open files.

### Symbols
- [x] symbol-parser (`Sym*`) Linux stubs — DONE (2026-07-24). The 15 `Sym*`
  exports (`SymConvertNameToAddress`, `SymLoadFileSymbol`, `SymbolInitLoad`,
  `SymGetFieldOffset`, `SymShowDataBasedOnSymbolTypes`, `SymSetTextMessageCallback`,
  …) live in the Windows-only `symbol-parser/` subproject (DbgHelp + DIA-SDK
  pdbex, ~3800 LOC, not built on Linux). They're called only by
  `script-engine/code/script-engine.c`, so `libscript-engine.so` was the one with
  the unresolved refs. Added `script-engine/code/symbol-stub-linux.c` (new file,
  `#ifdef __linux__`) implementing all 15 as no-op/failure stubs (return `0`/`FALSE`,
  out-params cleared), signatures mirroring `HyperDbgSymImports.h`. Wired into
  `script-engine/CMakeLists.txt` under `if(UNIX)`. User chose the stub path over a
  real backend port. Resolves all 15 `Sym*` link errors.
- [ ] Replace the `symbol-linux.cpp` (`Symbol*`) and `symbol-stub-linux.c`
  (`Sym*`) stubs with a real ELF/DWARF (or LLVM DebugInfo/PDB) symbol parser.

### Assembler (keystone)
- [ ] `assembler.cpp::AssembleData::Assemble` — Linux body stubbed (`return -1`).
  Needs a Linux Keystone build plus the two restored CMake lines; see the
  keystone section above.

### PCI ID database
- [x] `pci-id.cpp::GetVendorById` — done (2026-08-02). The Linux branch resolves
  the database through `SetupPathForFileName`, which normalizes the `'\\'` of
  `PCI_ID_DATABASE_PATH` (`pci-id.h:44`), so the constant itself stays shared.

### PE parsing
- [ ] Recreate Windows `IMAGE_*` headers for Linux and port `pe-parser.cpp`
  (replace `pe-parser-linux.cpp`). Affects `!pe`; `!hide` currently gets `0` for
  all syscall numbers (a Windows-guest feature, meaningless on a Linux host).

### Transport
- [ ] `platform-serial.c` Linux branch — implement termios serial I/O (currently stub).
- [ ] `platform-ioctl.c` Linux branch — needs a Linux kernel module + real ioctl
  (currently stub). This is the local driver interface used across many files.

### File location on disk — DONE (2026-08-02)

Three of the "find a file" stubs are now real. They are grouped here because the
first one is what unblocks the other two.

**`install-linux.cpp::SetupPathForFileName`** — the generic "find a file beside
my binary" helper, used for the driver path (`libhyperdbg.cpp`), the hwdbg
test/script files (`hwdbg-interpreter.cpp`, `hwdbg-scripts.cpp`), the exported
`hyperdbg_u_setup_path_for_filename` and now the PCI ID database. A 1:1 port of
the Windows body, call for call: `readlink("/proc/self/exe")` is the
`GetModuleFileName(GetModuleHandle(NULL), ...)` counterpart, `strrchr(..., '/')`
replaces `strrchr(..., '\\')`, and `access(path, F_OK)` replaces the
`CreateFile(..., OPEN_EXISTING)` probe behind `CheckFileExists`.

Two things the Windows version does not do:

- **`BufferLength` is honoured on every write.** `readlink` truncates silently,
  so a result that fills the buffer is rejected rather than handed back as a
  valid-looking path to the wrong file, and the directory + `'/'` + name + NUL
  total is checked before the append. (`install.cpp` gets the second half of
  this from `StringCbCat`, but not the first: its `GetModuleFileName` truncation
  is unchecked.)
- **Windows-style separators in `FileName` are normalized to `'/'`.** The file
  names come from shared headers and are spelled the Windows way —
  `HWDBG_TEST_READ_INSTANCE_INFO_PATH` (`HardwareDebugger.h:42`) and
  `PCI_ID_DATABASE_PATH` (`pci-id.h:44`). Doing it here keeps the constants
  shared and keeps the `#ifdef` out of every call site. Only the appended part
  is rewritten, so a `'\\'` inside a real Linux directory name survives.

**`common.cpp::ListDirectory`** — `opendir`/`readdir` with `fnmatch` for the
wildcard, mirroring how `FindFirstFileA` applies the pattern to the file name.
No `fnmatch` flag is passed, so a leading `'.'` is unremarkable and `"."`/`".."`
match a bare `"*"`, as they do in the Win32 walk. A missing directory throws the
same `std::runtime_error` as the `INVALID_HANDLE_VALUE` branch. One deliberate
difference: the result is sorted, because `readdir` returns entries in
file-system order and the caller (eval.cpp's test harness) is easier to compare
across runs when the order is stable.

**`pci-id.cpp::GetVendorById`** — the Linux `#else` builds the database path with
`SetupPathForFileName` and calls the same `GetVendorByIdStr` as Windows. This was
previously left Windows-only because wrapping just the two Win32 calls would have
left `strrchr` looking for a `'\\'`; the normalization above removes that
objection. `!pcitree` / `!pcicam` now resolve vendor and device names on Linux
whenever `constants/pci.ids` is deployed next to the binary.

Verified by extracting `SetupPathForFileName` and the Linux `ListDirectory`
branch into a standalone harness built with `-fsanitize=address,undefined`: 20
probes covering the separator normalization, the `CheckFileExists` outcomes, the
exact-fit and one-byte-short buffer boundaries, a buffer too small for the
executable path itself, the NULL/zero-length arguments, and the four
`ListDirectory` cases (filtering, sorting, empty match, missing directory). All
pass with no sanitizer diagnostics. The three modified files were also compiled
with the project's own Linux CMake build.

Still open: `GetConfigFilePath` remains stubbed. It is the same walk, but its
out-parameter is a `PWCHAR`, so it stays behind the wide-char item.

### Process control — `ud.cpp` DONE (2026-07-18)
Mechanical wrapper sweep (bucket 1): `DeviceIoControl`×10→`PlatformDeviceIoControl`,
`GetLastError`×10 (the `"ioctl failed"` sites)→`PlatformGetLastError`,
`RtlZeroMemory`×3→`PlatformZeroMemory`, `CloseHandle`×1 (event-handle close)→`PlatformCloseHandle`,
`CreateEvent(NULL,FALSE,FALSE,NULL)`→`PlatformCreateEvent(FALSE,FALSE)`.

Bucket 2 (Win32 process/thread mgmt) resolved two ways (user decision):
- **Group A — new guarded `Platform*` wrappers** (real on Windows, Linux stub) for the
  self-contained calls: `PlatformCreateProcess` (keeps `STARTUPINFO` internal),
  `PlatformOpenProcess`, `PlatformTerminateProcess`, `PlatformResumeThread`,
  `PlatformGetExitCodeProcess` — all in `platform-lib-calls.{h,c}`. ud.cpp call sites
  swapped 1:1; `UdCreateSuspendedProcess` now calls `PlatformCreateProcess` with the
  `CREATE_SUSPENDED|CREATE_NEW_CONSOLE` flags kept at the call site.
- **Group B — whole-body `#ifdef _WIN32` guards** (Windows verbatim, Linux stub) for the
  calls interleaved with UI/walk logic: `UdListProcessThreads`, `UdCheckThreadByProcessId`
  (Toolhelp snapshot walk), `UdPrintError` (`FormatMessage`/`MAKELANGID`). No Linux
  Toolhelp/`THREADENTRY32` types needed.
- **Pure additions:** Linux `PROCESS_INFORMATION` struct in `SDK/headers/BasicTypes.h`;
  `PROCESS_TERMINATE`/`PROCESS_QUERY_LIMITED_INFORMATION`/`CREATE_SUSPENDED`/
  `CREATE_NEW_CONSOLE`/`STILL_ACTIVE` `#define`s in `Environment.h` (Linux block).

TODO(Linux) still open in the wrapper bodies: real `fork`+`execve`/`ptrace` process
backend, and the Toolhelp thread-enumeration equivalent (`/proc`) — all stubbed for now.

### Process control — `pt.cpp` NOT STARTED

`extension-commands/pt.cpp` (Intel PT) is the last file in `commands/` with raw
Win32 calls — 27 sites, and no `#ifdef` guards anywhere in the file. This is
**not** a mechanical sweep: it is bucket-2 work structurally like `ud.cpp` was, so
it needs the same Group A / Group B decision per call site before anything is
swapped. What is in there:

- Toolhelp process/thread snapshot walk (`CreateToolhelp32Snapshot` + enumeration)
- `OpenProcess` / `OpenThread` + handle lifetime (`CloseHandle` ×8)
- Thread affinity pinning (`SetThreadAffinityMask`)
- Trace-thread lifecycle (`CreateEvent` / `CreateThread` / stop-event signalling)
- Two `DeviceIoControl` + `GetLastError` pairs (these two *are* bucket-1 and could
  be swapped in isolation, but leaving the file wholly untouched is cleaner than a
  half-ported file)

- [ ] Decide Group A vs Group B per call site, then port `pt.cpp`.

### `PlatformTerminateThread` — Linux stub

- [ ] Real teardown for the remote-debuggee listening thread.

There is **no POSIX equivalent to `TerminateThread` by design**: nothing forcibly
kills a thread without unwinding, because doing so never releases the target's
locks. `pthread_cancel` is the nearest primitive but differs semantically —
deferred by default, and glibc implements it as a forced unwind that runs
destructors and cleanup handlers (any `catch (...)` that does not rethrow breaks
it). `PTHREAD_CANCEL_ASYNCHRONOUS` recovers the abruptness at the cost of
undefined behaviour for anything not async-cancel-safe.

The only caller does not need a cancellation primitive at all.
`RemoteConnectionThreadListeningToDebuggee` (`remote-connection.cpp:211`) is a
`while (g_IsConnectedToRemoteDebuggee)` loop that blocks in `recv()` and `break`s
on any receive error. Correct Linux teardown is therefore: clear the flag →
`shutdown(fd, SHUT_RDWR)` to kick the thread out of `recv` → `pthread_join`. It
exits through its own existing error path.

Deferred because that requires a **call-site reorder** in `disconnect.cpp`
(teardown before thread-kill instead of after) plus `SD_SEND` → `SHUT_RDWR` in
`CommunicationClientShutdownConnection` to wake a blocked *reader* — a behaviour
change to shared logic, which the no-logic-changes rule rules out mid-port.

Also blocked on `PlatformCreateThread`, which returns NULL on Linux, so the
listening thread never starts there in the first place.

**Observation, not this batch's job:** on Windows the current ordering kills the
listener while it may be mid-`recv` holding the socket, and only *then* shuts the
socket down. That looks like a latent upstream bug — worth raising separately,
but out of scope for the port.

### Misc runtime stubs
- [ ] `PlatformGetOsVersion` — Linux returns FALSE; implement via `uname`.
- [ ] `$peb` pseudo-register — returns 0 on Linux (PEB is NT-only).
- [ ] `DebuggerGetNtoskrnlBase` — returns NULL on Linux (NT system-module enum).
- [ ] File I/O / user-debugger paths — stubbed (also blocked on wide-char above).
- [x] `common.cpp::ListDirectory` — done (2026-08-02) with `opendir`/`readdir` +
  `fnmatch`.
- [ ] `common.cpp::GetConfigFilePath` — Linux empties the path; resolve via
  `readlink("/proc/self/exe")` + append `CONFIG_FILE_NAME` (still blocked on
  wide-char — `SetupPathForFileName` is the `CHAR` counterpart and is done).
- [ ] `common.cpp::SetPrivilege` / `IsFileExistW` — Linux stubs (no callers /
  wide-char deferred respectively).
- [x] `cpu.cpp:148` & `cpu.cpp:200` — same `CpuIdEx`→`CpuCpuIdEx` typo fix as
  common.cpp. Done.

### Build system
- [ ] Add `.gitignore` rules for the in-source CMake build output
  (`CMakeCache.txt`, `CMakeFiles/`, generated `Makefile`, `cmake_install.cmake`,
  `*.o`, `*.so`) — or switch to an out-of-source `build/` directory. Now also
  applies to the kbuild output (`*.ko`, `*.mod`, `*.mod.c`, `.*.cmd`,
  `modules.order`, `Module.symvers`), which lands in-tree at the repo root.

---

## Kernel module (`HyperDbg.ko`) — IN PROGRESS

Phase two. Windows' seven kernel binaries (hyperkd.sys + hyperhv/hyperlog/
hyperevade/hypertrace/hyperperf/kdserial, KMDF export drivers) collapse into one
Linux module. Design notes: the root [`Kbuild`](../Kbuild) header.

```bash
cd linux/kernel     # NOT the CMake root Makefile — that builds the user-mode CLI
make                # -> HyperDbg.ko at the repo root
make clean / load / unload
make KDIR=/path/to/linux-headers
```

kbuild prefers the root `Kbuild` under `M=`, so both build systems coexist.
`Kbuild` lists every future object with the un-ported ones commented out. The
`include/platform/kernel/` layer (13 TUs) is now **complete** — all compile and
link into `HyperDbg.ko`. Next front is the subsystems above it (hyperlog first).

| TU | Status |
|----|--------|
| `PlatformMem.c` | ✅ builds (pre-existing) |
| `PlatformIntrinsics.c` | ✅ builds (pre-existing) |
| `PlatformIntrinsicsVmx.c` | ✅ builds (pre-existing) |
| `PlatformBroadcast.c` | ✅ ported 2026-08-01 |
| `PlatformCpu.c` | ✅ ported 2026-08-01 — see below |
| `PlatformDbg.c` | ✅ ported 2026-08-01 — see below |
| `PlatformDpc.c` | ✅ ported 2026-08-11 — BH-workqueue skeleton, see below |
| `PlatformEvent.c` | ✅ ported 2026-08-11 — stub skeleton, see below |
| `PlatformIo.c` | ✅ ported 2026-08-11 — stub skeleton, see below |
| `PlatformIrql.c` | ✅ ported 2026-08-11 — **real** (preempt_disable/enable) |
| `PlatformProcess.c` | ✅ ported 2026-08-11 — mostly real (`current`/task ids), $teb stub |
| `PlatformSpinlock.c` | ✅ ported 2026-08-11 — **real** (spinlock_t / spin_lock) |
| `PlatformTime.c` | ✅ ported 2026-08-11 — **real** (ktime + epoch conversion) |

**🎉 The entire `platform/kernel` OS-abstraction layer now compiles and
`HyperDbg.ko` links with all 13 TUs active (2026-08-11).** No `#error` stubs left
in the layer. Then `components/` (2026-08-14) and `hyperlog/` (2026-08-14, see
below) came online on top of it. **Next front: `script-eval/` — `Functions.c`
first (`wchar_t` has no kernel definition, plus `-Werror=return-type` on the
UNREACHABLE-style tails).**

### `PlatformCpu.c` — DONE (2026-08-01)

| Windows | Linux |
|---------|-------|
| `KeQueryActiveProcessorCount(0)` | `num_online_cpus()` |
| `KeGetCurrentProcessorNumberEx(NULL)` | `raw_smp_processor_id()` |

Both Windows calls return system-wide (all-group) values, matching Linux's flat
CPU numbering — no group translation.

`raw_` chosen deliberately: plain `smp_processor_id()` asserts preemption is
disabled (`CONFIG_DEBUG_PREEMPT` splat), a precondition the Windows API doesn't
have and no caller establishes. Both current callers can run preemptible
(`Logging.c:1001` per-core VMX buffer index, but only on the vmx-root path;
`PseudoRegisters.c:49` `$core`). The 72 raw `KeGetCurrentProcessorNumberEx` sites
in 21 files still pending (`__CPU_INDEX__`, `g_GuestState[]`/`g_DbgState[]`) are
all vmx-root/raised-IRQL — identical either way.

No new include needed (resolves via `linux/kernel/pch.h`; add `<linux/smp.h>` if
that ever breaks).

### `PlatformDbg.c` — DONE (2026-08-01)

| Windows | Linux |
|---------|-------|
| `vDbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, Format, ArgList)` | `vprintk(Format, ArgList)` |

`va_start`/`va_end` scaffolding unchanged; no new include needed.

Nuances, deliberately not "fixed" (would be logic changes; callers unaffected):
- printk reads its level from a `KERN_*` prefix on the format string and no caller
  has one → messages land at the default loglevel, not "info".
- printk isn't MSVC's CRT (no `%I64d`/`%ws`). All six callers use only `%s`/`%x`.
- `Logging.c:833` chunks at `DbgPrintLimitation` (512, `Constants.h:223`) =
  DbgPrint's limit, not printk's `LOG_LINE_MAX`. Works, just mis-sized.

- [ ] Revisit loglevel + `DbgPrintLimitation` sizing once kernel logging is exercised.

### `PlatformDpc.c` — DONE (2026-08-11) — BH-workqueue skeleton

Backing = **bottom-half (BH) workqueue** (`system_bh_wq`, kernel ≥6.9): runs in
softirq context like a DPC/DISPATCH_LEVEL, but via the non-deprecated workqueue
API (tasklets are deprecated). Only consumer is hyperlog's log→usermode notify
path. Linux `KDPC` type added to `DataTypes.h` (guard `__linux__ &&
HYPERDBG_KERNEL_MODE`; embeds `struct work_struct` + the routine/context/args) —
must be a real struct since it's stored by value in `NOTIFY_RECORD`
(`Logging.h:49`). `<linux/workqueue.h>` → `linux/kernel/pch.h`; `PlatformDpc.h`
prototypes widened to `|| __linux__`.

| Windows | Linux |
|---------|-------|
| `KeInitializeDpc` | stash routine/ctx + `INIT_WORK(&Work, trampoline)` |
| `KeInsertQueueDpc` | stash args + `queue_work(system_bh_wq, &Work)` |

Work callback gets only the work_struct ptr, so a `container_of` trampoline
replays the Windows 4-arg call. `queue_work()` returns false when already pending,
which matches `KeInsertQueueDpc`'s "FALSE if already queued" — returned directly.
**SKELETON — compile-clean, not runtime-tested:** no teardown (a queued work item
in a freed `NOTIFY_RECORD` = UAF; needs `cancel_work_sync` before free once
hyperlog is ported). No live caller yet (hyperlog not compiled).

### `PlatformEvent.c` — DONE (2026-08-11) — stub skeleton

The EVENT_BASED half of the same hyperlog notify path: user-mode passes an event
*handle*, the kernel references it to a `KEVENT` and later signals it. No direct
Linux analog (no NT handles/object-manager); the real backing would be **eventfd**
(deferred — it also needs a coordinated user-mode-side change). Stubbed for now.

Type plumbing added to `BasicTypes.h` Linux block (shared, reused by later kernel
TUs): `NTSTATUS`/`KPRIORITY`/`ACCESS_MASK`/`KPROCESSOR_MODE` scalars, opaque
`KEVENT`/`POBJECT_TYPE`/`POBJECT_HANDLE_INFORMATION`, and
`STATUS_SUCCESS`/`STATUS_NOT_IMPLEMENTED`/`NT_SUCCESS`. Sole definitions (no
collision — the two other `NTSTATUS` hits are *uses* in kernel headers).

| Windows | Linux stub | eventfd TODO |
|---------|-----------|--------------|
| `ObDereferenceObject` | no-op | `eventfd_ctx_put` |
| `KeSetEvent` | return 0 | `eventfd_signal` |
| `ObReferenceObjectByHandle` | `*Object=NULL`, return `STATUS_NOT_IMPLEMENTED` | `eventfd_ctx_fdget` |

Consequence: EVENT_BASED notify is a no-op/fail-closed on Linux until eventfd
lands. No live caller yet (hyperlog not compiled).

**Header cleanup (both Dpc + Event):** the `#if _WIN32 || _WIN64 || __linux__`
wrappers around the prototypes were removed — that condition is every supported
platform (the `.c` files `#error` on anything else), so the prototypes are now
declared unconditionally.

### `PlatformIo.c` — DONE (2026-08-11) — stub skeleton

The IRP (I/O Request Packet) third of the same hyperlog notify path. No NT IRP on
Linux — a real version maps the IRP onto the char-device read/ioctl request that
carries the notify buffer (deferred with the rest of the device layer). Stubbed.

Opaque types added to `BasicTypes.h` Linux block: `CCHAR` + forward-declared
`IRP`/`PIRP` and `IO_STACK_LOCATION`/`PIO_STACK_LOCATION` (enough for the
signatures; the member layouts are only needed once Logging.c compiles).

| Windows | Linux stub |
|---------|-----------|
| `IoGetCurrentIrpStackLocation` | return `NULL` |
| `IoCompleteRequest` | no-op |
| `IoMarkIrpPending` | no-op |

Prototype guard removed (unconditional, same as Dpc/Event). No live caller yet
(hyperlog not compiled).

### `PlatformIrql.c` — DONE (2026-08-11) — **real mapping** (not a stub)

DISPATCH_LEVEL = preemption off, hardware interrupts still on = Linux
`preempt_disable()`. (`local_irq_save` would be a *higher* level — wrong.) The
one caller (`Logging.c:387/406`) raises to hold off preemption while it takes a
lock, then lowers — a balanced pair, both arms under the same `if (!IsVmxRoot)`.

| Windows | Linux |
|---------|-------|
| `KeRaiseIrqlToDpcLevel()` | `preempt_disable()`; return 0 |
| `KeLowerIrql(OldIrql)` | `preempt_enable()` (OldIrql ignored) |

Verified against `include/linux/preempt.h`: `preempt_disable`/`preempt_enable`
are `preempt_count_inc`/`dec` — a **nesting counter**, no saved token. So the
returned `KIRQL` is unused (Linux restores by decrement, not to an absolute
level); correctness only needs the raise/lower to be **balanced**, which the
caller is. `preempt_enable`'s reschedule-on-zero even mirrors `KeLowerIrql`
draining pending DPCs. `KIRQL` (`UCHAR`) added to `BasicTypes.h`; guard removed.
`preempt_*` resolve transitively via pch — no extra include.

### `PlatformProcess.c` — DONE (2026-08-11) — mostly real

Five current-process/thread queries, all called from `PseudoRegisters.c`
(kernel branch: `$tid`/`$pid`/`$pname`/`$proc`/`$thread`/`$teb`).

| Windows | Linux |
|---------|-------|
| `PsGetCurrentThreadId` | `(HANDLE)(uintptr_t)task_pid_nr(current)` — `tsk->pid` = TID |
| `PsGetCurrentProcessId` | `(HANDLE)(uintptr_t)task_tgid_nr(current)` — `tsk->tgid` = PID |
| `PsGetCurrentProcess` | `(PVOID)current->group_leader` |
| `PsGetCurrentThread` | `(PVOID)current` (a task *is* a thread) |
| `PsGetCurrentThreadTeb` | `NULL` — no Linux TEB (cf. `$peb`) |

Watch the kernel's inverted naming: `task->pid` is the *thread* id, `task->tgid`
the *process* id — so Id uses `task_pid_nr`, ProcessId uses `task_tgid_nr` (NOT
the reverse). `pid_t`→`HANDLE` needs the `(uintptr_t)` cast (else `-Wint-conversion`).
`GetCurrentProcess` uses `current->group_leader` (NOT `current`): `PsGetCurrentProcess`
is per-*process* — identical for all threads — so it must be the thread-group leader,
not the per-thread `current` (which `GetCurrentThread` correctly returns). `current`/
`task_*_nr`/`uintptr_t` resolve via pch. **Follow-up:** `$pname` name won't work until
`CommonGetProcessNameFromProcessControlBlock` (hyperhv/hyperkd) is ported to read
`((struct task_struct *)Eprocess)->comm`.

### `PlatformSpinlock.c` — DONE (2026-08-11) — **real mapping**

`KeAcquireSpinLock` raises to DISPATCH_LEVEL (preempt off, HW IRQs on) + acquires
= Linux `spin_lock()`; same DISPATCH↔preempt mapping as `PlatformIrql`. All
callers are the hyperlog buffer locks (`Logging.c`).

| Windows | Linux |
|---------|-------|
| `KeInitializeSpinLock` | `spin_lock_init` |
| `KeAcquireSpinLock(l,&old)` | `spin_lock(l)`; `*OldIrql = 0` |
| `KeReleaseSpinLock(l,old)` | `spin_unlock(l)` (OldIrql ignored) |

`KSPIN_LOCK` is stored **by value** in `LOG_BUFFER_INFORMATION` (`Logging.h:70`),
so the Linux type is a real `spinlock_t` (typedef in `DataTypes.h`, kernel-guarded
like `KDPC`; `<linux/spinlock.h>` → pch). `PKIRQL` added to `BasicTypes.h`. The
`OldIrql` token is vestigial (preempt is a nesting counter). Chose `spin_lock`
over `spin_lock_irqsave` deliberately: irqsave disables IRQs (higher than Windows
takes here) *and* its `unsigned long flags` wouldn't fit the 1-byte `KIRQL`. If a
future caller takes one of these locks from hard-IRQ context, revisit (irqsave,
with flags stored in the lock struct). Guard removed.

### `PlatformTime.c` — DONE (2026-08-11) — **real** (log timestamps)

Sole caller: `Logging.c:1072` builds a log-message timestamp
(QuerySystemTime→ConvertToLocalTime→ConvertToTimeFields). NT time is 100-ns ticks
since **1601**; Linux gives Unix-epoch (1970) — bridged by two constants
(`HUNDRED_NS_PER_SEC`, `EPOCH_DIFF_1601_TO_1970_SECS = 11644473600`).

| Windows | Linux |
|---------|-------|
| `KeQuerySystemTime` | `ktime_get_real_ts64` → `(sec + epochdiff)*1e7 + nsec/100` |
| `ExSystemTimeToLocalTime` | subtract `sys_tz.tz_minuteswest*60*1e7` (0 if RTC=UTC) |
| `RtlTimeToTimeFields` | `time64_to_tm` → fill `TIME_FIELDS` (Year=tm_year+1900, Month=tm_mon+1, Weekday Sun=0) |

`TIME_FIELDS` added to `DataTypes.h` (plain struct). `<linux/timekeeping.h>` +
`<linux/time.h>` included locally in the `.c` (not pch — no shared type embeds
them). All three kernel symbols are module-exported (verified). Guard removed.
Note: upstream `Logging.c` currently has the timestamp *formatting* commented out
(`RtlStringCchPrintfA` not IRQL-safe), so the value is computed but not yet printed.

### `components/` layer — PARTIAL (2026-08-14)

Whole `components/` block had been uncommented in `Kbuild` optimistically, but only
three of six TUs are self-contained. Active now (compile clean, in `HyperDbg.ko`):
`spinlock/Spinlock.o`, `optimizations/AvlTree.o`, `optimizations/InsertionSort.o`.
Re-commented (hyperlog-gated — need `Log`/`LogInfo` + the `g_Callbacks` global):
`optimizations/BinarySearch.o`, `optimizations/OptimizationsExamples.o`,
`callback/HyperLogCallback.o`. Uncomment those alongside `hyperlog/Logging.c`.

**`Spinlock.c`** — three MSVC intrinsics routed through the platform-intrinsics
layer (chosen over file-local shims: keeps the `Cpu*` wrappers the single intrinsic
boundary). Two new kernel wrappers added to `PlatformIntrinsics.{h,c}` (both with
win + linux arms, mirroring the existing 64-bit CAS):

| Spinlock.c call | new wrapper | Linux impl |
|-----------------|-------------|-----------|
| `_mm_pause()` | `CpuPause()` (already existed) | `__asm__("pause")` |
| `_interlockedbittestandset(l,0)` | `CpuInterlockedBitTestAndSet` | `__atomic_fetch_or` → old bit |
| `InterlockedCompareExchange` | `CpuInterlockedCompareExchange` (32-bit) | `__atomic_compare_exchange_n` |

`HyperDbg.ko` links clean with these three components active.

Windows fallout (fixed same day): `hyperlog` was the one MSVC project that compiles
`Spinlock.c` without `PlatformIntrinsics.c`, so the new `Cpu*` calls broke its build
(C4013 + would not have linked). Added `PlatformIntrinsics.c` to `hyperlog.vcxproj`
(+ `.filters`) and `PlatformIntrinsics.h` to `hyperlog/header/pch.h`, matching what
`hyperperf`/`hypertrace`/`hyperkd`/`hyperevade`/`hyperhv` already do. Rule of thumb:
any new `Cpu*` call inside `include/components/` must be checked against every
`.vcxproj` that compiles that component.

### `hyperlog/` — DONE (2026-08-14) — `Logging.c` + `UnloadDll.c`

`HyperDbg.ko` links with the whole message-tracing layer active, plus the two
components that were waiting on it (`BinarySearch.o`, `OptimizationsExamples.o`).

| Windows | Linux |
|---------|-------|
| `vsprintf_s` / `sprintf_s` / `strnlen_s` | new `PlatformStr.{h,c}` → `vsnprintf` / `strnlen` |
| `RTL_NUMBER_OF`, `ASSERT`, `_Analysis_assume_` | `Environment.h` macros (`ASSERT` → `WARN_ON`) |
| `IRP`, `IO_STACK_LOCATION`, `IO_STATUS_BLOCK`, `UNICODE_STRING` | real structs in `BasicTypes.h` (WDK member names, NOT the NT layout) |
| `STATUS_PENDING/INVALID_PARAMETER/INSUFFICIENT_RESOURCES`, `IO_NO_INCREMENT`, `SYNCHRONIZE`, `EVENT_MODIFY_STATE` | same values, `BasicTypes.h` |
| `ExEventObjectType` (ntddk global) | placeholder token defined in `PlatformEvent.c` |

The IRP structs were the one real decision: `Logging.c` dereferences IRP members
directly, so the opaque `struct _IRP` shim had to grow the members the shared code
touches (the header already said this was owed once hyperlog compiled). Nothing
constructs one on Linux yet — `PlatformIo` is still stubbed, so the whole
IRP/EVENT notify path is compile-only until the char device lands.

`PlatformSprintf` already existed, misplaced, in `PlatformMem.{h,c}` with no
callers; moved into `PlatformStr` so both spellings share the "-1 on truncation"
semantics that `Logging.c` tests for. `PlatformStr.c` is wired into `Kbuild` +
`hyperlog.vcxproj`/`.filters` + both pch files.

Two Linux-only edits inside shared code: `(PVOID *)` cast on the
`PlatformObjectReferenceByHandle` out-param (`PKEVENT *` → `PVOID *`, which the
kernel build treats as an error and MSVC accepts), and the `#if defined(__linux__)`
include block at the top of `Logging.c`.

**DROPPED (Windows-only): `components/callback/HyperLogCallback.c`.** It *defines*
the same `LogCallback*` entry points as `Logging.c` — on Windows it is the per-DLL
forwarding shim (hyperhv/hypertrace/hyperperf cannot call into `hyperlog.dll`, so
each compiles it to bounce through its own `g_Callbacks`). One Linux module = the
real implementation is already linked in, so it is redundant *and* a duplicate
symbol. Kept commented out in `Kbuild` with that note.

The unified `linux/kernel/pch.h` gained `config/Configuration.h`, the hyperlog
SDK module/imports/intrinsics headers (the `Log`/`LogInfo` macros — on Windows
every project pch has these) and the three `components/optimizations` headers.

### WDK type shims — `WdkTypes.h` (2026-08-14)

New `include/platform/general/header/WdkTypes.h` collects **every** Linux stand-in
for a WDK/NT type, status code and ntdef macro in one place, instead of leaving
them spread through `BasicTypes.h` / `Environment.h`. The ones written earlier
(NTSTATUS, KEVENT, IRP, IO_STACK_LOCATION, UNICODE_STRING, KIRQL, STATUS_*, the
access masks) moved there too; `BasicTypes.h` now just includes it, positioned so
the scalar typedefs above are in scope and DataTypes.h is not needed.

Declarations are labelled by how much they can be trusted — **faithful** (means
the same on both sides), **structural** (WDK member names, non-NT layout, nothing
builds one yet), **opaque** (correctly-sized blob passed only by pointer). New
this round: PEPROCESS/PETHREAD, CLIENT_ID, OBJECT_ATTRIBUTES +
InitializeObjectAttributes, KAPC_STATE, ACCESS_STATE, GENERIC_MAPPING,
PROCESS_BASIC_INFORMATION, PROCESSINFOCLASS, MODE (KernelMode/UserMode),
DISPATCHER_HEADER, DEVICE/DRIVER_OBJECT, UNICODE_STRING32, LIST_ENTRY32, PWSTR,
SSIZE_T, the Rtl memory macros, CONTAINING_RECORD, PAGE_ALIGN, PAGED_CODE,
NTKERNELAPI, and the pre-SAL `IN`/`OUT`/`__in` annotation family.

Two of these had to become **complete** types rather than opaque handles, because
shared code embeds them by value: `KEVENT` (hyperkd's globals hold one) and
`DISPATCHER_HEADER` (inside NT_KPROCESS). Both are sized blobs with the reason
written at the declaration.

### The unified pch now DELEGATES (2026-08-14)

`linux/kernel/pch.h` no longer copies a module's include list — it includes the
module's own pch (`#include "../../hyperkd/header/pch.h"`), reached via
`-I$(src)/hyperkd` + `-I$(src)/hyperkd/header`. Those lists stay owned by
upstream and cannot drift. This only became possible once WdkTypes.h existed:
delegating before it broke every green TU (`PlatformMem.c` 0 -> 48 errors), and
after it that TU is back to 0.

Measured on the hyperkd block (33 TUs, none ported): **1528 -> 157 errors**, 5 TUs
now compile clean. ~79% of the original count was never WDK work at all, just
hyperkd's own declarations being invisible.

Also added to `Kbuild`: `-fcommon`. Every module keeps its globals as bare
tentative definitions in a header its pch pulls into every TU
(`KEVENT g_UserDebuggerWaitingCommandEvent;`). MSVC merges those; GCC has
defaulted to `-fno-common` since 10, which makes them a link error. `-fcommon`
restores the model the source was written against — the alternative was rewriting
every globals header to extern + one defining TU, which is a refactor of shared
Windows code.

What the remaining 157 actually are: WDK calls that ALREADY have Platform
wrappers and just need call-site swaps when each file is ported
(`PsGetCurrentProcessId` 13, `KeGetCurrentProcessorNumberEx` 8,
`KeQueryActiveProcessorCount` 7, `DbgBreakPoint` 17), plus genuine hyperhv
symbols (`LayoutGetCurrentProcessCr3` 13,
`CommonGetProcessNameFromProcessControlBlock` 8) that stay unresolved until the
VMM lands.

### `hyperhv/` — mechanical pass (2026-08-17): 143 -> 79 errors, 29/55 TUs clean

Measured per TU with `make one`. Nothing here is a design decision; the open ones
are listed at the end.

| Windows | Linux |
|---------|-------|
| `KeGetCurrentProcessorNumberEx(NULL)` (17), `KeQueryActiveProcessorCount(0)` (15) | `PlatformCpu*` |
| `PsGetCurrentProcessId/ThreadId/Process` (12), `ObDereferenceObject` (8) | `PlatformProcess*` / `PlatformObjectDereference` |
| `KeRaiseIrqlToDpcLevel` / `KeLowerIrql` | `PlatformIrql*` |
| `KeInitializeDpc` / `KeSetTargetProcessorDpc` / `KeInsertQueueDpc` | `PlatformDpc*` |
| `_bittest`, `__readpmc`, `_xsetbv`, `InterlockedExchange` | new `CpuBitTest`, `CpuReadPmc`, `CpuXsetbv`, `CpuInterlockedExchange` |
| `_inp/_inpw/_inpd`, `_outp/_outpw/_outpd` (Pci.c) | the `CpuIoIn*`/`CpuIoOut*` wrappers that already existed |
| `NT_ASSERT`, `KAFFINITY`, `MAXDWORD64`, `PAGE_*`, `MEM_*` | `WdkTypes.h` |
| `InitializeListHead`, `InsertHeadList`, `RemoveEntryList` | `nt-list.h`, now reachable from the kernel build |

`hyperhv/pch.h` gained the 5 platform headers those wrappers live in (Cpu, Dpc,
Event, Irql, Process); it only had Mem/Intrinsics/Broadcast/IntrinsicsVmx.

**`nt-list.h` reaches the kernel via `DataTypes.h`**, included right after the
`LIST_ENTRY` layout it operates on (the way `BasicTypes.h` includes
`WdkTypes.h`) — it was previously only in libhyperdbg's user-mode pch. Its
`#include <stddef.h>` had to become `<linux/stddef.h>` under
`HYPERDBG_KERNEL_MODE`: the module builds `-nostdinc`.

**`PlatformDpcSetTargetProcessor` is new.** Windows picks the DPC's CPU with a
setter on the object; Linux picks it as an argument at queue time
(`queue_work_on`). So the Linux `KDPC` carries a `TargetCore` (`-1` =
`KDPC_NO_TARGET_CORE`, set by `PlatformDpcInitialize`) and
`PlatformDpcInsertQueueDpc` applies it. Keeping the state means the call sites
keep their 1:1 Windows shape; every existing hyperlog caller stays on the
unpinned `queue_work` path.

**`KIDT_ENTRY`'s 64-bit half was invisible** — `IdtEmulation.h:55` guarded it with
`_M_AMD64`, which is MSVC's spelling only, so the GCC build silently lost
`HighestPart` + `Reserved`. Guard now also accepts `__x86_64__`.

**`_M_AMD64` is NOT defined globally for Linux** (the obvious shortcut): Zydis and
ia32-doc test it to select MSVC intrinsics, so defining it would steer them into
Windows-only code paths.

**⚠️ A Platform swap is TWO edits, and the Linux build cannot see the second one.**
Windows builds each driver as its own DLL that compiles only the `Platform*.c`
files listed in ITS `.vcxproj`; Linux links one module, so every wrapper is always
present. Swapping a call site therefore links fine here and fails on the Windows
CI with `LNK2001: unresolved external symbol Platform...`. This pass needed
`PlatformCpu/Dpc/Event/Irql/Process.c` added to `hyperhv.vcxproj` (+ `.filters`),
which had only Broadcast/Intrinsics/IntrinsicsVmx/Mem.

What each project compiles today, i.e. what the next sweep has to top up:

| project | has |
|---|---|
| hyperkd | Broadcast, Cpu, Intrinsics, Mem, Process, Str |
| hypertrace / hyperperf | Broadcast, Cpu, Intrinsics, Mem |
| hyperevade | Intrinsics, Mem |
| hyperlog | everything except Broadcast/IntrinsicsVmx |

Open, all needing a decision before hyperhv finishes: `DbgBreakPoint` (16 errors —
it comes from the `LogError` macro, so it hits nearly every file),
`KeGenericCallDpc` (3), the `Mm*`/`Zw*` memory family (~15), TSX `_xbegin/_xend`,
`ZydisKernel.c` (wants `ntimage.h`), and the 4 `__try`/`__except` files, which are
deliberately deferred to their own step.

### Integer-width typedefs: Windows LLP64 vs Linux LP64 (2026-08-17)

`BasicTypes.h` typedef'd `LONG`/`ULONG`/`DWORD` to `long`, which is **32-bit on
Windows and 64-bit on Linux**. Everything built, but the widths silently
disagreed with the Windows source's assumptions. Two failures made it visible:
`Pci.c`'s `switch` on `sizeof(DWORD)` vs `sizeof(QWORD)` became a *duplicate case*
(both 8), and `KIDT_ENTRY`'s `ULONG x : 16` bitfields were laid out in 64-bit
units.

They are now `int`/`unsigned int` on Linux (Windows keeps `long`, matching the
WDK's own typedefs — redefining them there would clash with `ntdef.h`).

Cost across BOTH builds: **one** cast site — `script-eval/Functions.c` declares a
parameter as raw `volatile long *` and passes it to a `volatile LONG *` API, which
is the same type only on Windows.

Still on `size_t`: `SIZE_T` (Windows: `unsigned __int64`). That is what the
5 remaining `CpuStosQ` and the `VmxVmread64P` pointer errors are.

### Found while validating the above: the user-mode build was broken (2026-08-17)

Unrelated to the kernel work, pre-existing on the branch, now fixed:

- `libhyperdbg/CMakeLists.txt` listed `ucpuid.cpp` with no path — CMake could not
  find it and generated **zero** sources for the whole target.
- `uin.cpp` / `uout.cpp` are in `libhyperdbg.vcxproj` but were never added to
  CMake, so `CommandUserIn`/`CommandUserOut` were undefined at link.
- `WdkTypes.h` `#define __reserved` (a pre-SAL annotation with **no users** in the
  tree) erased the member of that name in glibc's `<linux/stat.h>`
  (`struct statx_timestamp`), breaking every TU that included `<sys/stat.h>`
  after the SDK. Removed, with the reason recorded at the site.

Still missing from CMake vs the vcxproj, NOT added (they need a look first):
`../include/components/pe/code/pe-image-reader.cpp` and
`code/debugger/misc/pt-helper.cpp` (pt.cpp itself is Linux-stubbed).

---

## Building

```bash
cmake .   # re-run only when CMake files change (user-mode CLI)
make      # build; find the next file that fails, port it, repeat
```

### Kernel module — file-by-file port loop

From `linux/kernel/`:

```bash
make one FILE=include/platform/kernel/code/PlatformDpc.c  # compile ONE TU, no link
# ...stub out the WDK bits until it compiles clean...
# then move its line into the ACTIVE block of ../../Kbuild and:
make            # full module build -> ../../HyperDbg.ko
```

`make one` wraps kbuild's single-object target (`make -C $KDIR M=$ROOT File.o`):
it compiles just that TU with the Kbuild `ccflags-y` include paths and **does not
link the module**, so you see only that file's own WDK/type errors instead of a
wall of undefined-symbol link errors from the not-yet-ported files. The file need
not be in `Kbuild` yet. Keep `HyperDbg-objs` = only files that compile clean, so a
full `make` always yields a loadable `.ko`; promote each file's line once `make
one` on it is green.
