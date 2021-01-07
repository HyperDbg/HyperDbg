# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]
Unreleased description.

### Added

### Changed

### Removed


## [0.1.0.0] - 2021-03-31
This is the first release of HyperDbg Debugger.

### Added
- !pte
- lm
- .connect
- .disconnect
- wrmsr, rdmsr
- db dd dc dq
- !db !dd !dc !dq
- load 
- unload
- cpu
- exit
- .formats
- .cls
- u !u
- u2 !u2
- .script
- !hide
- !unhide
- !epthook2
- !epthook
- !syscall
- !sysret
- !monitor
- !cpuid
- !msrread
- !msrwrite
- !tsc
- !pmc
- !exception
- !interrupt
- !dr
- !ioin
- !ioout
- !vmcall
- !epthook
- .logopen .logclose
- !va2pa
- !pa2va
- pause
- g
- sleep
- eb !eb and all it's variants
- !measure
- .help : show the commands help function
- s  !s (search memory)
- event
- status .status : shows the state e.g. connected to a remote system
- bp bl bd bc (breakpoint clear)
- listen
- .debug
- g
- test  (test every functionalities)
- .formats should also support register values
- print
- .process /p /i (.process)
- r & rrcx=*
- t 
- p
