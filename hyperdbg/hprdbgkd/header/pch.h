/**
 * @file pch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Pre-compiled headers for debugger
 * @details
 * @version 0.2
 * @date 2023-01-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include <ntifs.h>
#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <Windef.h>

//
// Scope definitions
//
#define SCRIPT_ENGINE_KERNEL_MODE
#define HYPERDBG_KERNEL_MODE

//
// Import Configuration
//
#include "Configuration.h"

//
// HyperDbg SDK headers
//
#include "SDK/HyperDbgSdk.h"
#include "SDK/Imports/HyperDbgVmmImports.h"
#include "../../hprdbghv/header/common/RefactorTempStructs.h" // should be removed

//
// Import HyperLog Module
//
#include "SDK/Modules/HyperLog.h"
#include "SDK/Imports/HyperDbgHyperLogImports.h"
#include "SDK/Imports/HyperDbgHyperLogIntrinsics.h"

//
// Local Debugger headers
//
#include "globals/Global.h"
#include "driver/Driver.h"
#include "driver/Loader.h"
