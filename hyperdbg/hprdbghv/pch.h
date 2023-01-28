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
#define HYPERDBG_VMM

//
// Definition of Intel primitives (External header)
//
#include "ia32-doc/out/ia32.h"

//
// HyperDbg SDK headers
//
#include "SDK/HyperDbgSdk.h"

//
// HyperDbg Kernel-mode headers
//
#include "Configuration.h"
#include "macros/MetaMacros.h"
#include "platform/CrossApi.h"
#include "platform/Environment.h"

//
// VMM Callbacks
//
#include "SDK/Modules/VMM.h"

//
// The core's state
//
#include "common/State.h"

//
// VMX and EPT Types
//
#include "vmm/vmx/Vmx.h"
#include "vmm/ept/Ept.h"
#include "SDK/Imports/HyperDbgVmmImports.h"

//
// Disassembler Engine
//
#include "components/disasm/LengthDisassemblerEngine.h"

//
// VMX and Capabilities
//
#include "transparency/Transparency.h"
#include "vmm/vmx/VmxBroadcast.h"
#include "memory/MemoryMapper.h"
#include "interface/Dispatch.h"
#include "interface/Configuration.h"
#include "common/Dpc.h"
#include "vmm/vmx/HypervTlfs.h"
#include "common/Msr.h"
#include "memory/PoolManager.h"
#include "common/Trace.h"
#include "assembly/InlineAsm.h"
#include "vmm/ept/Vpid.h"
#include "common/Common.h"
#include "components/spinlock/header/Spinlock.h"
#include "vmm/vmx/Events.h"
#include "devices/Apic.h"
#include "vmm/vmx/Mtf.h"
#include "vmm/vmx/Counters.h"
#include "vmm/vmx/IdtEmulation.h"
#include "vmm/ept/Invept.h"
#include "vmm/vmx/Vmcall.h"
#include "vmm/vmx/Hv.h"
#include "vmm/vmx/MsrHandlers.h"
#include "vmm/vmx/ProtectedHv.h"
#include "vmm/vmx/IoHandler.h"
#include "vmm/vmx/VmxMechanisms.h"
#include "hooks/Hooks.h"
#include "interface/Callback.h"

//
// Broadcast headers
//
#include "broadcast/Broadcast.h"
#include "broadcast/DpcRoutines.h"

//
// Global Variables should be the last header to include
//
#include "globals/GlobalVariableManagement.h"
#include "globals/GlobalVariables.h"

//
// Define callback prefix for hyperlog
//
#define HYPERLOG_PREFIX g_Callbacks

//
// HyperLog Module
//
#include "SDK/Modules/HyperLog.h"
#include "SDK/Imports/HyperDbgHyperLogIntrinsics.h"
