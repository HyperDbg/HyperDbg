/**
 * @file ZydisKernel-linux.c
 * @brief Linux stand-in for the Windows-only ZydisKernel.c
 *
 * @details The upstream ZydisKernel.c is a byte-for-byte reformat of Zydis's own
 * *sample* kernel driver (`dependencies/zydis/examples/ZydisWinKernel.c`): its
 * single function `DriverEntryTest` finds the driver's own image base via the WDK
 * PE helpers (`<ntimage.h>`, `RtlPcToFileHeader`, `RtlImageNtHeader`,
 * `IMAGE_FIRST_SECTION`), disassembles its INIT section and prints it, then
 * deliberately returns a failure status so the sample never stays resident. It is
 * called by nothing in HyperDbg — pure demonstration code.
 *
 * Zydis ships only a Windows kernel sample (hence `ZydisWinKernel.c`); there is no
 * Linux-kernel counterpart upstream, because a Linux module has no `DriverEntry`
 * and no PE image to walk. Since the TU is dead code AND its only dependency is the
 * Windows PE image layout, the Linux module swaps the whole file for this empty
 * stub (the established `*-linux.c` file-swap convention) rather than shimming
 * `<ntimage.h>` and the `IMAGE_*` types into WdkTypes.h for a function no caller
 * invokes. The Windows build keeps compiling the original ZydisKernel.c unchanged
 * (still in hyperhv.vcxproj); this file is Kbuild-only.
 *
 * TODO(Linux): nothing owed — resurrect only if a Linux self-disassembly sample is
 * ever wanted, at which point it would walk the ELF module layout instead of PE.
 */

#include "pch.h"
