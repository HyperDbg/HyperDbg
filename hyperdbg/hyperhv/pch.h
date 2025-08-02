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

#pragma warning(disable : 4201) // Suppress nameless struct/union warning

//
// Scope definitions
//
#define SCRIPT_ENGINE_KERNEL_MODE
#define HYPERDBG_KERNEL_MODE
#define HYPERDBG_VMM

//
// Environment headers
//
#include "platform/kernel/header/Environment.h"

#ifdef ENV_WINDOWS

//
// General WDK headers
//
#    include <ntifs.h>
#    include <ntstrsafe.h>
#    include <Windef.h>
#    include <assert.h>

#endif // ENV_WINDOWS

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
#include "config/Configuration.h"
#include "macros/MetaMacros.h"

//
// Platform independent headers
//
#include "platform/kernel/header/Mem.h"

//
// VMM Callbacks
//
#include "SDK/modules/VMM.h"

//
// The core's state
//
#include "common/State.h"

//
// VMX and EPT Types
//
#include "vmm/vmx/Vmx.h"
#include "vmm/vmx/VmxRegions.h"
#include "vmm/ept/Ept.h"
#include "SDK/imports/kernel/HyperDbgVmmImports.h"

//
// Hyper-V TLFS
//
#include "hyper-v/HypervTlfs.h"

//
// VMX and Capabilities
//
#include "vmm/vmx/VmxBroadcast.h"
#include "memory/MemoryMapper.h"
#include "interface/Dispatch.h"
#include "common/Dpc.h"
#include "common/Msr.h"
#include "memory/PoolManager.h"
#include "common/Trace.h"
#include "assembly/InlineAsm.h"
#include "vmm/ept/Vpid.h"
#include "memory/Conversion.h"
#include "memory/Layout.h"
#include "memory/SwitchLayout.h"
#include "memory/AddressCheck.h"
#include "memory/Segmentation.h"
#include "common/Bitwise.h"
#include "common/Common.h"
#include "vmm/vmx/Events.h"
#include "devices/Apic.h"
#include "devices/Pci.h"
#include "processor/Smm.h"
#include "processor/Idt.h"
#include "vmm/vmx/Mtf.h"
#include "vmm/vmx/Counters.h"
#include "vmm/vmx/IdtEmulation.h"
#include "vmm/ept/Invept.h"
#include "vmm/vmx/Vmcall.h"
#include "interface/DirectVmcall.h"
#include "vmm/vmx/Hv.h"
#include "vmm/vmx/MsrHandlers.h"
#include "vmm/vmx/ProtectedHv.h"
#include "vmm/vmx/IoHandler.h"
#include "vmm/vmx/VmxMechanisms.h"
#include "hooks/Hooks.h"
#include "hooks/ModeBasedExecHook.h"
#include "hooks/SyscallCallback.h"
#include "interface/Callback.h"
#include "features/DirtyLogging.h"
#include "features/CompatibilityChecks.h"
#include "mmio/MmioShadowing.h"

//
// Disassembler Header
//
#include "Zydis/Zydis.h"
#include "disassembler/Disassembler.h"

//
// Broadcast headers
//
#include "broadcast/Broadcast.h"
#include "broadcast/DpcRoutines.h"

//
// Headers for supporting the reversing machine (TRM)
//
#include "hooks/ExecTrap.h"

//
// Headers for exporting functions to remove the driver
//
#include "common/UnloadDll.h"

//
// Optimization algorithms
//
#include "components/optimizations/header/AvlTree.h"
#include "components/optimizations/header/BinarySearch.h"
#include "components/optimizations/header/InsertionSort.h"

//
// Spinlocks
//
#include "components/spinlock/header/Spinlock.h"

//
// Global Variables should be the last header to include
//
#include "globals/GlobalVariableManagement.h"
#include "globals/GlobalVariables.h"

//
// HyperLog Module
//
#include "SDK/modules/HyperLog.h"
#include "SDK/imports/kernel/HyperDbgHyperLogIntrinsics.h"
#include "components/interface/HyperLogCallback.h"

//
// Transparent-mode (hyperevade) headers
//
#include "SDK/modules/HyperEvade.h"
#include "SDK/imports/kernel/HyperDbgHyperEvade.h"
