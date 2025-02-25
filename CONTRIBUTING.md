# Contribution

First off, thanks for taking the time to contribute! ❤️

HyperDbg is a large-scale project that requires a lot of time and effort from the community. Given the current number of developers and their limited time and resources, we cannot develop every part simultaneously. Therefore, new developers are warmly welcomed to join and add their contributions to the project. Here, we made a list of potential improvements that you can contribute on. Feel free to open up an issue if you think you have any ideas that would make a good addition to the list.

## Things to Work on

Please make sure to create a [discussion](https://github.com/orgs/HyperDbg/discussions) or an [issue](https://github.com/HyperDbg/HyperDbg/issues), or even better, join the HyperDbg groups ([Telegram](https://t.me/HyperDbg), [Discord](https://discord.gg/anSPsGUtzN), [Matrix](https://matrix.to/#/#hyperdbg-discussion:matrix.org)) on social media. Discuss the way you want to implement your changes and inform developers, because we often see people simultaneously working on the same issue. To avoid this collision, make sure to inform us before you start developing.

- Writing blog posts and creating videos about use-cases of HyperDbg (make sure to add it to the [awesome](https://github.com/HyperDbg/awesome) repository).
- Fixing unresolved GitHub [issues](https://github.com/HyperDbg/HyperDbg/issues).
- Troubleshooting problems with running on Hyper-V's nested virtualization.
- Troubleshooting problems with running on VirtualBox's nested virtualization.
- Supporting KDNET (sending data over the network).
- Enhancing HyperDbg's [Transparent Mode](https://docs.hyperdbg.org/using-hyperdbg/prerequisites/operation-modes#transparent-mode), especially for anti-hypervisor methods.
- Enhancing and adding more features to the ['.pe'](https://docs.hyperdbg.org/commands/meta-commands/.pe) command.
- Adding HyperDbg to the system startup using UEFI.
- Adding routines to activate and use Last Branch Record (LBR) and Branch Trace Store (BTS).
- Creating a QT-based GUI.
- Creating a SoftICE-style GUI.
- Supporting nested-virtualization on HyperDbg itself.
- Protecting HyperDbg code and memory from modification using VT-x capabilities.
- Adding support for the Intel Processor Trace (PT).
- Creating a wrapper that automatically interprets the [HyperDbg SDK](https://github.com/HyperDbg/HyperDbg/tree/master/hyperdbg/include/SDK) to GO, RUST, C#, Python, etc.
- Creating syntax highlighting for dslang for different IDEs (VSCode, VIM, etc.).
- Building HyperDbg using LLVM clang.
- Helping us start supporting HyperDbg on Linux (discussion needed).
- Helping us start supporting HyperDbg on AMD processors (discussion needed).
- Adding digital (FPGA) modules to the hwdbg hardware debugger.
- Creating a [ret-sync](https://github.com/bootleg/ret-sync) module for HyperDbg.
- Adding fuzzing capabilities to HyperDbg (maybe integrating [AFL++](https://github.com/AFLplusplus/AFLplusplus) into HyperDbg).
- Working on live memory migration and adding support for kernel-mode time travel debugging.
- Integrating the [z3 project](https://github.com/Z3Prover/z3) into HyperDbg and adding commands based on the z3 solver.
- Adding the [Bochs emulator](https://github.com/bochs-emu/Bochs) to HyperDbg.
- ~~Creating commands to inspect and read details of PCIe devices.~~ Added: [<a href="https://docs.hyperdbg.org/commands/extension-commands/pcitree" target="_blank">link</a>][<a href="https://docs.hyperdbg.org/commands/extension-commands/pcicam" target="_blank">link</a>]
- ~~Mitigating the anti-hypervisor method described [here](https://howtohypervise.blogspot.com/2019/01/a-common-missight-in-most-hypervisors.html).~~ [[Fixed](https://github.com/HyperDbg/HyperDbg/pull/497)]
- Creating different examples of how to use the SDK (using different programming languages).
- Debugging and fixing bugs related to HyperDbg's physical serial communication.
- Reading symbol information from modules in memory (currently, HyperDbg opens a file which continues the debugger).
- Adding APIC virtualization.
- Reading the list of modules for the '[lm](https://docs.hyperdbg.org/commands/debugging-commands/lm)' command directly from kernel-mode.
- Detecting and fixing anti-hypervisor methods described [here](https://github.com/Ahora57/MAJESTY-technologies).
- Investigating why the symbols parser (DIA SDK) could not read symbols of the 'kernel32!*'.
- ~~Fixing the problem with [XSETBV instruction freezing](https://github.com/HyperDbg/HyperDbg/issues/429).~~ [[Fixed](https://github.com/HyperDbg/HyperDbg/pull/491)]
- Adding an event function that detects coverage.
- Bypassing [al-khaser](https://github.com/LordNoteworthy/al-khaser).
- Creating the 'alias' command that converts or registers scripts as a command, for example: "alias !list .script list.dbg" (discussion needed).
- Adding support for [Hardware Performance Counters (HPC)](https://en.wikipedia.org/wiki/Hardware_performance_counter).

- Any other interesting tasks you might find!

This list will be updated frequently.

## Fixing Bugs

HyperDbg likely contains numerous bugs and may not have considered various scenarios that could lead to system crashes. We consistently invest a substantial amount of time in addressing these issues to improve the overall user experience with HyperDbg. Your assistance in identifying and fixing bugs would be greatly appreciated. Here's a glimpse into how HyperDbg developers allocate their daily time:

<img align="center" width="50%" height="30%" src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Memes/bug-fix.jpg" alt="Fixing bugs!">

## Coding Style
If you want to create a pull request please read [coding-style](https://docs.hyperdbg.org/contribution/style-guide/coding-style), [doxygen-style](https://docs.hyperdbg.org/contribution/style-guide/doxygen-style), and [command-style](https://docs.hyperdbg.org/contribution/style-guide/command-style) in the case you want a new feature, or report a bug, please explain it in [issues](https://GitHub.com/HyperDbg/HyperDbg/issues/).

## FAQ
Here are some Frequently Asked Questions which may help with your moving process

#### Did you write a patch that fixes a bug?
Open a new GitHub pull request with the patch and also ensure the PR description clearly describes the problem and solution.

#### Suggesting Enhancements

If you want to submit an enhancement or suggestion for HyperDbg, including completely new features and minor improvements to existing functionality please explain it in [issues](https://GitHub.com/HyperDbg/HyperDbg/issues/).

#### Would you like to add a new feature or change an existing one?
Suggest your change by opening an [issue](https://GitHub.com/HyperDbg/HyperDbg/issues/) and start writing code.

#### Do you have questions about the source code?
Ask any question about HyperDbg by opening an [issue](https://GitHub.com/HyperDbg/HyperDbg/issues/).

## Source-code Tree
Here's the overview of HyperDbg's source code, including the project's relations and dependencies.
</br></br></br>
<img align="center" width="90%" height="50%" src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Diagrams/source-tree/source-tree.png" alt="Source Tree">


