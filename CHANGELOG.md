# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.6.0.0] - 2023-XX-XX
New release of the HyperDbg Debugger.

### Added
- **!crwrite** - Control Register Modification Event ([link](https://docs.hyperdbg.org/commands/extension-commands/crwrite))

## [0.5.0.0] - 2023-08-07
New release of the HyperDbg Debugger.

### Added
- The event calling stage mechanism ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/event-calling-stage))
- New pseudo-registers (**$stage**) in the script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#pseudo-registers))

### Changed
- The disassembler now warns if you mistakenly used the 'u' command over a 32-bit program ([link](https://github.com/HyperDbg/HyperDbg/commit/9d239ccdfd7901cad197a4b49327efbf322cd116))
- The debuggee won't load the VMM module if the debugger is not listening
- The debugger and the debuggee now perform a version/build check to prevent version mismatch 
- fix the 'eb' command's parsing issue with '0xeb' hex bytes ([link](https://github.com/HyperDbg/HyperDbg/commit/b7dc237d7fd72b6f0130f86eb3b30f9f490917d6))
- Fix the connection problem with serial (checksum error) over two VMs 
- fix the 't' command's indicator of trap flags and simulatenous stepping of multiple threads ([link](https://github.com/HyperDbg/HyperDbg/pull/249))
- fix the problem with the '.kill' and '.restart' commands

## [0.4.0.0] - 2023-07-18
New release of the HyperDbg Debugger.

### Added
- The **!monitor** command now supports 'execution' interception ([link](https://docs.hyperdbg.org/commands/extension-commands/monitor))
- **.pagein** - command is added to the debugger to bring pages in ([link](https://docs.hyperdbg.org/commands/meta-commands/.pagein))

### Changed
- The '.start' command's mechanism for finding the entrypoint is changed to address issues ([link](https://docs.hyperdbg.org/commands/meta-commands/.start))
- The buffer overlap error in hyperlog in multi-core systems is fixed ([link](https://github.com/HyperDbg/HyperDbg/commit/1fa06c0b5a8b93656803fdc455025f59aadd0adb))
- The implementation of 'dd' (define dwrod, 32-bit), and 'dw' (define word, 16-bit) is changed ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#keywords))
- The problem with unloading driver (#238) is fixed ([link](https://github.com/HyperDbg/HyperDbg/issues/238))
- The symbol files for 32-bit modules are now loaded based on SysWOW64, and the issue (#243) is fixed ([link](https://github.com/HyperDbg/HyperDbg/issues/243))
- New alias names for u, !u as u64, !u64 and for u2, !u2 as u32, !u32 ([link](https://docs.hyperdbg.org/commands/extension-commands/u))([link](https://docs.hyperdbg.org/commands/debugging-commands/u))

## [0.3.0.0] - 2023-06-08
New release of the HyperDbg Debugger.

### Added
- The event short-circuiting mechanism ([link](https://docs.hyperdbg.org/tips-and-tricks/misc/event-short-circuiting))
- New pseudo-registers (**$tag**, **$id**) in the script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/assumptions-and-evaluations#pseudo-registers))
- The breakpoint interception manipulation option is added to the 'test' command ([link](https://docs.hyperdbg.org/commands/debugging-commands/test))
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
- The parameters of !cpuid extension command is changed, and a new EAX index parameter is added ([link](https://docs.hyperdbg.org/commands/extension-commands/cpuid#parameters))
- The problem with removing EPT hooks (!monitor and !epthook) is fixed ([link](https://github.com/HyperDbg/HyperDbg/commit/e2ea08ac35834ff869512c3c450004bc50a06390))

## [0.2.0.0] - 2023-05-03
The second (2nd) release of the HyperDbg Debugger.

### Added
- HyperDbg Software Development Kit (SDK) is now available 
- **flush()** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/events/flush))
- **memcpy()** function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/memory/memcpy))

### Changed
- Global code refactor and fixing bugs!
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
