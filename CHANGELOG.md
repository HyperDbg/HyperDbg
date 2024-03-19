# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
