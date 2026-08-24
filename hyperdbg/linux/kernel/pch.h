/**
 * @file pch.h
 * @brief Unified pre-compiled-header equivalent for the HyperDbg Linux kernel
 *        module (HyperDbg.ko).
 *
 * @details This is the pch for the SHARED translation units only — the platform
 *          layer, components/, and linux/kernel/Entry.c — which belong to no
 *          module and so have no pch of their own on Windows either.
 *
 *          Every module TU (hyperkd/, hyperhv/, hyperlog/, script-eval/) keeps
 *          resolving `#include "pch.h"` to its OWN module pch, via the
 *          per-object include roots at the bottom of the root Kbuild. That is
 *          required, not stylistic: hyperkd and hyperhv each define
 *          _NT_KPROCESS and _PAGE_ENTRY, so no single pch can serve both.
 *
 *          The Linux kernel headers and scope macros are force-included ahead
 *          of everything by linux/kernel/LinuxPrelude.h.
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#pragma once


//
// Definition of Intel primitives (external header, kept ahead of the SDK the
// same way every Windows kernel pch orders it)
//
#include "ia32-doc/out/ia32.h"

//
// HyperDbg SDK + environment detection
//
#include "SDK/HyperDbgSdk.h"
#include "platform/general/header/Environment.h"
#include "config/Configuration.h"
#include "config/Definition.h"
#include "macros/MetaMacros.h"

//
// hyperlog message tracing: the exported LogCallback* prototypes plus the
// Log/LogInfo/LogError macros that expand to them. On Windows each project's
// own pch.h pulls these in; here they serve every TU of the single module.
//
#include "SDK/modules/HyperLog.h"
#include "SDK/imports/kernel/HyperDbgHyperLogImports.h"
#include "SDK/imports/kernel/HyperDbgHyperLogIntrinsics.h"

//
// The other modules' exported interfaces (VmFunc*, HyperTrace*). Declarations
// only — the defining TUs are not in the module yet, so anything that actually
// calls these stays commented out in Kbuild until hyperhv/hypertrace land.
//
#include "SDK/modules/VMM.h"
#include "SDK/imports/kernel/HyperDbgVmmImports.h"
#include "SDK/modules/HyperTrace.h"
#include "SDK/imports/kernel/HyperDbgHyperTrace.h"

//
// Platform abstraction (only the pieces that compile as kernel code today;
// add more headers here as their Platform*.c wrappers come online)
//
#include "platform/kernel/header/PlatformMem.h"
#include "platform/kernel/header/PlatformIntrinsics.h"
#include "platform/kernel/header/PlatformIntrinsicsVmx.h"
#include "platform/kernel/header/PlatformCpu.h"
#include "platform/kernel/header/PlatformDbg.h"
#include "platform/kernel/header/PlatformDpc.h"
#include "platform/kernel/header/PlatformEvent.h"
#include "platform/kernel/header/PlatformIo.h"
#include "platform/kernel/header/PlatformIrql.h"
#include "platform/kernel/header/PlatformProcess.h"
#include "platform/kernel/header/PlatformSpinlock.h"
#include "platform/kernel/header/PlatformStr.h"
#include "platform/kernel/header/PlatformTime.h"

//
// Shared components (declaration-only; module-specific state headers such as
// hyperlog's Logging.h are included by their own .c, not here, so their
// header-defined globals are not duplicated into every TU).
//
#include "components/spinlock/header/Spinlock.h"
#include "components/optimizations/header/AvlTree.h"
#include "components/optimizations/header/BinarySearch.h"
#include "components/optimizations/header/InsertionSort.h"


