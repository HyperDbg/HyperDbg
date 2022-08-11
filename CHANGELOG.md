# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0.0] - 2021-xx-xx
The second 2nd of the HyperDbg Debugger.

### Added
- HyperDbg SDK is available!
- fush function in script engine ([link](https://docs.hyperdbg.org/commands/scripting-language/functions/events/flush))
- !crwrite - Control Register Modification Event ([link](https://docs.hyperdbg.org/commands/extension-commands/crwrite))

### Changed
- Compiling HyperDbg by using the latest Windows 11 WDK
- enable_event function name changed to event_enable
- disable_event function name changed to event_disable

### Removed


## [0.1.0.0] - 2021-05-31
This is the 1st release of HyperDbg Debugger.

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
