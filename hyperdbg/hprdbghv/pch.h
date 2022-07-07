/**
 * @file pch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author Alee Amini (alee@hyperdbg.org)
 * @brief Pre-compiled headers
 * @details
 *
 * @version 0.1
 * @date 2020-07-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#define _NO_CRT_STDIO_INLINE

#pragma warning(disable : 4201) // suppress nameless struct/union warning

//
// Windows defined functions
//
#include <ntifs.h>
#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <Windef.h>
#include <intrin.h>

//
// Scope definitions
//
#define SCRIPT_ENGINE_KERNEL_MODE
#define HYPERDBG_KERNEL_MODE

//
// External headers
//
#include "ia32-doc/out/ia32.h"

//
// HyperDbg Kernel-mode headers
//
#include "SDK/HyperDbgSdk.h"
#include "Definition.h"
#include "Configuration.h"
#include "platform/CrossApi.h"
#include "platform/Environment.h"
#include "platform/MetaMacros.h"

#include "..\hprdbghv\header\vmm\vmx\VmxBroadcast.h"
#include "..\hprdbghv\header\common\Dpc.h"
#include "..\hprdbghv\header\common\LengthDisassemblerEngine.h"
#include "..\hprdbghv\header\common\Logging.h"
#include "..\hprdbghv\header\memory\MemoryMapper.h"
#include "..\hprdbghv\header\common\Msr.h"
#include "..\hprdbghv\header\debugger\tests\KernelTests.h"
#include "..\hprdbghv\header\memory\PoolManager.h"
#include "..\hprdbghv\header\common\Trace.h"
#include "..\hprdbghv\header\debugger\core\Debugger.h"
#include "..\hprdbghv\header\debugger\broadcast\DpcRoutines.h"
#include "..\hprdbghv\header\misc\InlineAsm.h"
#include "..\hprdbghv\header\vmm\ept\Vpid.h"
#include "..\hprdbghv\header\vmm\ept\Ept.h"
#include "..\hprdbghv\header\common\Common.h"
#include "..\hprdbghv\header\vmm\vmx\Events.h"
#include "..\hprdbghv\header\debugger\script-engine\ScriptEngine.h"
#include "..\hprdbghv\header\devices\Apic.h"
#include "..\hprdbghv\header\debugger\kernel-level\Kd.h"
#include "..\hprdbghv\header\debugger\user-level\Ud.h"
#include "..\hprdbghv\header\vmm\vmx\Mtf.h"
#include "..\hprdbghv\header\debugger\core\DebuggerEvents.h"
#include "..\hprdbghv\header\debugger\features\Hooks.h"
#include "..\hprdbghv\header\vmm\vmx\Counters.h"
#include "..\hprdbghv\header\debugger\transparency\Transparency.h"
#include "..\hprdbghv\header\vmm\vmx\IdtEmulation.h"
#include "..\hprdbghv\header\vmm\ept\Invept.h"
#include "..\hprdbghv\header\debugger\broadcast\Broadcast.h"
#include "..\hprdbghv\header\vmm\vmx\Vmcall.h"
#include "..\hprdbghv\header\vmm\vmx\ManageRegs.h"
#include "..\hprdbghv\header\vmm\vmx\Vmx.h"
#include "..\hprdbghv\header\debugger\commands\BreakpointCommands.h"
#include "..\hprdbghv\header\debugger\commands\DebuggerCommands.h"
#include "..\hprdbghv\header\debugger\commands\ExtensionCommands.h"
#include "..\hprdbghv\header\debugger\commands\Callstack.h"
#include "..\hprdbghv\header\debugger\communication\SerialConnection.h"
#include "..\hprdbghv\header\debugger\objects\Process.h"
#include "..\hprdbghv\header\debugger\objects\Thread.h"
#include "..\hprdbghv\header\components\registers\DebugRegisters.h"
#include "..\hprdbghv\header\vmm\vmx\Hv.h"
#include "..\hprdbghv\header\vmm\vmx\MsrHandlers.h"
#include "..\hprdbghv\header\vmm\vmx\ProtectedHv.h"
#include "..\hprdbghv\header\vmm\vmx\IoHandler.h"
#include "..\hprdbghv\header\vmm\vmx\VmxMechanisms.h"
#include "..\hprdbghv\header\debugger\user-level\Attaching.h"
#include "..\hprdbghv\header\debugger\core\Termination.h"
#include "..\hprdbghv\header\debugger\user-level\UserAccess.h"
#include "..\hprdbghv\header\debugger\user-level\ThreadHolder.h"
#include "..\script-eval\header\ScriptEngineCommonDefinitions.h"
#include "..\script-eval\header\ScriptEngineHeader.h"

//
// Global Variables should be the last header to include
//
#include "..\hprdbghv\header\globals\GlobalVariableManagement.h"
#include "..\hprdbghv\header\globals\GlobalVariables.h"
