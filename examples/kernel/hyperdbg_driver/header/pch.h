/**
 * @file pch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Pre-compiled headers for RM
 * @details
 * @version 0.2
 * @date 2023-01-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

//
// Scope definitions
//
#define HYPERDBG_KERNEL_MODE
#define HYPERDBG_RM

//
// General WDK headers
//
#include <ntifs.h>
#include <Windef.h>

//
// Definition of Intel primitives (External header)
//
// #include "ia32-doc/out/ia32.h"

//
// Import Configuration and Definitions
//
#include "Configuration.h"

//
// Macros
//
#include "macros/MetaMacros.h"

//
// HyperDbg SDK headers
//
#include "SDK/HyperDbgSdk.h"

//
// Import HyperLog Module
//
#include "SDK/modules/HyperLog.h"
#include "SDK/imports/kernel/HyperDbgHyperLogImports.h"
#include "SDK/imports/kernel/HyperDbgHyperLogIntrinsics.h"

//
// Import VMM Module
//
#include "SDK/modules/VMM.h"
#include "SDK/imports/kernel/HyperDbgVmmImports.h"

//
// Local Driver headers
//
#include "driver/Driver.h"
#include "driver/Loader.h"

//
// Core headers
//
#include "core/Core.h"

//
// Spinlock component
//
#include "components/spinlock/header/Spinlock.h"

//
// ---------------------------------------------
// Global Variables (Should be last)
//
#include "misc/Global.h"
