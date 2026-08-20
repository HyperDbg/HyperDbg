/* SPDX-License-Identifier: GPL-3.0 */
/*
 * AsmVmexitHandler-linux.S  --  Linux (GAS) port of AsmVmexitHandler.asm
 *
 * STUB ONLY.  Every symbol below is an EMPTY placeholder that just returns, so
 * the module links while the real hypervisor assembly is ported.  The Windows
 * build keeps using AsmVmexitHandler.asm (MASM) verbatim -- this file is Kbuild-only and
 * is never added to any .vcxproj.  The "-linux" suffix keeps the GAS port
 * visibly distinct from the MASM original, matching symbol-linux.cpp etc.
 *
 * TODO(Linux): translate the real bodies from AsmVmexitHandler.asm (MASM to GAS/AT&T).
 *   - getters currently return garbage in %rax (a bare ret)
 *   - the interrupt-handler and VM-exit entry points will need a proper
 *     SYM_CODE frame plus iretq/vmx handling when implemented; a plain ret
 *     here only satisfies the linker, it is not a working handler.
 */
#include <linux/linkage.h>

	.text

SYM_FUNC_START(AsmVmexitHandler)
	ret			/* TODO(Linux): port from AsmVmexitHandler.asm */
SYM_FUNC_END(AsmVmexitHandler)

SYM_FUNC_START(AsmVmxoffHandler)
	ret			/* TODO(Linux): port from AsmVmexitHandler.asm */
SYM_FUNC_END(AsmVmxoffHandler)

SYM_FUNC_START(AsmVmxoffRestoreXmmRegs)
	ret			/* TODO(Linux): port from AsmVmexitHandler.asm */
SYM_FUNC_END(AsmVmxoffRestoreXmmRegs)
