/* SPDX-License-Identifier: GPL-3.0 */
/*
 * AsmDebugger-linux.S  --  Linux (GAS) port of AsmDebugger.asm
 *
 * STUB ONLY.  Every symbol below is an EMPTY placeholder that just returns, so
 * the module links while the real debugger assembly is ported.  The Windows
 * build keeps using AsmDebugger.asm (MASM) verbatim -- this file is Kbuild-only
 * and is never added to any .vcxproj.  The "-linux" suffix keeps the GAS port
 * visibly distinct from the MASM original, matching the hyperhv Asm*-linux.S
 * stubs and symbol-linux.cpp etc.
 *
 * TODO(Linux): translate the real bodies from AsmDebugger.asm (MASM to GAS/AT&T).
 *   - AsmDebuggerCustomCodeHandler / AsmDebuggerConditionCodeHandler dispatch to
 *     a user-supplied code buffer and marshal 3-4 args; a bare ret only satisfies
 *     the linker and returns garbage in %rax.
 *   - AsmDebuggerSpinOnThread is the halted-core spin loop; the stub returns
 *     immediately instead of spinning.
 */
#include <linux/linkage.h>

	.text

SYM_FUNC_START(AsmDebuggerCustomCodeHandler)
	ret			/* TODO(Linux): port from AsmDebugger.asm */
SYM_FUNC_END(AsmDebuggerCustomCodeHandler)

SYM_FUNC_START(AsmDebuggerConditionCodeHandler)
	ret			/* TODO(Linux): port from AsmDebugger.asm */
SYM_FUNC_END(AsmDebuggerConditionCodeHandler)

SYM_FUNC_START(AsmDebuggerSpinOnThread)
	ret			/* TODO(Linux): port from AsmDebugger.asm */
SYM_FUNC_END(AsmDebuggerSpinOnThread)
