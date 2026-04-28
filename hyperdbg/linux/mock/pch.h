/**
 * @file pch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Pre-compiled headers for Linux mock
 * @details
 * @version 0.19
 * @date 2026-04-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

//
// Scope definitions
//
#define HYPERDBG_KERNEL_MODE
#define HYPERDBG_LINUX

//
// General Linux driver headers
//
#include <linux/module.h>

//
// Platform headers
//
#include "../../include/SDK/HyperDbgSdk.h"

//
// Environment headers
//
#include "../../include/platform/general/header/Environment.h"

//
// Platform headers
//
#include "../../include/platform/kernel/header/PlatformMem.h"
