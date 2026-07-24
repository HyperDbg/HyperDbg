# HyperDbg Linux Port — Status & TODO Ledger

This file tracks **what has been changed** for the Linux port and **what is still
stubbed / deferred**, so that when the port compiles end-to-end we have a single
list of the shortcuts that must be revisited before Linux is actually functional.

It complements [`README.md`](README.md) (the contributor how-to). This file is
the *state* of the work; the README is the *method*.

> Status in one line: the userspace library (`libhyperdbg`) compiles file-by-file
> on Linux. Many Windows-only paths are **stubbed to compile+link**, not yet
> implemented. See the TODO ledger below.

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
| `.../driver-loader/install-linux.cpp` | `install.cpp` (SCM driver loader) | The 2 Linux-visible public fns: `ManageDriver` (→ FALSE) and `SetupPathForFileName` (→ FALSE). The 4 `SC_HANDLE` helpers (`InstallDriver`/`RemoveDriver`/`StartDriver`/`StopDriver`) are guarded out of `install.h` on Linux (never referenced there). | `ManageDriver`: load/unload a future HyperDbg Linux kernel module via `finit_module`/`delete_module` (needs CAP_SYS_MODULE). `SetupPathForFileName`: `readlink("/proc/self/exe")` + strip + append + `access()` (generic "find a file beside my binary"; also used by hwdbg). |
| `.../communication/namedpipe-linux.cpp` | `namedpipe.cpp` (Win32 named-pipe IPC) | All 10 public `NamedPipeServer*`/`NamedPipeClient*` fns. `Create*` → `INVALID_HANDLE_VALUE` (print); send/read → 0/FALSE; close → no-op (quiet, unreachable once Create fails). The two internal `*Example()` demos are not in the Linux TU. | Back with a filesystem FIFO (`mkfifo`) or, better for framed bidirectional messages, an `AF_UNIX` socket derived from the `\\.\pipe\NAME` string; overlapped/event I/O collapses to blocking `read`/`write`. |

All four self-guard with `#ifdef __linux__` and print
`"... is not supported on Linux yet"` at runtime (named-pipe: only in the
`Create*` entry points, to avoid per-loop spam).

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
    `GetConfigFilePath` (`GetModuleFileNameW`/shlwapi; wide-char deferred → empties path),
    `ListDirectory` (`FindFirstFileA`; → empty vector, only caller is eval.cpp test harness).
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
Windows-only and Linux returns NULL. `GetVendorByIdStr`, `GetDeviceFromVendor`,
`FreeVendor` and `FreePciIdDatabase` are plain C and compile unchanged.
Consequence: `!pcitree` / `!pcicam` show no vendor or device names on Linux.

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

**Result: `hyperdbg-cli` links and runs on Linux for the first time.** The binary
starts, prints its banner and reaches the `HyperDbg>` prompt, and `.exit` exits
cleanly (0). The port is out of the compile/link phase and into the runtime phase.

- [ ] Port `pt.cpp` for real — see the process-control entry in the TODO ledger.

---

## TODO ledger — revisit before Linux is functional

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
- [ ] `pci-id.cpp::GetVendorById` — Linux returns NULL (whole body Windows-only).
  Needs `readlink("/proc/self/exe")` **plus** a portable path separator and a
  portable `PCI_ID_DATABASE_PATH` (`pci-id.h:44` is `"constants\\pci.ids"`).
  Until then `!pcitree` / `!pcicam` print no vendor/device names on Linux.

### PE parsing
- [ ] Recreate Windows `IMAGE_*` headers for Linux and port `pe-parser.cpp`
  (replace `pe-parser-linux.cpp`). Affects `!pe`; `!hide` currently gets `0` for
  all syscall numbers (a Windows-guest feature, meaningless on a Linux host).

### Transport
- [ ] `platform-serial.c` Linux branch — implement termios serial I/O (currently stub).
- [ ] `platform-ioctl.c` Linux branch — needs a Linux kernel module + real ioctl
  (currently stub). This is the local driver interface used across many files.

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
- [ ] `common.cpp::ListDirectory` — Linux returns empty vector; reimplement with
  `opendir`/`readdir` + `fnmatch` (only caller today is eval.cpp's test harness).
- [ ] `common.cpp::GetConfigFilePath` — Linux empties the path; resolve via
  `readlink("/proc/self/exe")` + append `CONFIG_FILE_NAME` (also blocked on wide-char).
- [ ] `common.cpp::SetPrivilege` / `IsFileExistW` — Linux stubs (no callers /
  wide-char deferred respectively).
- [x] `cpu.cpp:148` & `cpu.cpp:200` — same `CpuIdEx`→`CpuCpuIdEx` typo fix as
  common.cpp. Done.

### Build system
- [ ] Add `.gitignore` rules for the in-source CMake build output
  (`CMakeCache.txt`, `CMakeFiles/`, generated `Makefile`, `cmake_install.cmake`,
  `*.o`, `*.so`) — or switch to an out-of-source `build/` directory.

---

## Building

```bash
cmake .   # re-run only when CMake files change
make      # build; find the next file that fails, port it, repeat
```
