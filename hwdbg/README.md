<p align="center">
   <img alt="hwdbg" title="hwdbg" src="https://github.com/HyperDbg/graphics/blob/master/Logos/hwdbg/hwdbg-high-resolution-logo-transparent.png?raw=true" width="300">
</p>

<p align="left">
<a href="https://hwdbg.hyperdbg.org"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Website-orange.svg" alt="Website"></a>
<a href="https://hwdbg.hyperdbg.org/docs"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Docs-yellow.svg" alt="Docs"></a>
<a href="https://hwdbg.hyperdbg.org/api"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-API-green.svg" alt="API"></a>
<a href="https://research.hyperdbg.org"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/Link-Research-pink.svg" alt="Published Researches"></a>
<a href="https://www.gnu.org/licenses/gpl-3.0"><img src="https://raw.githubusercontent.com/HyperDbg/graphics/master/Badges/License-GPLv3-blue.svg" alt="License"></a>
</p>

## Description
The **hwdbg** debugger chip generator is a gate-level debugging tool designed to make configurable and synthesizable hardware debuggers for white-box and black-box chip fuzzing, testing, and reverse engineering. The primary goal of **hwdbg** is to provide control over hardware, enabling monitoring and modification of signals down to the granular level of a single clock cycle. It is written in Chisel and Verilog.

- ⚠️ This project is a work in progress and is not yet ready for testing.

**hwdbg** is a highly customizable debugger designed to ease hardware debugging by bringing software debugging concepts into the hardware debugging domain. **hwdbg** aims to help with the complexities associated with debugging hardware, including chips and IP cores. Key features of **hwdbg** include the ability to step through the hardware design at the clock-cycle level, visualize waveforms, inspect values (e.g., like a logical analyzer), and modify signals. Moreover, it is synthesizable into [FPGAs](https://github.com/HyperDbg/hwdbg-fpga) and has the potential for fabrication into physical chips.

```
            ┏━━━━━━━━━━━━━━━━━━━━━━━┓
     _             _  _             ┃
    | |_  _ _ _  _| || |_  ___      ┃
    | . || | | |/ . || . \/ . |     ┃
    |_|_||__/_/ \___||___/\_. |     ┃
                          <___'     ┃
                                    ┃      ╱|、
  HyperDbg's chip-level debugger    ┃     (˚ˎ 。7
                                    ┃     |、 ˜〵
            ┗━━━━━━━━━━━━━━━━━━━━━━━┛     じしˍ,)ノ
```

## Deployment Board 

[This repository](https://github.com/HyperDbg/hwdbg-fpga) contains pre-built TCL files to facilitate project creation for running **hwdbg** on various FPGA development boards.

## Output 

For generating SystemVerilog files, you need to install [Chisel](https://www.chisel-lang.org/docs/installation). Once installed, use the following commands:

```sh
$ sbt run
```

This command prompts you to select a component. The `hwdbg.Main` class contains the debugger for synthesis purposes, while the `hwdbg.MainWithInitializedBRAM` class includes a pre-initialized Block RAM (BRAM), primarily for simulation and testing.

After selecting the appropriate class for synthesis (option `1`) or simulation (option `2`), the output should look like this:

```sh
$ sbt run
[info] welcome to sbt 1.9.7 (Eclipse Adoptium Java 17.0.10)
[info] loading settings for project -build-build-build from metals.sbt ...
[info] loading project definition from /home/sina/HyperDbg//project/project/project
[info] loading settings for project -build-build from metals.sbt ...
[info] loading project definition from /home/sina/HyperDbg//project/project
[success] Generated .bloop/-build-build.json
[success] Total time: 1 s, completed Apr 16, 2024, 1:49:05 PM
[info] loading settings for project -build from metals.sbt,plugins.sbt ...
[info] loading project definition from /home/sina/HyperDbg//project
[success] Total time: 0 s, completed Apr 16, 2024, 1:49:05 PM
[info] loading settings for project root from build.sbt ...
[info] set current project to hwdbg (in build file:/home/sina/HyperDbg/hwdbg/)

Multiple main classes detected. Select one to run:
 [1] hwdbg.Main
 [2] hwdbg.MainWithInitializedBRAM

Enter number: 2
[info] running hwdbg.MainWithInitializedBRAM
```

The generated code for the debugger can be found in the `generated` directory.

## Testbenches

To test **hwdbg**, [cocotb](https://www.cocotb.org/) should be installed. After that, first, run the debugger (generated SystemVerilog files) and then run the following commands:

```sh
cd sim/hwdbg/DebuggerModuleTestingBRAM
./test.sh
```

The above command generates a waves file at `./sim/hwdbg/DebuggerModuleTestingBRAM/sim_build/DebuggerModuleTestingBRAM.fst` which can be read using [GTKWave](https://gtkwave.sourceforge.net/).

```sh
cd sim/hwdbg/DebuggerModuleTestingBRAM
gtkwave ./sim_build/DebuggerModuleTestingBRAM.fst
```

### ModelSim

If you prefer to use ModelSim instead of GTKWave, you can configure the `modelsim.config` file. Please visit <a href="https://github.com/HyperDbg/hwdbg/blob/main/sim/modelsim/README.md">here</a> for more information.

## API

If you want to create the latest version of API documentation, you can run the following command:

```sh
$ sbt doc
```

This will generate documentation at `./target/scala-{version}/api/index.html`.

## License

**hwdbg** and all its submodules and repos, unless a license is otherwise specified, are licensed under **GPLv3** LICENSE.
