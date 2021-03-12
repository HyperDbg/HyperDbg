<p align="left">
<a href="https://hyperdbg.com"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Website-orange.svg" alt="Website"></a>
<a href="https://docs.hyperdbg.com"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Docs-yellow.svg" alt="Documentation"></a>
<a href="https://doxygen.hyperdbg.com"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Doxygen-lightgreen.svg" alt="Doxygen"></a>
<a href="https://www.gnu.org/licenses/gpl-3.0"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/License-GPLv3-blue.svg" alt="License"></a>
<a href="https://twitter.com/HyperDbg"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-HyperDbg-Twitter-skyblue.svg" alt="Twitter"></a>
</p>

<a href="https://hyperdbg.com/"><img align="right" width="150" height="150" src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Art%20Board/HyperDbg-Cat.Circle.jpg" alt="HyperDbg Debugger"></a></br>

# HyperDbg Debugger 

### (HyperDbg is NOT in a WORKING STATE - YOU SHOULD NOT USE IT, but you can observe codes, please wait for first release in late April 2021)
#### We planned for first-release in late April 2021

**HyperDbg** debugger is an open-source, hypervisor-assisted user-mode, and kernel-mode Windows debugger with a focus on using modern hardware technologies. It is a debugger designed for analyzing, fuzzing and reversing.

