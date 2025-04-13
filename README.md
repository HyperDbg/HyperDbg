<p align="left">
<a href="https://hyperdbg.org"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Website-orange.svg" alt="Website"></a>
<a href="https://docs.hyperdbg.org"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Docs-yellow.svg" alt="Documentation"></a>
<a href="https://doxygen.hyperdbg.org"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Doxygen-lightgreen.svg" alt="Doxygen"></a>
<a href="https://research.hyperdbg.org"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Research-pink.svg" alt="Published Academic Researches"></a>
<a href="https://www.gnu.org/licenses/gpl-3.0"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/License-GPLv3-blue.svg" alt="License"></a>
</p>


# HyperDbg Debugger

<a href="https://hyperdbg.org/"><img align="right" width="150" height="150" src="https://github.com/HyperDbg/graphics/raw/master/Art%20Board/HyperDbg-Cat.Circle.Compressed.png" alt="HyperDbg Debugger"></a></br>
HyperDbg Debugger is an open-source, community-driven, hypervisor-assisted, user-mode, and kernel-mode Windows debugger with a focus on using modern hardware technologies. It is a debugger designed for analyzing, fuzzing, and reversing.

You can follow **HyperDbg** on **[Twitter](https://twitter.com/HyperDbg)** or **[Mastodon](https://infosec.exchange/@hyperdbg)** to get notified about new releases, or join any of the HyperDbg groups, where you can ask developers and open-source reversing enthusiasts for help setting up and using HyperDbg.

- **[Telegram](https://t.me/HyperDbg)**
- **[Discord](https://discord.gg/anSPsGUtzN)**
- **[Matrix](https://matrix.to/#/#hyperdbg-discussion:matrix.org)**

## Description

**HyperDbg** is designed with a focus on using modern hardware technologies to provide new features to the debuggers' world. It operates on top of Windows by virtualizing an already running system using Intel VT-x and EPT. This debugger aims not to use any APIs and software debugging mechanisms, but instead, it uses Second Layer Page Table (a.k.a. Extended Page Table or EPT) extensively to monitor both kernel and user executions.
<p align="center"><a href="https://hyperdbg.org/"><img align="center" width="600" height="500" src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Art%20Board/Artboard%201.png" alt="HyperDbg Debugger"></a></br>
</p>
HyperDbg comes with features like hidden hooks, which are as fast as old inline hooks, but also stealth. It mimics hardware debug registers for (read & write) to a specific location, but this time entirely invisible for both Windows kernel and the programs, and of course, without any limitation in size or count!

Using TLB-splitting, and having features such as measuring code coverage and monitoring all mov(s) to/from memory by a function, makes HyperDbg a unique debugger.

Although it has novel features, HyperDbg tries to be as stealthy as possible. It doesn’t use any debugging APIs to debug Windows or any application, so classic anti-debugging methods won’t detect it. Also, it resists the exploitation of time delta methods (e.g., RDTSC/RDTSCP) to detect the presence of hypervisors, therefore making it much harder for applications, packers, protectors, malware, anti-cheat engines, etc. to discover the debugger.

## Build & Installation

You can download the latest compiled binary files from **[releases](https://github.com/HyperDbg/HyperDbg/releases)**; otherwise, if you want to build HyperDbg, you should clone HyperDbg with the `--recursive` flag.
```
git clone --recursive https://github.com/HyperDbg/HyperDbg.git
```
Please visit **[Build & Install](https://docs.hyperdbg.org/getting-started/build-and-install)** and **[Quick Start](https://docs.hyperdbg.org/getting-started/quick-start)** for a detailed explanation of how to start with **HyperDbg**. You can also see the **[FAQ](https://docs.hyperdbg.org/getting-started/faq)** for more information, or if you previously used other native debuggers like GDB, LLDB, or WinDbg, you could see the [command map](https://hyperdbg.github.io/commands-map).

## Tutorials

The **OpenSecurityTraining2's "Reversing with HyperDbg (Dbg3301)**" tutorial series, available on [**OST2's website**](https://ost2.fyi/Dbg3301) (_preferred_) and [**YouTube**](https://www.youtube.com/playlist?list=PLUFkSN0XLZ-kF1f143wlw8ujlH2A45nZY) is the recommended way to get started with and learn HyperDbg. It guides you through the initial steps of using HyperDbg, covering essential concepts, principles, and debugging functionalities, along with practical examples and numerous reverse engineering methods that are unique to HyperDbg.

If you're interested in understanding the internal design and architecture of hypervisors and HyperDbg, you can read the [**Hypervisor From Scratch**](https://rayanfam.com/tutorials) tutorials.

## Publications

In case you use one of **HyperDbg**'s components in your work, please consider citing our papers.

**1. [HyperDbg: Reinventing Hardware-Assisted Debugging (CCS'22)](https://dl.acm.org/doi/abs/10.1145/3548606.3560649)** [[arXiv](https://arxiv.org/abs/2207.05676)]

```
@inproceedings{karvandi2022hyperdbg,
  title={HyperDbg: Reinventing Hardware-Assisted Debugging},
  author={Karvandi, Mohammad Sina and Gholamrezaei, MohammadHosein and Khalaj Monfared, Saleh and Meghdadizanjani, Soroush and Abbassi, Behrooz and Amini, Ali and Mortazavi, Reza and Gorgin, Saeid and Rahmati, Dara and Schwarz, Michael},
  booktitle={Proceedings of the 2022 ACM SIGSAC Conference on Computer and Communications Security},
  pages={1709--1723},
  year={2022}
}
```
<details>

<summary><b>Other paper built upon HyperDbg...</b></summary>


**2. [The Reversing Machine: Reconstructing Memory Assumptions](https://arxiv.org/pdf/2405.00298)** [[arXiv](https://arxiv.org/abs/2405.00298)]

```
@article{karvandi2024reversing,
  title={The Reversing Machine: Reconstructing Memory Assumptions},
  author={Karvandi, Mohammad Sina and Meghdadizanjani, Soroush and Arasteh, Sima and Monfared, Saleh Khalaj and Fallah, Mohammad K and Gorgin, Saeid and Lee, Jeong-A and van der Kouwe, Erik},
  journal={arXiv preprint arXiv:2405.00298},
  year={2024}
}
```
</details>

You can also read [this article](https://research.hyperdbg.org/debugger/kernel-debugger-design/) as it describes the overall architecture, technical difficulties, design decisions, and internals of HyperDbg Debugger, [this article](https://research.hyperdbg.org/vmm/transparency/) about our efforts on vm-exit transparency, [this article](https://research.hyperdbg.org/debugger/chasing-bugs/) about chasing bugs within hypervisors, and [this article](https://research.hyperdbg.org/debugger/gaining-insights/) about new reverse engineering techniques introduced in HyperDbg. More articles, posts, and resources are available at the **[awesome](https://github.com/HyperDbg/awesome)** repo, and in addition, the **[slides](https://github.com/HyperDbg/slides)** repo provides presentation slides for further reference.

## Unique Features
* Advanced Hypervisor-based Kernel Mode Debugger [<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/getting-started/attach-to-hyperdbg/debug" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/getting-started/attach-to-hyperdbg/local-debugging" target="_blank">link</a>]
* Classic EPT Hook (Hidden Breakpoint) [<a href="https://docs.hyperdbg.org/commands/extension-commands/epthook" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/design/features/vmm-module/design-of-epthook" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/hooking-any-function" target="_blank">link</a>]
* Inline EPT Hook (Inline Hook) [<a href="https://docs.hyperdbg.org/commands/extension-commands/epthook2" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/design/features/vmm-module/design-of-epthook2" target="_blank">link</a>]
* Monitor Memory For R/W (Emulating Hardware Debug Registers Without Limitation) [<a href="https://docs.hyperdbg.org/commands/extension-commands/monitor" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/design/features/vmm-module/design-of-monitor" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/monitoring-accesses-to-structures" target="_blank">link</a>]
* SYSCALL Hook (Disable EFER & Handle #UD) [<a href="https://docs.hyperdbg.org/commands/extension-commands/syscall" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/design/features/vmm-module/design-of-syscall-and-sysret" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/intercepting-all-syscalls" target="_blank">link</a>]
* SYSRET Hook (Disable EFER & Handle #UD) [<a href="https://docs.hyperdbg.org/commands/extension-commands/sysret" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/design/features/vmm-module/design-of-syscall-and-sysret" target="_blank">link</a>]
* CPUID Hook & Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/cpuid" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/triggering-special-instructions" target="_blank">link</a>]
* RDMSR Hook & Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/msrread" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/identifying-system-behavior" target="_blank">link</a>]
* WRMSR Hook & Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/msrwrite" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/identifying-system-behavior" target="_blank">link</a>]
* RDTSC/RDTSCP Hook & Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/tsc" target="_blank">link</a>]
* RDPMC Hook & Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/pmc" target="_blank">link</a>]
* VMCALL Hook & Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/vmcall" target="_blank">link</a>]
* Debug Registers Hook & Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/dr" target="_blank">link</a>]
* I/O Port (In Instruction) Hook & Monitor  [<a href="https://docs.hyperdbg.org/commands/extension-commands/ioin" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/triggering-special-instructions" target="_blank">link</a>]
* I/O Port (Out Instruction) Hook & Monitor  [<a href="https://docs.hyperdbg.org/commands/extension-commands/ioout" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/triggering-special-instructions" target="_blank">link</a>]
* MMIO Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/monitor" target="_blank">link</a>]
* Exception (IDT < 32) Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/exception" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/design/features/vmm-module/design-of-exception-and-interrupt" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/identifying-system-behavior" target="_blank">link</a>]
* External-Interrupt (IDT > 32) Monitor [<a href="https://docs.hyperdbg.org/commands/extension-commands/interrupt" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/design/features/vmm-module/design-of-exception-and-interrupt" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/events/identifying-system-behavior" target="_blank">link</a>]
* Running Automated Scripts [<a href="https://docs.hyperdbg.org/commands/scripting-language/debugger-script" target="_blank">link</a>]
* Transparent-mode (Anti-debugging and Anti-hypervisor Resistance) [<a href="https://docs.hyperdbg.org/tips-and-tricks/considerations/transparent-mode" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/extension-commands/hide" target="_blank">link</a>]
* Running Custom Assembly In Both VMX-root, VMX non-root (Kernel & User) [<a href="https://docs.hyperdbg.org/using-hyperdbg/prerequisites/how-to-create-an-action" target="_blank">link</a>]
* Checking For Custom Conditions [<a href="https://docs.hyperdbg.org/using-hyperdbg/prerequisites/how-to-create-a-condition" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/design/debugger-internals/conditions" target="_blank">link</a>]
* Process-specific & Thread-specific Debugging [<a href="https://docs.hyperdbg.org/commands/meta-commands/.process" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/meta-commands/.thread" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/user-mode-debugging/examples/basics/switching-to-a-specific-process-or-thread" target="_blank">link</a>]
* VMX-root Compatible Message Tracing [<a href="https://docs.hyperdbg.org/design/features/vmm-module/vmx-root-mode-compatible-message-tracing" target="_blank">link</a>]
* Powerful Kernel Side Scripting Engine [<a href="https://docs.hyperdbg.org/commands/scripting-language" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/design/script-engine" target="_blank">link</a>]
* Support To Symbols (Parsing PDB Files) [<a href="https://docs.hyperdbg.org/commands/meta-commands/.sympath" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/meta-commands/.sym" target="_blank">link</a>]
* Mapping Data To Symbols & Create Structures, Enums From PDB Files [<a href="https://docs.hyperdbg.org/commands/debugging-commands/dt" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/debugging-commands/struct" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/basics/mapping-data-and-create-structures-and-enums-from-symbols" target="_blank">link</a>]
* Event Forwarding (#DFIR) [<a href="https://docs.hyperdbg.org/tips-and-tricks/misc/event-forwarding" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/debugging-commands/output" target="_blank">link</a>]
* Transparent Breakpoint Handler [<a href="https://docs.hyperdbg.org/commands/debugging-commands/bp" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/using-hyperdbg/kernel-mode-debugging/examples/basics/setting-breakpoints-and-stepping-instructions" target="_blank">link</a>]
* Various Custom Scripts [<a href="https://github.com/HyperDbg/scripts" target="_blank">link</a>]
* HyperDbg Software Development Kit (SDK) [<a href="https://docs.hyperdbg.org/using-hyperdbg/sdk" target="_blank">link</a>]
* Event Short-circuiting [<a href="https://docs.hyperdbg.org/tips-and-tricks/misc/event-short-circuiting" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/scripting-language/functions/events/event_sc" target="_blank">link</a>]
* Tracking records of function calls and return addresses [<a href="https://docs.hyperdbg.org/commands/extension-commands/track" target="_blank">link</a>]
* Kernel-level Length Disassembler Engine (LDE) [<a href="https://docs.hyperdbg.org/commands/scripting-language/functions/diassembler/disassemble_len" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/scripting-language/functions/diassembler/disassemble_len32" target="_blank">link</a>]
* Memory Execution Monitor & Execution Blocking [<a href="https://docs.hyperdbg.org/commands/extension-commands/monitor" target="_blank">link</a>]
* Custom Page-fault Injection [<a href="https://docs.hyperdbg.org/commands/meta-commands/.pagein" target="_blank">link</a>]
* Different Event Calling Stages [<a href="https://docs.hyperdbg.org/tips-and-tricks/misc/event-calling-stage" target="_blank">link</a>]
* Injecting Custom Interrupts/Exceptions/Faults [<a href="https://docs.hyperdbg.org/commands/scripting-language/functions/events/event_inject" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/scripting-language/functions/events/event_inject_error_code" target="_blank">link</a>]
* Instant events in the Debugger Mode [<a href="https://docs.hyperdbg.org/tips-and-tricks/misc/instant-events" target="_blank">link</a>]
* Detect kernel-to-user and user-to-kernel transitions [<a href="https://docs.hyperdbg.org/commands/extension-commands/mode" target="_blank">link</a>]
* Physical memory monitoring hooks [<a href="https://docs.hyperdbg.org/commands/extension-commands/monitor" target="_blank">link</a>]
* Enumerating PCI/PCI-e Devices [<a href="https://docs.hyperdbg.org/commands/extension-commands/pcitree" target="_blank">link</a>]
* Interpreting and Dumping PCI/PCI-e Configuration Space (CAM) [<a href="https://docs.hyperdbg.org/commands/extension-commands/pcicam" target="_blank">link</a>]
* Dumping IDT entries, I/O APIC, and Local APIC in XAPIC and X2APIC modes [<a href="https://docs.hyperdbg.org/commands/extension-commands/idt" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/extension-commands/ioapic" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/extension-commands/apic" target="_blank">link</a>]

## How does it work?

You can read about the internal design of HyperDbg and its features in the [documentation](https://docs.hyperdbg.org/design). Here's a top-level diagram that shows how HyperDbg works:
</br>

<p align="center"><a href="https://hyperdbg.org/"><img align="center" width="70%" height="100%" src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Diagrams/Diagram-hq-v1/Diagram_v1.jpg" alt="HyperDbg Design"></a></br>
</p>
</br>

## Scripts
You can write your **[scripts](https://github.com/HyperDbg/scripts)** to automate your debugging journey. **HyperDbg** has a powerful, fast, and entirely kernel-side implemented [script engine](https://docs.hyperdbg.org/commands/scripting-language).

## Contributing
Contributing to HyperDbg is super appreciated. We have made a list of potential [tasks](https://github.com/HyperDbg/HyperDbg/blob/master/CONTRIBUTING.md#things-to-work-on) that you might be interested in contributing towards.

If you want to contribute to HyperDbg, please read the [Contribution Guide](https://github.com/HyperDbg/HyperDbg/blob/master/CONTRIBUTING.md).


## License
**HyperDbg**, and all its submodules and repos, unless a license is otherwise specified, are licensed under **GPLv3** LICENSE.

Dependencies are licensed by their own.
