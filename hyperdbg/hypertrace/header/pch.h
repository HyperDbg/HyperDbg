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
#define HYPERDBG_HYPER_LOG

#include "UnloadDll.h"
#include "SDK/HyperDbgSdk.h"
#include "SDK/modules/HyperLog.h"
#include "SDK/imports/kernel/HyperDbgHyperLogImports.h"
#include "components/spinlock/header/Spinlock.h"
#include "Tracing.h"

//
// Platform independent headers
//
#include "platform/kernel/header/Mem.h"