Follow **HyperDbg** on **Twitter** to get notified about new releases ! 
<p align="center">(https://twitter.com/HyperDbg)</p>

## Description

HyperDbg is designed with a focus on using modern hardware technologies to provide new features to the reverse engineering world. It operates on top of Windows by virtualizing an already running system using Intel VT-x and Intel PT. This debugger aims not to use any APIs and software debugging mechanisms, but instead, it uses Second Layer Page Table (a.k.a. Extended Page Table or EPT) extensively to monitor both kernel and user executions.
<p align="center"><a href="https://hyperdbg.com/"><img align="center" width="600" height="500" src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Art%20Board/Artboard%201.png" alt="HyperDbg Debugger"></a></br>
</p>
HyperDbg comes with features like hidden hooks, which is as fast as old inline hooks, but also stealth. It mimics hardware debug registers for (read & write) to a specific location, but this time entirely invisible for both Windows kernel and the programs, and of course without any limitation in size or count!

Using TLB-splitting, and having features such as measuring code coverage and monitoring all mov(s) to/from memory by a function, makes HyperDbg a unique debugger.

Although it has novel features, HyperDbg tries to be as stealth as possible. It doesn’t use any debugging APIs to debug Windows or any application, so classic anti-debugging methods won’t detect it. Also, it resists the exploitation of time delta methods (e.g., RDTSC/RDTSCP) to detect the presence of hypervisors, therefore making it much harder for applications, packers, protectors, malware, anti-cheat engines, etc. to discover the debugger.

## Unique Features
### First Release (v0.1.0.0)
* Classic EPT Hook (Hidden Breakpoint) [<a href="https://docs.hyperdbg.com/commands/extension-commands/epthook" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/design/features/vmm-module/design-of-epthook" target="_blank">link</a>]
* Inline EPT Hook (Inline Hook) [<a href="https://docs.hyperdbg.com/commands/extension-commands/epthook2" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/design/features/vmm-module/design-of-epthook2" target="_blank">link</a>]
* Monitor Memory For R/W (Emulating Hardware Debug Registers Without Limitation) [<a href="https://docs.hyperdbg.com/commands/extension-commands/monitor" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/design/features/vmm-module/design-of-monitor" target="_blank">link</a>]
* SYSCALL Hook (Disable EFER & Handle #UD) [<a href="https://docs.hyperdbg.com/commands/extension-commands/syscall" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/design/features/vmm-module/design-of-syscall-and-sysret" target="_blank">link</a>]
* SYSRET Hook (Disable EFER & Handle #UD) [<a href="https://docs.hyperdbg.com/commands/extension-commands/sysret" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/design/features/vmm-module/design-of-syscall-and-sysret" target="_blank">link</a>]
* CPUID Hook & Monitor [<a href="https://docs.hyperdbg.com/commands/extension-commands/cpuid" target="_blank">link</a>]
* RDMSR Hook & Monitor [<a href="https://docs.hyperdbg.com/commands/extension-commands/msrread" target="_blank">link</a>]
* WRMSR Hook & Monitor [<a href="https://docs.hyperdbg.com/commands/extension-commands/msrwrite" target="_blank">link</a>]
* RDTSC/RDTSCP Hook & Monitor [<a href="https://docs.hyperdbg.com/commands/extension-commands/tsc" target="_blank">link</a>]
* RDPMC Hook & Monitor [<a href="https://docs.hyperdbg.com/commands/extension-commands/pmc" target="_blank">link</a>]
* VMCALL Hook & Monitor [<a href="https://docs.hyperdbg.com/commands/extension-commands/vmcall" target="_blank">link</a>]
* Debug Registers Hook & Monitor [<a href="https://docs.hyperdbg.com/commands/extension-commands/dr" target="_blank">link</a>]
* I/O Port (In Instruction) Hook & Monitor  [<a href="https://docs.hyperdbg.com/commands/extension-commands/ioin" target="_blank">link</a>]
* I/O Port (Out Instruction) Hook & Monitor  [<a href="https://docs.hyperdbg.com/commands/extension-commands/ioout" target="_blank">link</a>]
* MMIO Monitor 
* Exception (IDT < 32) Monitor [<a href="https://docs.hyperdbg.com/commands/extension-commands/exception" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/design/features/vmm-module/design-of-exception-and-interrupt" target="_blank">link</a>]
* External-Interrupt (IDT > 32) Monitor [<a href="https://docs.hyperdbg.com/commands/extension-commands/interrupt" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/design/features/vmm-module/design-of-exception-and-interrupt" target="_blank">link</a>]
* Running Automated Scripts [<a href="https://docs.hyperdbg.com/using-hyperdbg/examples/running-hyperdbg-script" target="_blank">link</a>]
* Transparent-mode (Anti-debugging and Anti-hypervisor Resistance) [<a href="https://docs.hyperdbg.com/tips-and-tricks/considerations/transparent-mode" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/using-hyperdbg/examples/defeating-anti-debug-and-anti-hypervisor-methods" target="_blank">link</a>]
* Running Custom Assembly In Both VMX-root, VMX non-root (Kernel & User) [<a href="https://docs.hyperdbg.com/using-hyperdbg/prerequisites/how-to-create-an-action" target="_blank">link</a>]
* Checking For Custom Conditions [<a href="https://docs.hyperdbg.com/using-hyperdbg/prerequisites/how-to-create-a-condition" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/design/debugger-internals/conditions" target="_blank">link</a>]
* VMX-root Compatible Message Tracing [<a href="https://docs.hyperdbg.com/design/features/vmm-module/vmx-root-mode-compatible-message-tracing" target="_blank">link</a>]
* Powerful Kernel Side Scripting Engine [<a href="https://docs.hyperdbg.com/commands/scripting-language" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/design/script-engine" target="_blank">link</a>]
* Event Forwarding (#DFIR) [<a href="https://docs.hyperdbg.com/tips-and-tricks/misc/event-forwarding" target="_blank">link</a>][<a href="https://docs.hyperdbg.com/commands/debugging-commands/output" target="_blank">link</a>]
* Transparent Breakpoint Handler
* Various Custom Scripts [<a href="https://github.com/HyperDbg/scripts" target="_blank">link</a>]

### Second Release (v0.2.0.0)
(not released yet !)

## Build & Installation
If you want to build HyperDbg, you should clone HyperDbg with `--recursive` flag.
```
git clone --recursive https://github.com/HyperDbg/HyperDbg.git
```
Please visit <a href="https://docs.hyperdbg.com/getting-started/build-and-install">Build & Install</a> and <a href="https://docs.hyperdbg.com/getting-started/quick-start">Quick Start</a> for a detailed explanation of how to start with **HyperDbg**. You can also see <a href="https://docs.hyperdbg.com/getting-started/faq">FAQ</a> for more information.

## How does it work?

We explained about how HyperDbg internally works and how we designed its features in details, take a look at : </br>
<p align="center">(https://docs.hyperdbg.com/design)</p>

Here's a diagram that shows how HyperDbg works !
</br>

<p align="center"><a href="https://hyperdbg.com/"><img align="center" width="70%" height="100%" src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Diagrams/Diagram-hq-v1/Diagram_v1.jpg" alt="HyperDbg Design"></a></br>
</p>
</br>


## Plugins
The plugin framework is not ready for the current version of HyperDbg. Future versions will support plugins.

## Donations to charity
We spent thousands of hours on HyperDbg and it's free and open-source for you, If you want to help to develop HyperDbg, please donate to children in Africa and send a picture of your donation to us, this makes all HyperDbg developers, super happy! Don't hesitate to send us the pictures, this way we know that we're doing something useful.
<p align="center">(https://www.compassion.com/donate/donate-to-children-in-africa.htm)</p>

## Credits
- Sina Karvandi (<a href="https://twitter.com/Intel80x86">@Intel80x86</a>)
- MH Gholamrezaei (<a href="https://twitter.com/mohoseinam">@mohoseinam</a>)
- Mohammad Ataei (<a href="https://twitter.com/mammadataei">@mammadataei</a>)
- Saleh Khalaj Monfared (<a href="https://twitter.com/S4l3hh">@S4l3hh</a>)
- Alee Amini (<a href="https://twitter.com/AleeAmini">@AleeAmini</a>)

## Contributing
Contributing in HyperDbg is super appreciated.

If you want to create a pull request or contribute in HyperDbg please read [Contribution Guide](https://github.com/HyperDbg/HyperDbg/blob/master/CONTRIBUTING.md).


## License
Dependencies are licensed by their own licenses.

**HyperDbg** is under **GPLv3** LICENSE.
