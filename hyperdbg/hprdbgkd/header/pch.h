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
#define HYPERDBG_KERNEL_MODE
#define HYPERDBG_DEBUGGER
#define SCRIPT_ENGINE_KERNEL_MODE

//
// Definition of Intel primitives (External header)
//
#include "ia32-doc/out/ia32.h"

//
// Import Configuration and Definitions
//
#include "Configuration.h"
#include "Definition.h"

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
#include "SDK/Modules/HyperLog.h"
#include "SDK/Imports/HyperDbgHyperLogImports.h"
#include "SDK/Imports/HyperDbgHyperLogIntrinsics.h"

//
// Import VMM Module
//
#include "SDK/Modules/VMM.h"
#include "SDK/Imports/HyperDbgVmmImports.h"

//
// Local Debugger headers
//
#include "debugger/core/State.h"
#include "driver/Driver.h"
#include "driver/Loader.h"

//
// Spinlock component
//
#include "components/spinlock/header/Spinlock.h"

//
// Debugger Types
//
#include "header/debugger/core/Debugger.h"

//
// Assembly files
//
#include "header/assembly/Assembly.h"

//
// Debugger Sub-types
//
#include "header/debugger/tests/KernelTests.h"
#include "header/debugger/broadcast/DpcRoutines.h"
#include "header/debugger/core/DebuggerEvents.h"
#include "header/debugger/script-engine/ScriptEngine.h"
#include "header/debugger/memory/Memory.h"
#include "header/common/Common.h"
#include "header/debugger/memory/Allocations.h"
#include "header/debugger/kernel-level/Kd.h"
#include "header/debugger/user-level/Ud.h"
#include "header/debugger/commands/BreakpointCommands.h"
#include "header/debugger/commands/DebuggerCommands.h"
#include "header/debugger/commands/ExtensionCommands.h"
#include "header/debugger/commands/Callstack.h"
#include "header/debugger/communication/SerialConnection.h"
#include "header/debugger/objects/Process.h"
#include "header/debugger/objects/Thread.h"
#include "header/debugger/user-level/Attaching.h"
#include "header/debugger/core/Termination.h"
#include "header/debugger/user-level/UserAccess.h"
#include "header/debugger/user-level/ThreadHolder.h"
#include "header/debugger/broadcast/DpcRoutines.h"
#include "header/debugger/core/DebuggerVmcalls.h"

//
// DPC Headers
//
#include "header/common/Dpc.h"

//
// Script engine headers
//
#include "../script-eval/header/ScriptEngineCommonDefinitions.h"
#include "../script-eval/header/ScriptEngineHeader.h"

//
// Global variables
//
#include "globals/Global.h"
