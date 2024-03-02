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

#pragma warning(disable : 4201) // Suppress nameless struct/union warning

//
// Scope definitions
//
#define HYPERDBG_KERNEL_MODE
#define HYPERDBG_DEBUGGER
#define SCRIPT_ENGINE_KERNEL_MODE

//
// General WDK headers
//
#include <ntifs.h>
#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <Windef.h>

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
// Optimization algorithms
//
#include "components/optimizations/header/AvlTree.h"
#include "components/optimizations/header/BinarySearch.h"
#include "components/optimizations/header/InsertionSort.h"

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
#include "header/debugger/user-level/UserAccess.h"
#include "header/debugger/user-level/ThreadHolder.h"
#include "header/debugger/core/DebuggerVmcalls.h"
#include "header/debugger/core/HaltedCore.h"

//
// Broadcast functions
//
#include "header/debugger/broadcast/DpcRoutines.h"
#include "header/debugger/broadcast/HaltedRoutines.h"
#include "header/debugger/broadcast/HaltedBroadcast.h"

//
// DPC Headers
//
#include "header/common/Dpc.h"

//
// Events & Meta events
//
#include "header/debugger/events/ApplyEvents.h"
#include "header/debugger/events/Termination.h"
#include "header/debugger/events/DebuggerEvents.h"
#include "header/debugger/events/ValidateEvents.h"
#include "header/debugger/meta-events/Tracing.h"
#include "header/debugger/meta-events/MetaDispatch.h"

//
// Script engine headers
//
#include "../script-eval/header/ScriptEngineCommonDefinitions.h"
#include "../script-eval/header/ScriptEngineHeader.h"

//
// Global variables
//
#include "globals/Global.h"
