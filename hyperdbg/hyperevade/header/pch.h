/**
 * @file pch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers of Message logging and tracing
 * @details
 * @version 0.2
 * @date 2023-01-23
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#pragma once

#define _NO_CRT_STDIO_INLINE

#pragma warning(disable : 4201) // Suppress nameless struct/union warning

//
// Environment headers
//
#include "platform/kernel/header/Environment.h"

#ifdef ENV_WINDOWS

//
// The DLL is flagged by antivirus software, since it contains anti-debugging and anti-hypervisor methods
// as well as different anti-debugging strings
// For now, we disable the HyperDbg Hyperevade module
//
#    define DISABLE_HYPERDBG_HYPEREVADE TRUE

//
// Windows defined functions
//
#    include <ntddk.h>
#    include <ntstrsafe.h>
#    include <Windef.h>

#endif // ENV_WINDOWS

//
// Scope definitions
//
#define HYPERDBG_KERNEL_MODE
#define HYPERDBG_HYPEREVADE

//
// Macros
//
#include "macros/MetaMacros.h"

//
// Definition of Intel primitives (External header)
//
#include "ia32-doc/out/ia32.h"

//
// HyperDbg SDK headers
//
#include "SDK/HyperDbgSdk.h"

//
// Configuration
//
#include "config/Configuration.h"

//
// Hyperlog headers
//
#include "components/interface/HyperLogCallback.h"
#include "SDK/imports/kernel/HyperDbgHyperLogIntrinsics.h"

//
// Platform independent headers
//
#include "platform/kernel/header/Mem.h"

//
// Hyperevade Callbacks
//
#include "SDK/modules/HyperEvade.h"

//
// Transparency and footprints headers
//
#include "Transparency.h"
#include "VmxFootprints.h"
#include "SyscallFootprints.h"

//
// Optimization algorithms
//
#include "components/optimizations/header/AvlTree.h"
#include "components/optimizations/header/BinarySearch.h"
#include "components/optimizations/header/InsertionSort.h"

//
// Spinlocks
//
#include "components/spinlock/header/Spinlock.h"

//
// Hyper-V TLFS
//
#include "hyper-v/HypervTlfs.h"

//
// Export functions
//
#include "SDK/imports/kernel/HyperDbgHyperEvade.h"
