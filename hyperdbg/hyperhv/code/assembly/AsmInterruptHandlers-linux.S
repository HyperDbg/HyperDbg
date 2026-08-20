/* SPDX-License-Identifier: GPL-3.0 */
/*
 * AsmInterruptHandlers-linux.S  --  Linux (GAS) port of AsmInterruptHandlers.asm
 *
 * STUB ONLY.  Every symbol below is an EMPTY placeholder that just returns, so
 * the module links while the real hypervisor assembly is ported.  The Windows
 * build keeps using AsmInterruptHandlers.asm (MASM) verbatim -- this file is Kbuild-only and
 * is never added to any .vcxproj.  The "-linux" suffix keeps the GAS port
 * visibly distinct from the MASM original, matching symbol-linux.cpp etc.
 *
 * TODO(Linux): translate the real bodies from AsmInterruptHandlers.asm (MASM to GAS/AT&T).
 *   - getters currently return garbage in %rax (a bare ret)
 *   - the interrupt-handler and VM-exit entry points will need a proper
 *     SYM_CODE frame plus iretq/vmx handling when implemented; a plain ret
 *     here only satisfies the linker, it is not a working handler.
 */
#include <linux/linkage.h>

	.text

SYM_FUNC_START(GenericInterruptHandler)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(GenericInterruptHandler)

SYM_FUNC_START(InterruptHandler0)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler0)

SYM_FUNC_START(InterruptHandler1)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler1)

SYM_FUNC_START(InterruptHandler2)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler2)

SYM_FUNC_START(InterruptHandler3)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler3)

SYM_FUNC_START(InterruptHandler4)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler4)

SYM_FUNC_START(InterruptHandler5)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler5)

SYM_FUNC_START(InterruptHandler6)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler6)

SYM_FUNC_START(InterruptHandler7)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler7)

SYM_FUNC_START(InterruptHandler8)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler8)

SYM_FUNC_START(InterruptHandler9)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler9)

SYM_FUNC_START(InterruptHandler10)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler10)

SYM_FUNC_START(InterruptHandler11)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler11)

SYM_FUNC_START(InterruptHandler12)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler12)

SYM_FUNC_START(InterruptHandler13)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler13)

SYM_FUNC_START(InterruptHandler14)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler14)

SYM_FUNC_START(InterruptHandler15)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler15)

SYM_FUNC_START(InterruptHandler16)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler16)

SYM_FUNC_START(InterruptHandler17)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler17)

SYM_FUNC_START(InterruptHandler18)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler18)

SYM_FUNC_START(InterruptHandler19)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler19)

SYM_FUNC_START(InterruptHandler20)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler20)

SYM_FUNC_START(InterruptHandler21)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler21)

SYM_FUNC_START(InterruptHandler22)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler22)

SYM_FUNC_START(InterruptHandler23)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler23)

SYM_FUNC_START(InterruptHandler24)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler24)

SYM_FUNC_START(InterruptHandler25)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler25)

SYM_FUNC_START(InterruptHandler26)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler26)

SYM_FUNC_START(InterruptHandler27)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler27)

SYM_FUNC_START(InterruptHandler28)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler28)

SYM_FUNC_START(InterruptHandler29)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler29)

SYM_FUNC_START(InterruptHandler30)
	ret			/* TODO(Linux): port from AsmInterruptHandlers.asm */
SYM_FUNC_END(InterruptHandler30)
