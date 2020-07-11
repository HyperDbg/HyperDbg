/**
 * @file InlineAsm.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The definition of functions written in Assembly
 * @details 
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#pragma once

//
// ====================  Vmx Operations ====================
// File : AsmVmxOperation.asm
//
extern void inline AsmEnableVmxOperation();
extern void inline AsmRestoreToVmxOffState();
extern NTSTATUS inline AsmVmxVmcall(unsigned long long VmcallNumber, unsigned long long OptionalParam1, unsigned long long OptionalParam2, long long OptionalParam3);
extern UINT64 inline AsmHypervVmcall(unsigned long long HypercallInputValue, unsigned long long InputParamGPA, unsigned long long OutputParamGPA, unsigned long long Optional4);

//
// ====================  Vmx Context State Operations ====================
// File : AsmVmxContextState.asm
//
extern void
AsmVmxSaveState();
extern void
AsmVmxRestoreState();

//
// ====================  Vmx VM-Exit Handler ====================
// File : AsmVmexitHandler.asm
//
extern void
AsmVmexitHandler();
extern void inline AsmSaveVmxOffState();

//
// ====================  Extended Page Tables ====================
// File : AsmEpt.asm
//
extern unsigned char inline AsmInvept(unsigned long Type, void * Descriptors);
extern unsigned char inline AsmInvvpid(unsigned long Type, void * Descriptors);

//
// ====================  Get segment registers ====================
// File : AsmSegmentRegs.asm
//

/* Segment registers */
extern unsigned short
AsmGetCs();
extern unsigned short
AsmGetDs();
extern unsigned short
AsmGetEs();
extern unsigned short
AsmGetSs();
extern unsigned short
AsmGetFs();
extern unsigned short
AsmGetGs();
extern unsigned short
AsmGetLdtr();
extern unsigned short
AsmGetTr();

/* Gdt related functions */
extern unsigned long long inline AsmGetGdtBase();
extern unsigned short
AsmGetGdtLimit();

/* Idt related functions */
extern unsigned long long inline AsmGetIdtBase();
extern unsigned short
AsmGetIdtLimit();

//
// ====================  Common Functions ====================
// File : AsmCommon.asm
//
extern unsigned short
AsmGetRflags();
extern void inline AsmCliInstruction();
extern void inline AsmStiInstruction();

extern void
AsmReloadGdtr(void * GdtBase, unsigned long GdtLimit);
extern void
AsmReloadIdtr(void * GdtBase, unsigned long GdtLimit);

//
// ====================  Debugger Functions ====================
// File : AsmDebugger.asm
//
extern void
AsmGeneralDetourHook();
extern unsigned long long
AsmDebuggerCustomCodeHandler(unsigned long long Param1, unsigned long long Param2, unsigned long long Param3, unsigned long long Param4);
extern unsigned long long
AsmDebuggerConditionCodeHandler(unsigned long long Param1, unsigned long long Param2, unsigned long long Param3);
