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
#include "Definition.h"
#include "Configuration.h"
#include "platform/CrossApi.h"
#include "platform/Environment.h"
#include "platform/MetaMacros.h"

//
// VMM Callbacks
//
#include "SDK/Modules/VMM.h"

//
// The core's state
//
#include "header/common/State.h"

//
// VMX and EPT Types
//
#include "header/vmm/vmx/Vmx.h"
#include "header/vmm/ept/Ept.h"
#include "SDK/Imports/HyperDbgVmmImports.h"

//
// VMX and Capabilities
//
#include "transparency/Transparency.h"
#include "vmm/vmx/VmxBroadcast.h"
#include "memory/MemoryMapper.h"
#include "interface/Dispatch.h"
#include "common/Dpc.h"
#include "common/LengthDisassemblerEngine.h"
#include "vmm/vmx/HypervTlfs.h"
#include "common/Msr.h"
#include "memory/PoolManager.h"
#include "common/Trace.h"
#include "misc/InlineAsm.h"
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
#include "vmm/vmx/ManageRegs.h"
#include "components/registers/DebugRegisters.h"
#include "vmm/vmx/Hv.h"
#include "vmm/vmx/MsrHandlers.h"
#include "vmm/vmx/ProtectedHv.h"
#include "vmm/vmx/IoHandler.h"
#include "vmm/vmx/VmxMechanisms.h"
#include "header/hooks/Hooks.h"

//
// Broadcast headers
//
#include "header/broadcast/Broadcast.h"
#include "header/broadcast/DpcRoutines.h"

//
// Global Variables should be the last header to include
//
#include "header/globals/GlobalVariableManagement.h"
#include "header/globals/GlobalVariables.h"

//
// Define callback prefix for hyperlog
//
#define HYPERLOG_PREFIX g_Callbacks

//
// HyperLog Module
//
#include "SDK/Modules/HyperLog.h"
#include "SDK/Imports/HyperDbgHyperLogIntrinsics.h"
