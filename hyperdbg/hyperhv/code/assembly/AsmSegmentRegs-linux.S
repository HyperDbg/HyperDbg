/* SPDX-License-Identifier: GPL-3.0 */
/*
 * AsmSegmentRegs-linux.S  --  Linux (GAS) port of AsmSegmentRegs.asm
 *
 * STUB ONLY.  Every symbol below is an EMPTY placeholder that just returns, so
 * the module links while the real hypervisor assembly is ported.  The Windows
 * build keeps using AsmSegmentRegs.asm (MASM) verbatim -- this file is Kbuild-only and
 * is never added to any .vcxproj.  The "-linux" suffix keeps the GAS port
 * visibly distinct from the MASM original, matching symbol-linux.cpp etc.
 *
 * TODO(Linux): translate the real bodies from AsmSegmentRegs.asm (MASM to GAS/AT&T).
 *   - getters currently return garbage in %rax (a bare ret)
 *   - the interrupt-handler and VM-exit entry points will need a proper
 *     SYM_CODE frame plus iretq/vmx handling when implemented; a plain ret
 *     here only satisfies the linker, it is not a working handler.
 */
#include <linux/linkage.h>

	.text

SYM_FUNC_START(AsmGetAccessRights)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetAccessRights)

SYM_FUNC_START(AsmGetCs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetCs)

SYM_FUNC_START(AsmGetDs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetDs)

SYM_FUNC_START(AsmGetEs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetEs)

SYM_FUNC_START(AsmGetFs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetFs)

SYM_FUNC_START(AsmGetGdtBase)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetGdtBase)

SYM_FUNC_START(AsmGetGdtLimit)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetGdtLimit)

SYM_FUNC_START(AsmGetGs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetGs)

SYM_FUNC_START(AsmGetIdtBase)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetIdtBase)

SYM_FUNC_START(AsmGetIdtLimit)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetIdtLimit)

SYM_FUNC_START(AsmGetLdtr)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetLdtr)

SYM_FUNC_START(AsmGetSs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetSs)

SYM_FUNC_START(AsmGetTr)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmGetTr)

SYM_FUNC_START(AsmSetDs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmSetDs)

SYM_FUNC_START(AsmSetEs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmSetEs)

SYM_FUNC_START(AsmSetFs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmSetFs)

SYM_FUNC_START(AsmSetSs)
	ret			/* TODO(Linux): port from AsmSegmentRegs.asm */
SYM_FUNC_END(AsmSetSs)
