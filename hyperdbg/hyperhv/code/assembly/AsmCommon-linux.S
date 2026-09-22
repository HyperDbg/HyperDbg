/* SPDX-License-Identifier: GPL-3.0 */
/*
 * AsmCommon-linux.S  --  Linux (GAS) port of AsmCommon.asm
 *
 * STUB ONLY.  Every symbol below is an EMPTY placeholder that just returns, so
 * the module links while the real hypervisor assembly is ported.  The Windows
 * build keeps using AsmCommon.asm (MASM) verbatim -- this file is Kbuild-only and
 * is never added to any .vcxproj.  The "-linux" suffix keeps the GAS port
 * visibly distinct from the MASM original, matching symbol-linux.cpp etc.
 *
 * TODO(Linux): translate the real bodies from AsmCommon.asm (MASM to GAS/AT&T).
 *   - getters currently return garbage in %rax (a bare ret)
 *   - the interrupt-handler and VM-exit entry points will need a proper
 *     SYM_CODE frame plus iretq/vmx handling when implemented; a plain ret
 *     here only satisfies the linker, it is not a working handler.
 */
#include <linux/linkage.h>

	.text

SYM_FUNC_START(AsmCliInstruction)
	ret			/* TODO(Linux): port from AsmCommon.asm */
SYM_FUNC_END(AsmCliInstruction)

SYM_FUNC_START(AsmGetRflags)
	ret			/* TODO(Linux): port from AsmCommon.asm */
SYM_FUNC_END(AsmGetRflags)

SYM_FUNC_START(AsmReadSsp)
	ret			/* TODO(Linux): port from AsmCommon.asm */
SYM_FUNC_END(AsmReadSsp)

SYM_FUNC_START(AsmReloadGdtr)
	ret			/* TODO(Linux): port from AsmCommon.asm */
SYM_FUNC_END(AsmReloadGdtr)

SYM_FUNC_START(AsmReloadIdtr)
	ret			/* TODO(Linux): port from AsmCommon.asm */
SYM_FUNC_END(AsmReloadIdtr)

SYM_FUNC_START(AsmStiInstruction)
	ret			/* TODO(Linux): port from AsmCommon.asm */
SYM_FUNC_END(AsmStiInstruction)
