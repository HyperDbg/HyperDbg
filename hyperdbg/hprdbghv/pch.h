/**
 * @file pch.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Pre-compiled headers 
 * @details 
 *
 * @version 0.1
 * @date 2020-06-01
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//
// Windows defined functions
//
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
#include "Dpc.h"
#include "LengthDisassemblerEngine.h"
#include "Logging.h"
#include "MemoryMapper.h"
#include "Msr.h"
#include "PoolManager.h"
#include "Trace.h"
#include "DpcRoutines.h"
#include "Transparency.h"
#include "InlineAsm.h"
#include "Vpid.h"
#include "Ept.h"
#include "Events.h"
#include "Common.h"
#include "Debugger.h"
#include "DebuggerEvents.h"
#include "Hooks.h"
#include "Counters.h"
#include "IdtEmulation.h"
#include "Invept.h"
#include "Broadcast.h"
#include "Vmcall.h"
#include "Vmx.h"
#include "DebuggerCommands.h"
#include "ExtensionCommands.h"
#include "HypervisorRoutines.h"
#include "Ntifs2.h"
#include "IoHandler.h"
#include "GlobalVariables.h"
