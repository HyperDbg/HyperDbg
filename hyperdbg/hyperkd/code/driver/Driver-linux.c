/**
 * @file Driver-linux.c
 * @brief Linux stand-in for the Windows-only Driver.c (WDM driver bootstrap)
 *
 * @details The upstream Driver.c is pure WDM: DriverEntry() builds a DRIVER_OBJECT/
 * DEVICE_OBJECT, fills the IRP_MJ_* dispatch table (DrvCreate/Read/Write/Close/
 * Unsupported), creates the \\Device and \\DosDevices symbolic link, and gates
 * access with SeSinglePrivilegeCheck(SE_DEBUG_PRIVILEGE). None of that model
 * exists on Linux (DRIVER_OBJECT is only an opaque forward-decl in WdkTypes.h),
 * so — following the established `*-linux.c` whole-file swap convention (see
 * ZydisKernel-linux.c) — the Linux module compiles this stand-in instead of
 * Driver.c. The Windows build keeps compiling the original Driver.c unchanged
 * (still in hyperkd.vcxproj); this file is Kbuild-only.
 *
 * It is intentionally an EMPTY skeleton for now: the char-device layer is the one
 * genuine design piece left in hyperkd and is deferred to its own pass.
 *
 * TODO(Linux): replace the WDM bootstrap with a misc/char device —
 *   - module_init()/module_exit() in place of DriverEntry()/DrvUnload()
 *   - alloc_chrdev_region() + cdev_add() (or misc_register()) for the /dev node
 *   - a struct file_operations whose .unlocked_ioctl dispatches into the existing
 *     IOCTL handler (Ioctl.c, DrvDispatchIoControl equivalent), .open/.release
 *     standing in for DrvCreate/DrvClose
 *   - capable(CAP_SYS_PTRACE)/CAP_SYS_ADMIN in place of the SE_DEBUG_PRIVILEGE check
 */

#include "pch.h"
