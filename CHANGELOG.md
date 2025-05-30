# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.13.2.0] - 2025-05-26
New release of the HyperDbg Debugger.

### Added
- Intercepting system-call return results using the TRAP flag for the transparent-mode
- Added optional parameters and context for the transparent-mode system-call return interceptions

### Changed
- Set variable length (stack frames) for showing the callstack ([link](https://docs.hyperdbg.org/commands/debugging-commands/k))
- Fixed VMCS layout corruption due to NMI injection (VMRESUME 0x7 error) in nested-virtualization on Meteor Lake processors
- Restore RDMSR handler for VM-exits

## [0.13.1.0] - 2025-04-14
New release of the HyperDbg Debugger.

### Added
- Added new transparency methods for hiding nested virtualization environments thanks to [@CokeTree3](https://github.com/CokeTree3) ([link](https://github.com/HyperDbg/HyperDbg/pull/515))

### Changed
- Fix '.thread' command crash ([link](https://github.com/HyperDbg/HyperDbg/pull/510))
- Update .clang-format format file based on the new version of LLVM
- Update the list of required contributions

## [0.13.0.0] - 2025-02-25
New release of the HyperDbg Debugger.

### Added
- Added mitigation for the anti-hypervisor method in handling the trap flag for emulated instructions ([link](https://github.com/HyperDbg/HyperDbg/pull/497))
- Export the SDK functions for enabling and disabling transparent mode ([link](https://docs.hyperdbg.org/commands/extension-commands/hide#sdk))([link](https://docs.hyperdbg.org/commands/extension-commands/unhide#sdk))
- New description of changing script engine constants ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/customize-build/change-script-engine-limitations))
- Added the command for interpreting PCI CAM (PCI configuration space) fields ([link](https://docs.hyperdbg.org/commands/extension-commands/pcicam))
- Added the command for dumping PCI CAM (PCI configuration space) memory ([link](https://docs.hyperdbg.org/commands/extension-commands/pcicam))
- Checking for and unloading the older version of the driver (if it exists) ([link](https://github.com/HyperDbg/HyperDbg/pull/503))
- **memcpy_pa()** function in the script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/memory/memcpy_pa))
- **poi_pa**, **hi_pa**, **low_pa**, **db_pa**, **dd_pa**, **dw_pa**, and **dq_pa** keywords in the script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#keywords))
- **eb_pa**, **ed_pa**, and **eq_pa** functions in the script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/memory/eb_pa-ed_pa-eq_pa))

### Changed
- Fix the 'lm' command issue of not showing kernel module addresses (KASLR leak mitigation) introduced in Windows 11 24h2 ([link](https://docs.hyperdbg.org/commands/debugging-commands/lm))
- Deprecated TSC mitigation for the transparent mode ([link](https://docs.hyperdbg.org/commands/extension-commands/measure))
- Changed the parameters of the '!hide' command ([link](https://docs.hyperdbg.org/commands/extension-commands/hide))
- Changed the parameters of the '!unhide' command ([link](https://docs.hyperdbg.org/commands/extension-commands/unhide))
- Fix containing backslash escape character in script strings ([link](https://github.com/HyperDbg/HyperDbg/pull/499))
- Fix reading/writing into devices' physical memory (MMIO region) in VMI Mode ([link](https://github.com/HyperDbg/HyperDbg/pull/500))
- All test cases for command parsing are now passed ([link](https://github.com/HyperDbg/HyperDbg/pull/504))
- The '.sympath' command now requires the symbol server path to be within quotes, although it is not mandatory ([link](https://docs.hyperdbg.org/commands/meta-commands/.sympath))

## [0.12.0.0] - 2025-01-02
New release of the HyperDbg Debugger.

### Added
- Added the PCI tree command ([link](https://docs.hyperdbg.org/commands/extension-commands/pcitree))
- Added the proper handling for the xsetbv VM exits thanks to [@Shtan7](https://github.com/Shtan7) ([link](https://github.com/HyperDbg/HyperDbg/pull/491))
- Added the IDT command for interpreting Interrupt Descriptor Table (IDT) ([link](https://docs.hyperdbg.org/commands/extension-commands/idt))
- Export SDK APIs for getting Interrupt Descriptor Table (IDT) entries

### Changed
- Fix buffer overflow in the symbols path converter thanks to [@binophism](https://github.com/binophism) ([link](https://github.com/HyperDbg/HyperDbg/pull/490))
- Fix script engine's "printf" function to improve safety thanks to [@Reodus](https://github.com/Reodus) ([link](https://github.com/HyperDbg/HyperDbg/pull/489))

## [0.11.0.0] - 2024-12-03
New release of the HyperDbg Debugger.

### Added
- Added the local APIC command (xAPIC and x2APIC modes) ([link](https://docs.hyperdbg.org/commands/extension-commands/apic))
- Added the I/O APIC command ([link](https://docs.hyperdbg.org/commands/extension-commands/ioapic))
- The new link is added to help increase the number of EPT hook breakpoints in a single page ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/customize-build/number-of-ept-hooks-in-one-page))
- Export SDK APIs for Local APIC and X2APIC

### Changed
- The link for changing the communication buffer size is updated ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/customize-build/increase-communication-buffer-size))
- Update Microsoft's DIA SDK and symsrv

## [0.10.2.0] - 2024-10-11
New release of the HyperDbg Debugger.

### Added
- Automated test case parsing and test case compilation (generation) for the hwdbg debugger
- Export hwdbg testing functions
- Automated test case interpretation and emulation of hwdbg hardware scripts
- Create JSON representation of hwdbg configs

### Changed
- Fix main command parser bugs according to test cases
- Improvements in symbol structure, token structure, and stack buffer in the script engine
- Fix compatibility mode program crash when terminating 32-bit process (#479) ([link](https://github.com/HyperDbg/HyperDbg/pull/479))
- Extensive refactor of chip instance info interpretation codes of hwdbg debugger
- Separating functions of hwdbg interpreter and script manager
- Fix synthesize inconsistencies between Icarus iVerilog and Xilinx ISim
- Fix runtime error for deallocating memory from separate DLLs
- Exporting standard functions (import/export) for the script engine
- Exporting standard functions (import/export) for the symbol parser
- Avoid passing signals once the stage is not configured

## [0.10.1.0] - 2024-09-08
New release of the HyperDbg Debugger.

### Added
- Added feature to pause the debuggee immediately upon connection
- The '.debug' command now supports pausing the debuggee at startup ([link](https://docs.hyperdbg.org/commands/meta-commands/.debug))
- Export SDK API for assembling instructions
- The 'struct' command now supports a path as output ([link](https://docs.hyperdbg.org/commands/debugging-commands/struct))
- Export SDK API closing connection to the remote debuggee
- Automated tests for the main command parser
- Export SDK APIs for stepping and tracing instructions
- Export SDK APIs for tracking execution

### Changed
- HyperDbg command-line comment sign is changed from '#' to C-like comments ('//' and '/**/')
- Integrating a new command parser for the regular HyperDbg commands
- Fix showing a list of active outputs using the 'output' command ([link](https://docs.hyperdbg.org/commands/debugging-commands/output))
- Fix the issue of passing arguments to the '.start' command ([link](https://docs.hyperdbg.org/commands/meta-commands/.start))
- Fix the problem with parsing multiple spaces within the events (#420) ([link](https://github.com/HyperDbg/HyperDbg/issues/420))
- Fix the problem with escaping '{' in the command parser (#421) ([link](https://github.com/HyperDbg/HyperDbg/issues/421))
- Fix nested brackets issues in the main command parser
- Fix script engine bugs on order of passing arguments to functions (#453) ([link](https://github.com/HyperDbg/HyperDbg/issues/453))
- Fix the script test case for factorial computation ([link](https://github.com/HyperDbg/script-engine-test/blob/main/semantic-test-cases/manual-test-cases_60-69.ds))
- Fix the script test case for computation iterative Fibonacci ([link](https://github.com/HyperDbg/script-engine-test/blob/main/semantic-test-cases/manual-test-cases_60-69.ds))
- Fix miscomputation of physical address width for physical address validity checks (#469) ([link](https://github.com/HyperDbg/HyperDbg/issues/469))

## [0.10.0.0] - 2024-07-22
New release of the HyperDbg Debugger.

### Added
- Support using assembly conditions and codes in all events ([link](https://docs.hyperdbg.org/using-hyperdbg/prerequisites/how-to-create-a-condition))([link](https://docs.hyperdbg.org/using-hyperdbg/prerequisites/how-to-create-an-action))
- Added support for forwarding events to binary (DLL) modules ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/event-forwarding))([link](https://docs.hyperdbg.org/commands/debugging-commands/output))([link](https://github.com/HyperDbg/event-forwarding-examples))
- Added the assembler command 'a' for virtual memory ([link](https://docs.hyperdbg.org/commands/debugging-commands/a))
- Added the assembler command '!a' for physical memory ([link](https://docs.hyperdbg.org/commands/extension-commands/a))
- Providing a unified SDK API for reading memory in the VMI Mode and the Debugger Mode
- Export SDK APIs for reading/writing into registers in the Debugger Mode
- Export SDK API for writing memory in the VMI Mode and the Debugger Mode
- Export SDK API for getting kernel base address
- Export SDK API for connecting to the debugger and from debuggee in the Debugger Mode
- Export SDK API for starting a new process
- Add and export SDK API for unsetting message callback
- Event commands are coming with more examples regarding scripts and assembly codes
- Add message callback using shared memory
- Add maximum execution limitation to the script IRs (#435) ([link](https://github.com/HyperDbg/HyperDbg/pull/435))

### Changed
- Fix clearing '!monitor' hooks on a different process or if the process is closed (#409) ([link](https://github.com/HyperDbg/HyperDbg/issues/409))
- Fix triggering multiple '!monitor' hooks with different contexts (#415) ([link](https://github.com/HyperDbg/HyperDbg/issues/415))
- Fix the problem of repeating commands once kHyperDbg is disconnected
- Fix step-over hangs if the process terminates/excepts within call instruction (#406) ([link](https://github.com/HyperDbg/HyperDbg/issues/406))
- Fix crash on editing invalid physical addresses (#424) ([link](https://github.com/HyperDbg/HyperDbg/issues/424))
- Fix exporting VMM module load and install it in the SDK
- Fix function interpretation issues and update the parser and the code execution (#435) ([link](https://github.com/HyperDbg/HyperDbg/pull/435))

## [0.9.1.0] - 2024-06-30
New release of the HyperDbg Debugger.

### Added
- Regular port/pin value read and modification in hwdbg
- Conditional statement evaluation in hwdbg
- Added automatic script buffer packet generator for hwdbg
- Added support for @hw_pinX and @hw_portX registers
- Added hwdbg instance information interpreter
- Added stack buffer in vmx-root ([link](https://github.com/HyperDbg/HyperDbg/commit/5df4d2a5268a6b190dc126bf6cfe0527703296eb))
- Exporting functions to support loading drivers with different names
- Exporting function to connect and load HyperDbg drivers
- Exporting function to connect and load HyperDbg drivers
- **$date** and **$time** pseudo-registers are added ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#pseudo-registers))([link](https://github.com/HyperDbg/HyperDbg/issues/397))

### Changed
- Fix using constant WSTRINGs in the **wcsncmp** function ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/strings/wcsncmp))
- Fix `phnt` build error with 24H2 SDK
- `hprdbgctrl.dll` changed to `libhyperdbg.dll`
- `hprdbgkd.sys` changed to `hyperkd.sys`
- `hprdbghv.dll` changed to `hyperhv.dll`
- Dividing user/kernel exported headers in the SDK

## [0.9.0.0] - 2024-06-09
New release of the HyperDbg Debugger.

### Added
- The **!monitor** command now physical address hooking ([link](https://docs.hyperdbg.org/commands/extension-commands/monitor))
- **hwdbg** is merged to HyperDbg codebase ([link](https://hwdbg.hyperdbg.org))
- **strncmp(Str1, Str2, Num)**, and **wcsncmp(WStr1, WStr2, Num)** functions in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/strings/strncmp))([link](https://docs.hyperdbg.org/commands/scripting-language/functions/strings/wcsncmp))

### Changed
- Using a separate HOST IDT in VMCS (not OS IDT) (fix to this [VM escape](https://www.unknowncheats.me/forum/c-and-c-/390593-vm-escape-via-nmi.html) issues)
- Using a dedicated HOST GDT and TSS Stack
- Checking for race-condition of not locked cores before applying instant-events and switching cores
- The error message for invalid address is changed ([more information](https://docs.hyperdbg.org/tips-and-tricks/considerations/accessing-invalid-address))
- Fix the problem of not locking all cores after running the '.pagein' command

## [0.8.4.0] - 2024-05-10
New release of the HyperDbg Debugger.

### Changed
- Fixed the signedness overflow of the command parser

## [0.8.3.0] - 2024-05-03
New release of the HyperDbg Debugger.

### Added
- Added hwdbg headers ([link](https://hwdbg.hyperdbg.org))
- Added support NUMA configuration with multiple count CPU sockets ([link](https://github.com/HyperDbg/HyperDbg/commit/040f70024fdad879ecf2ff2b31da066d4ed759fc))
- Added citation to TRM paper ([link](https://arxiv.org/abs/2405.00298))

### Changed
- Change release flag of hyperdbg-cli to Multi-threaded Debug (/MTd)
- Fix bitwise extended type, fixed memleaks, remove excess else and cmp int with EOF ([link](https://github.com/HyperDbg/HyperDbg/commit/7bcf1e5c71364aa3177eef0929c07463155ce5c6))

## [0.8.2.0] - 2024-03-19
New release of the HyperDbg Debugger.

### Added
- Add user-defined functions and variable types in script engine thanks to [@xmaple555](https://github.com/xmaple555) ([link](https://docs.hyperdbg.org/commands/scripting-language/constants-and-functions))([link](https://github.com/HyperDbg/HyperDbg/pull/342))

### Changed
- Fix debuggee crash after running the '.debug close' command on the debugger  
- The problem with adding edge MTRR pages is fixed thanks to [@Maladiy](https://github.com/Maladiy) ([link](https://github.com/HyperDbg/HyperDbg/pull/347)) 
- All compiler/linker warnings of kernel-mode modules are fixed  
- User/Kernel modules of HyperDbg now compiled with "treat warning as error"  
- After downloading new symbols it is automatically loaded
- Fix error messages/comments spelling typos

## [0.8.1.0] - 2024-02-01
New release of the HyperDbg Debugger.

### Added
- The **!monitor** command now supports length in parameters ([link](https://docs.hyperdbg.org/commands/extension-commands/monitor#syntax))

### Changed
- Fix the issue of not intercepting memory monitoring on non-contiguous physical memory allocations
- The speed of memory read/write/execution interception is enhanced by avoiding triggering out-of-range events

## [0.8.0.0] - 2024-01-28
New release of the HyperDbg Debugger thanks to [@mattiwatti](https://github.com/Mattiwatti).

### Added
- The **!mode** event command is added to detect kernel-to-user and user-to-kernel transitions ([link](https://docs.hyperdbg.org/commands/extension-commands/mode))
- The 'preactivate' command is added to support initializing special functionalities in the Debugger Mode ([link](https://docs.hyperdbg.org/commands/debugging-commands/preactivate))

### Changed
- Fix miscalculating MTRRs in 13th gen processors 

## [0.7.2.0] - 2024-01-23
New release of the HyperDbg Debugger thanks to [@mattiwatti](https://github.com/Mattiwatti) and [@cutecatsandvirtualmachines](https://github.com/cutecatsandvirtualmachines).

### Changed
- Fix INVEPT invalidation using out of scope descriptor ([link](https://github.com/HyperDbg/HyperDbg/commit/94b3a052a9b84bb7cf64114eba9fc2fe71d0eccf))
- Fix MTRR initialization crash ([link](https://github.com/HyperDbg/HyperDbg/pull/329))

## [0.7.1.0] - 2023-12-20
New release of the HyperDbg Debugger.

### Changed
- Fix the single core broadcasting events issue ([link](https://github.com/HyperDbg/HyperDbg/commit/ab95cd76285ef9aad084560c5c9dc8970bba84b7))
- Evaluate the '.pagin' ranges as expressions ([link](https://github.com/HyperDbg/HyperDbg/commit/ab95cd76285ef9aad084560c5c9dc8970bba84b7))
- Add hexadecimal escape sequence as string parameter for string functions ([link](https://github.com/HyperDbg/HyperDbg/commit/60fbec6936330643d8de1ec7b548f651ac8f106d))
- Add hexadecimal escape sequence as wstring parameter for wstring functions ([link](https://github.com/HyperDbg/HyperDbg/commit/e6dbc3f49e2d20a51d2f20120316fd0392067fa2))
- Fix breakpoint and the '!epthook' problems in the same address ([link](https://github.com/HyperDbg/HyperDbg/pull/326))

## [0.7.0.0] - 2023-11-22
New release of the HyperDbg Debugger.

### Added
- HyperDbg now applies events immediately as implemented in the "instant events" mechanism ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/instant-events))
- The Event Forwarding mechanism is now supported in the Debugger Mode ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/event-forwarding))
- The Event Forwarding mechanism now supports external modules (DLLs) ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/event-forwarding))
- **event_clear(EventId)** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/events/event_clear))
- HyperDbg now supports string inputs for strlen and other related functions thanks to [@xmaple555](https://github.com/xmaple555) ([link](https://github.com/HyperDbg/HyperDbg/pull/297))
- New semantic tests for the script engine (50 to 59) is added mainly for testing new string and memory comparison functions ([link](https://github.com/HyperDbg/script-engine-test))
- **strlen** and **wcslen** functions now support string and wide-character string as the input ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/strings/strlen))([link](https://docs.hyperdbg.org/commands/scripting-language/functions/strings/wcslen))
- **strcmp(Str1, Str2)**, **wcscmp(WStr1, WStr2)** and **memcmp(Ptr1, Ptr2, Num)** functions in script engine thanks to [@xmaple555](https://github.com/xmaple555) ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/strings/strcmp))([link](https://docs.hyperdbg.org/commands/scripting-language/functions/strings/wcscmp))([link](https://docs.hyperdbg.org/commands/scripting-language/functions/memory/memcmp))
- The debug break interception (\#DB) manipulation option is added to the 'test' command ([link](https://docs.hyperdbg.org/commands/debugging-commands/test))
- The '.pagein' command, now supports address ranges (length in bytes) to bring multiple pages into the RAM ([link](https://docs.hyperdbg.org/commands/meta-commands/.pagein))

### Changed
- Fix the problem with the "less than" and the "greater than" operators for signed numbers thanks to [@xmaple555](https://github.com/xmaple555) ([link](https://github.com/HyperDbg/HyperDbg/pull/279))
- Fix the problem checking for alternative names thanks to [@xmaple555](https://github.com/xmaple555) ([link](https://github.com/HyperDbg/HyperDbg/pull/276))
- Fix the crash by turning off the breakpoints while a breakpoint is still active thanks to [@xmaple555](https://github.com/xmaple555) ([link](https://github.com/HyperDbg/HyperDbg/pull/273))
- Fix the crash on reading symbols on remote debuggee thanks to [@xmaple555](https://github.com/xmaple555) ([link](https://github.com/HyperDbg/HyperDbg/pull/274))
- The 'prealloc' command is updated with new instant-event preallocated pools ([link](https://docs.hyperdbg.org/commands/debugging-commands/prealloc))
- Fix wrong removing of EPT Hook (hidden breakpoints)
- The 'event' command, no longer continues debuggee for clearing events, instead just disables the event and removes the effects of the event when debuggee continues ([link](https://docs.hyperdbg.org/commands/debugging-commands/events))
- **$id** pseudo-register changed to **$event_id** ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#pseudo-registers))
- **$tag** pseudo-register changed to **$event_tag** ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#pseudo-registers))
- **$stage** pseudo-register changed to **$event_stage** ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#pseudo-registers))
- Fix adding pseudo-registers with underscore in the script engine ([link](https://github.com/HyperDbg/HyperDbg/pull/313))
- Fix the boolean expression interpretation in **if** conditions in the script engine ([link](https://github.com/HyperDbg/HyperDbg/issues/311))
- HyperDbg now intercepts all debug breaks (\#DBs) if it's not explicitly asked not to by using the 'test' command ([link](https://docs.hyperdbg.org/commands/debugging-commands/test))
- Fix '%d' bug in script engine ([link](https://github.com/HyperDbg/HyperDbg/pull/318))

## [0.6.0.0-beta] - 2023-09-25
New release of the HyperDbg Debugger.

### Added
- **event_inject(InterruptionType, Vector)** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/events/event_inject))
- **event_inject_error_code(InterruptionType, Vector, ErrorCode)** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/events/event_inject_error_code))
- **.dump** - command is added to the debugger to dump the virtual memory into a file ([link](https://docs.hyperdbg.org/commands/meta-commands/.dump))
- **!dump** - command is added to the debugger to dump the physical memory into a file ([link](https://docs.hyperdbg.org/commands/extension-commands/dump))
- **gu** - command is added to the debugger to step-out or go up instructions thanks to [@xmaple555](https://github.com/xmaple555) ([link](https://docs.hyperdbg.org/commands/debugging-commands/gu))

### Changed
- HyperDbg now switched to a multiple EPTP memory model, and each core has its own EPT table ([link](https://github.com/HyperDbg/HyperDbg/commit/7f53fab2ee3ba5b6a48eac6ddeb5975398c4da31))
- Building mtrr map by adding smrr, fixed ranges, and default memory type is fixed (#255) thanks to [@Air14](https://github.com/Air14)
- The problem of removing multiple EPT hooks on a single address is fixed
- The problem of not intercepting the step-over command 'p' when executed in different cores is fixed
- HyperDbg now checks for the validity of physical addresses based on CPUID.80000008H:EAX\[7:0\]'s physical address width

## [0.5.0.0] - 2023-08-07
New release of the HyperDbg Debugger.

### Added
- The event calling stage mechanism ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/event-calling-stage))
- New pseudo-registers (**$stage**) in the script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#pseudo-registers))

### Changed
- The disassembler now warns if you mistakenly used the 'u' command over a 32-bit program ([link](https://github.com/HyperDbg/HyperDbg/commit/9d239ccdfd7901cad197a4b49327efbf322cd116))
- The debuggee won't load the VMM module if the debugger is not listening
- The debugger and the debuggee now perform a version/build check to prevent version mismatch 
- Fix the 'eb' command's parsing issue with '0xeb' hex bytes ([link](https://github.com/HyperDbg/HyperDbg/commit/b7dc237d7fd72b6f0130f86eb3b30f9f490917d6))
- Fix the connection problem with serial (checksum error) over two VMs 
- Fix the 't' command's indicator of trap flags and simultaneous stepping of multiple threads ([link](https://github.com/HyperDbg/HyperDbg/pull/249))
- Fix the problem with the '.kill' and '.restart' commands
- Show the stage of the event once the debugger is paused
- Fix sending context, tag, and registers once '!epthook2' wants to halt the debugger

## [0.4.0.0] - 2023-07-18
New release of the HyperDbg Debugger.

### Added
- The **!monitor** command now supports 'execution' interception ([link](https://docs.hyperdbg.org/commands/extension-commands/monitor))
- **.pagein** - command is added to the debugger to bring pages in ([link](https://docs.hyperdbg.org/commands/meta-commands/.pagein))

### Changed
- The '.start' command's mechanism for finding the entrypoint is changed to address issues ([link](https://docs.hyperdbg.org/commands/meta-commands/.start))
- The buffer overlap error in hyperlog in multi-core systems is fixed ([link](https://github.com/HyperDbg/HyperDbg/commit/1fa06c0b5a8b93656803fdc455025f59aadd0adb))
- The implementation of 'dd' (define dword, 32-bit), and 'dw' (define word, 16-bit) is changed ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#keywords))
- The problem with unloading driver (#238) is fixed ([link](https://github.com/HyperDbg/HyperDbg/issues/238))
- The symbol files for 32-bit modules are now loaded based on SysWOW64, and the issue (#243) is fixed ([link](https://github.com/HyperDbg/HyperDbg/issues/243))
- New alias names for u, !u as u64, !u64 and for u2, !u2 as u32, !u32 ([link](https://docs.hyperdbg.org/commands/extension-commands/u))([link](https://docs.hyperdbg.org/commands/debugging-commands/u))

## [0.3.0.0] - 2023-06-08
New release of the HyperDbg Debugger.

### Added
- The event short-circuiting mechanism ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/event-short-circuiting))
- New pseudo-registers (**$tag**, **$id**) in the script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#pseudo-registers))
- The breakpoint interception manipulation (\#BP) option is added to the 'test' command ([link](https://docs.hyperdbg.org/commands/debugging-commands/test))
- The '!track' command to create the tracking records of function CALLs and RETs along with registers ([link](https://docs.hyperdbg.org/commands/extension-commands/track))
- **disassemble_len(Address)** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/diassembler/disassemble_len))
- **disassemble_len32(Address)** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/diassembler/disassemble_len32))
- **event_sc(DisableOrEnable)** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/events/event_sc))

### Changed
- The old Length Disassembler Engine is replaced by Zydis ([link](https://github.com/HyperDbg/HyperDbg/pull/234))

## [0.2.2.0] - 2023-06-02
The patch for fixing bugs of HyperDbg Debugger.

### Changed
- The problem with the callstack command (k) is fixed ([link](https://github.com/HyperDbg/HyperDbg/commit/4194880a2e5578a4bb9055e2ac3e2fdb564e3d82))

## [0.2.1.0] - 2023-05-24
The patch for fixing bugs of the second (2nd) release of HyperDbg Debugger.

### Changed
- Fixing bugs!
- The parameters of the '!cpuid' extension command is changed, and a new EAX index parameter is added ([link](https://docs.hyperdbg.org/commands/extension-commands/cpuid#parameters))
- The problem with removing EPT hooks (!monitor and !epthook) is fixed ([link](https://github.com/HyperDbg/HyperDbg/commit/e2ea08ac35834ff869512c3c450004bc50a06390))

## [0.2.0.0] - 2023-05-03
The second (2nd) release of the HyperDbg Debugger.

### Added
- HyperDbg Software Development Kit (SDK) is now available 
- **flush()** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/events/flush))
- **memcpy()** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/memory/memcpy))

### Changed
- Global code refactoring and fixing bugs!
- Compiling HyperDbg by using the latest Windows 11 WDK
- **enable_event** function name changed to **event_enable** ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/events/event_enable))
- **disable_event** function name changed to **event_disable** ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/events/event_disable))
- The "**settings**" command now preserves the configurations in the config file
- The communication buffer is now separated from the hyperlogger buffer chunks and the buffer size is increased X10 times ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/increase-communication-buffer-size)) 
- Zydis submodule is updated to version 4 ([link](https://github.com/zyantific/zydis/releases/tag/v4.0.0)) 

### Removed
- **enable_event** script engine function
- **disable_event** script engine function

## [0.1.0.0] - 2022-05-31
This is the first (1st) release of HyperDbg Debugger.

### Added
- \# (comment in batch scripts)
- ? (evaluate and execute expressions and scripts in debuggee)
- ~ (display and change the current operating core)
- load (load the kernel modules)
- unload (unload the kernel modules)
- status (show the debuggee status)
- events (show and modify active/disabled events)
- p (step-over)
- t (step-in)
- i (instrumentation step-in)
- r (read or modify registers)
- bp (set breakpoint)
- bl (list breakpoints)
- be (enable breakpoints)
- bd (disable breakpoints)
- bc (clear and remove breakpoints)
- g (continue debuggee or processing kernel packets)
- x (examine symbols and find functions and variables address)
- db, dc, dd, dq (read virtual memory)
- eb, ed, eq (edit virtual memory)
- sb, sd, sq (search virtual memory)
- u, u2 (disassemble virtual address)
- k, kd, kq (display stack backtrace)
- dt (display and map virtual memory to structures)
- struct (make structures, enums, data types from symbols)
- sleep (wait for specific time in the .script command)
- pause (break to the debugger and pause processing kernel packets)
- print (evaluate and print expression in debuggee)
- lm (view loaded modules)
- cpu (check cpu supported technologies)
- rdmsr (read model-specific register)
- wrmsr (write model-specific register)
- flush (remove pending kernel buffers and messages)
- prealloc (reserve pre-allocated pools)
- output (create output source for event forwarding)
- test (test functionalities)
- settings (configures different options and preferences)
- exit (exit from the debugger)
- .help (show the help of commands)
- .debug (prepare and connect to debugger)
- .connect (connect to a session)
- .disconnect (disconnect from a session)
- .listen (listen on a port and wait for the debugger to connect)
- .status (show the debugger status)
- .start (start a new process)
- .restart (restart the process)
- .attach (attach to a process)
- .detach (detach from the process)
- .switch (show the list and switch between active debugging processes)
- .kill (terminate the process)
- .process, .process2 (show the current process and switch to another process)
- .thread, .thread2 (show the current thread and switch to another thread)
- .formats (show number formats)
- .script (run batch script commands)
- .sympath (set the symbol server)
- .sym (load pdb symbols)
- .pe (parse PE file)
- .logopen (open log file)
- .logclose (close log file)
- .cls (clear the screen)
- !pte (display page-level address and entries)
- !db, !dc, !dd, !dq (read physical memory)
- !eb, !ed, !eq (edit physical memory)
- !sb, !sd, !sq (search physical memory)
- !u, !u2 (disassemble physical address)
- !dt (display and map physical memory to structures)
- !epthook (hidden hook with EPT - stealth breakpoints)
- !epthook2 (hidden hook with EPT - detours)
- !monitor (monitor read/write to a page)
- !syscall, !syscall2 (hook system-calls)
- !sysret, !sysret2 (hook SYSRET instruction execution)
- !cpuid (hook CPUID instruction execution)
- !msrread (hook RDMSR instruction execution)
- !msrwrite (hook WRMSR instruction execution)
- !tsc (hook RDTSC/RDTSCP instruction execution)
- !pmc (hook RDPMC instruction execution)
- !vmcall (hook hypercalls)
- !exception (hook first 32 entries of IDT)
- !interrupt (hook external device interrupts)
- !dr (hook access to debug registers)
- !ioin (hook IN instruction execution)
- !ioout (hook OUT instruction execution)
- !hide (enable transparent-mode)
- !unhide (disable transparent-mode)
- !measure (measuring and providing details for transparent-mode)
- !va2pa (convert a virtual address to physical address)
- !pa2va (convert physical address to virtual address)
