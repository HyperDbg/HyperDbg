/**
 * @file LinuxPrelude.h
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief Force-included in front of every TU of HyperDbg.ko
 *
 * @details Each module keeps its OWN pch.h and each TU keeps resolving
 * `#include "pch.h"` to it, exactly as on Windows (see the per-object include
 * roots in the root Kbuild). What no module pch can provide is the Linux side:
 * the kernel headers, and the scope macros that a Windows .vcxproj would pass
 * as /D. Those live here and are pushed in front of every TU with
 * `-include`, so no module pch had to be edited to carry a Linux arm.
 *
 * Keep this SMALL. Anything module-specific belongs in that module's pch;
 * anything shared belongs in the SDK or the platform layer.
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#pragma once

//
// Scope definitions. On Windows these are per-project (each pch.h defines its
// own set); in a single module they are simply all true at once. Only
// HYPERDBG_KERNEL_MODE and HYPERDBG_HYPER_LOG are actually tested anywhere —
// see the greps in linux/PORTING_STATUS.md — so the union is safe.
//
#define HYPERDBG_KERNEL_MODE
#define HYPERDBG_LINUX

//
// General Linux kernel headers
//
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/workqueue.h> // struct work_struct + BH workqueue — backs the Linux KDPC (see DataTypes.h)
#include <linux/spinlock.h>  // spinlock_t — backs the Linux KSPIN_LOCK (see DataTypes.h)

//
// The WDK type surface. On Windows every module pch opens with <ntddk.h>, so NT
// types are in scope before the module's own headers are reached (hyperlog's
// pch includes UnloadDll.h, which says NTSTATUS, before it includes the SDK).
// BasicTypes.h — which pulls in WdkTypes.h — plays exactly that role here, so
// it has to arrive just as early.
//
#include "SDK/headers/BasicTypes.h"
