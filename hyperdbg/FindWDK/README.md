# FindWDK
CMake module for building drivers with Windows Development Kit (WDK) [![Build status](https://ci.appveyor.com/api/projects/status/o7cyircahkb6nv07/branch/master?svg=true)](https://ci.appveyor.com/project/SergiusTheBest/findwdk/branch/master) [![CI](https://github.com/SergiusTheBest/FindWDK/actions/workflows/ci.yml/badge.svg)](https://github.com/SergiusTheBest/FindWDK/actions/workflows/ci.yml)

- [Introduction](#introduction)
- [Usage](#usage)
  - [Kernel driver](#kernel-driver)
  - [Kernel library](#kernel-library)
  - [Linking to WDK libraries](#linking-to-wdk-libraries)
- [Samples](#samples)
- [License](#license)
- [Version history](#version-history)

# Introduction
FindWDK makes it possible to build kernel drivers and kernel libraries with Windows Development Kit (WDK) and CMake.

Requirements:
- WDK 8.0 and higher
- Visual Studio 2015 and higher
- CMake 3.0 and higher

# Usage
Add FindWDK to the module search path and call `find_package`:

```cmake
list(APPEND CMAKE_MODULE_PATH "<path_to_FindWDK>")
find_package(WDK REQUIRED)
```

FindWDK will search for the latest installed Windows Development Kit (WDK) and expose commands for creating kernel drivers and kernel libraries. Also the following variables will be defined:
- `WDK_FOUND` -- if false, do not try to use WDK
- `WDK_ROOT` -- where WDK is installed
- `WDK_VERSION` -- the version of the selected WDK
- `WDK_WINVER` -- the WINVER used for kernel drivers and libraries (default value is `0x0601` and can be changed per target or globally)
- `WDK_NTDDI_VERSION` -- the NTDDI_VERSION used for kernel drivers and libraries, if not set, the value will be automatically calculated by WINVER

`WDKContentRoot` environment variable overrides the default WDK search path.

## Kernel driver
The following command adds a kernel driver target called `<name>` to be built from the source files listed in the command invocation:

```cmake
wdk_add_driver(<name> 
    [EXCLUDE_FROM_ALL]
    [KMDF <kmdf_version>]
    [WINVER <winver_version>]
    [NTDDI_VERSION <ntddi_version>]
    source1 [source2 ...]
    )
```

Options:
- `EXCLUDE_FROM_ALL` -- exclude from the default build target
- `KMDF <kmdf_version>` -- use KMDF and set KMDF version
- `WINVER <winver_version>` -- use specific WINVER version
- `NTDDI_VERSION <ntddi_version>` -- use specific NTDDI_VERSION

Example:

```cmake
wdk_add_driver(KmdfCppDriver 
    KMDF 1.15 
    WINVER 0x0602
    Main.cpp
    )
```

## Kernel library
The following command adds a kernel library target called `<name>` to be built from the source files listed in the command invocation:

```cmake
wdk_add_library(<name> [STATIC | SHARED]
    [EXCLUDE_FROM_ALL]
    [KMDF <kmdf_version>]
    [WINVER <winver_version>]
    [NTDDI_VERSION <ntddi_version>]
    source1 [source2 ...]
    )
```

Options:
- `EXCLUDE_FROM_ALL` -- exclude from the default build target
- `KMDF <kmdf_version>` -- use KMDF and set KMDF version
- `WINVER <winver_version>` -- use specific WINVER version
- `NTDDI_VERSION <ntddi_version>` -- use specific NTDDI_VERSION
- `STATIC or SHARED` -- specify the type of library to be created

Example:

```cmake
wdk_add_library(KmdfCppLib STATIC 
    KMDF 1.15
    WINVER 0x0602
    KmdfCppLib.h 
    KmdfCppLib.cpp
    )
```

## Linking to WDK libraries
FindWDK creates imported targets for all WDK libraries. The naming pattern is `WDK::<UPPERCASED_LIBNAME>`. Linking a minifilter driver to `FltMgr.lib` is shown below:

```cmake
target_link_libraries(MinifilterCppDriver WDK::FLTMGR)
```

# Samples
Take a look at the [samples](samples) folder to see how WMD and KMDF drivers and libraries are built.

# License
FindWDK is licensed under the OSI-approved 3-clause BSD license. You can freely use it in your commercial or opensource software.

# Version history

## Version 1.0.2 (TBD)

## Version 1.0.1 (13 Mar 2018)
- New: Add ability to link to WDK libraries
- New: Add MinifilterCppDriver sample
- Fix: W4 warnings in C version of the driver, add missing /W4 /WX for C compiler

## Version 1.0.0 (03 Feb 2018)
- Initial public release
