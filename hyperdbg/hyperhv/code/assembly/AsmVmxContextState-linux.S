/* SPDX-License-Identifier: GPL-3.0 */
/*
 * AsmVmxContextState-linux.S  --  Linux (GAS) port of AsmVmxContextState.asm
 *
 * STUB ONLY.  Every symbol below is an EMPTY placeholder that just returns, so
 * the module links while the real hypervisor assembly is ported.  The Windows
 * build keeps using AsmVmxContextState.asm (MASM) verbatim -- this file is Kbuild-only and
 * is never added to any .vcxproj.  The "-linux" suffix keeps the GAS port
 * visibly distinct from the MASM original, matching symbol-linux.cpp etc.
 *
 * TODO(Linux): translate the real bodies from AsmVmxContextState.asm (MASM to GAS/AT&T).
 *   - getters currently return garbage in %rax (a bare ret)
 *   - the interrupt-handler and VM-exit entry points will need a proper
 *     SYM_CODE frame plus iretq/vmx handling when implemented; a plain ret
 *     here only satisfies the linker, it is not a working handler.
 */
#include <linux/linkage.h>

	.text

SYM_FUNC_START(AsmVmxRestoreState)
	ret			/* TODO(Linux): port from AsmVmxContextState.asm */
SYM_FUNC_END(AsmVmxRestoreState)

SYM_FUNC_START(AsmVmxSaveState)
	ret			/* TODO(Linux): port from AsmVmxContextState.asm */
SYM_FUNC_END(AsmVmxSaveState)
