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
#define HYPERDBG_HYPERTRACE

//
// Unload function (to be called when the driver is unloaded)
//
#include "UnloadDll.h"

//
// SDK headers
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
// Spinlock headers
//
#include "components/spinlock/header/Spinlock.h"

//
// Hypertrace Callbacks
//
#include "SDK/modules/HyperTrace.h"

//
// Definition of tracing types and structures
//
#include "Tracing.h"

//
// Platform independent headers
//
#include "platform/kernel/header/Mem.h"

//
// Export functions
//
#include "SDK/imports/kernel/HyperDbgHyperTrace.h"
