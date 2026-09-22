/* SPDX-License-Identifier: GPL-3.0 */
/*
 * AsmVmxOperation-linux.S  --  Linux (GAS) port of AsmVmxOperation.asm
 *
 * STUB ONLY.  Every symbol below is an EMPTY placeholder that just returns, so
 * the module links while the real hypervisor assembly is ported.  The Windows
 * build keeps using AsmVmxOperation.asm (MASM) verbatim -- this file is Kbuild-only and
 * is never added to any .vcxproj.  The "-linux" suffix keeps the GAS port
 * visibly distinct from the MASM original, matching symbol-linux.cpp etc.
 *
 * TODO(Linux): translate the real bodies from AsmVmxOperation.asm (MASM to GAS/AT&T).
 *   - getters currently return garbage in %rax (a bare ret)
 *   - the interrupt-handler and VM-exit entry points will need a proper
 *     SYM_CODE frame plus iretq/vmx handling when implemented; a plain ret
 *     here only satisfies the linker, it is not a working handler.
 */
#include <linux/linkage.h>

	.text

SYM_FUNC_START(AsmEnableVmxOperation)
	ret			/* TODO(Linux): port from AsmVmxOperation.asm */
SYM_FUNC_END(AsmEnableVmxOperation)

SYM_FUNC_START(AsmHypervVmcall)
	ret			/* TODO(Linux): port from AsmVmxOperation.asm */
SYM_FUNC_END(AsmHypervVmcall)

SYM_FUNC_START(AsmVmfunc)
	ret			/* TODO(Linux): port from AsmVmxOperation.asm */
SYM_FUNC_END(AsmVmfunc)

SYM_FUNC_START(AsmVmxVmcall)
	ret			/* TODO(Linux): port from AsmVmxOperation.asm */
SYM_FUNC_END(AsmVmxVmcall)
