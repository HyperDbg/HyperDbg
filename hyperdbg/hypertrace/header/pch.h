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
#include "platform/general/header/Environment.h"

#ifdef HYPERDBG_ENV_WINDOWS

//
// Windows defined functions
//
#    include <ntddk.h>
#    include <ntstrsafe.h>
#    include <Windef.h>

#endif // HYPERDBG_ENV_WINDOWS

//
// Scope definitions
//
#define HYPERDBG_KERNEL_MODE
#define HYPERDBG_HYPERTRACE

//
// Add ia32-doc
//
#include "ia32-doc/out/ia32.h"

//
// SDK headers
//
#include "SDK/HyperDbgSdk.h"

//
// Configuration
//
#include "config/Configuration.h"

//
// Platform independent headers
//
#include "platform/kernel/header/PlatformMem.h"

//
// DPC and broadcasting function headers
//
#include "broadcast/Dpc.h"
#include "broadcast/DpcRoutines.h"
#include "broadcast/Broadcast.h"

//
// Unload function (to be called when the driver is unloaded)
//
#include "common/UnloadDll.h"

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
// Definition of general tracing types
//
#include "api/TraceApi.h"

//
// Definition of tracing types and structures (Last Branch Record)
//
#include "api/LbrApi.h"
#include "lbr/Lbr.h"

//
// Definition of tracing types and structures (Processor Trace)
//
#include "api/PtApi.h"
#include "pt/Pt.h"

//
// Export functions
//
#include "SDK/imports/kernel/HyperDbgHyperTrace.h"

//
// Global variables
//
#include "globals/GlobalVariables.h"
