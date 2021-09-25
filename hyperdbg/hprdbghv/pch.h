/**
 * @file pch.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @author Alee Amini (aleeaminiz@gmail.com)
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

//
// Windows defined functions
//
#include <ntifs.h>
#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <Windef.h>

//
// HyperDbg Kernel-mode headers
//
#include "Definition.h"
#include "Configuration.h"
#include "..\hprdbghv\Header Files\Common\Dpc.h"
#include "..\hprdbghv\Header Files\Common\LengthDisassemblerEngine.h"
#include "..\hprdbghv\Header Files\Common\Logging.h"
#include "..\hprdbghv\Header Files\Common\MemoryMapper.h"
#include "..\hprdbghv\Header Files\Common\Msr.h"
#include "..\hprdbghv\Header Files\Debugger\Tests\KernelTests.h"
#include "..\hprdbghv\Header Files\Common\PoolManager.h"
#include "..\hprdbghv\Header Files\Common\Trace.h"
#include "..\hprdbghv\Header Files\Debugger\Essentials\DpcRoutines.h"
#include "..\hprdbghv\Header Files\Misc\InlineAsm.h"
#include "..\hprdbghv\Header Files\VMM\EPT\Vpid.h"
#include "..\hprdbghv\Header Files\VMM\EPT\Ept.h"
#include "..\hprdbghv\Header Files\VMM\VMX\Events.h"
#include "..\hprdbghv\Header Files\Common\Common.h"
#include "..\hprdbghv\Header Files\Debugger\Essentials\Debugger.h"
#include "..\hprdbghv\Header Files\Devices\Apic.h"
#include "..\hprdbghv\Header Files\Debugger\Essentials\Kd.h"
#include "..\hprdbghv\Header Files\VMM\VMX\Mtf.h"
#include "..\hprdbghv\Header Files\Debugger\Communication\GdbStub.h"
#include "..\hprdbghv\Header Files\Debugger\Essentials\DebuggerEvents.h"
#include "..\hprdbghv\Header Files\Debugger\Features\Hooks.h"
#include "..\hprdbghv\Header Files\VMM\VMX\Counters.h"
#include "..\hprdbghv\Header Files\Debugger\Transparency\Transparency.h"
#include "..\hprdbghv\Header Files\VMM\VMX\IdtEmulation.h"
#include "..\hprdbghv\Header Files\VMM\EPT\Invept.h"
#include "..\hprdbghv\Header Files\Debugger\Essentials\Broadcast.h"
#include "..\hprdbghv\Header Files\VMM\VMX\Vmcall.h"
#include "..\hprdbghv\Header Files\VMM\VMX\ManageRegs.h"
#include "..\hprdbghv\Header Files\VMM\VMX\Vmx.h"
#include "..\hprdbghv\Header Files\Debugger\Commands\BreakpointCommands.h"
#include "..\hprdbghv\Header Files\Debugger\Commands\DebuggerCommands.h"
#include "..\hprdbghv\Header Files\Debugger\Commands\ExtensionCommands.h"
#include "..\hprdbghv\Header Files\Debugger\Communication\SerialConnection.h"
#include "..\hprdbghv\Header Files\VMM\VMX\HypervisorRoutines.h"
#include "..\hprdbghv\Header Files\VMM\VMX\IoHandler.h"
#include "..\hprdbghv\Header Files\Debugger\Essentials\Steppings.h"
#include "..\hprdbghv\Header Files\Debugger\Essentials\Termination.h"
#include "ScriptEngineCommonDefinitions.h"

//
// Global Variables should be the last header to include
//
#include "..\hprdbghv\Header Files\Misc\GlobalVariables.h"
