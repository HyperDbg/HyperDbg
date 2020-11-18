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

/**
 * @brief Enable VMX Operation
 * 
 */
extern void inline AsmEnableVmxOperation();

/**
 * @brief Restore in vmxoff state
 * 
 */
extern void inline AsmRestoreToVmxOffState();

/**
 * @brief Request Vmcall
 * 
 * @param VmcallNumber 
 * @param OptionalParam1 
 * @param OptionalParam2 
 * @param OptionalParam3 
 * @return NTSTATUS 
 */
extern NTSTATUS inline AsmVmxVmcall(unsigned long long VmcallNumber, unsigned long long OptionalParam1, unsigned long long OptionalParam2, long long OptionalParam3);

/**
 * @brief Hyper-v vmcall handler
 * 
 * @param HypercallInputValue 
 * @param InputParamGPA 
 * @param OutputParamGPA 
 * @param Optional4 
 * @return UINT64 
 */
extern UINT64 inline AsmHypervVmcall(unsigned long long HypercallInputValue, unsigned long long InputParamGPA, unsigned long long OutputParamGPA, unsigned long long Optional4);

//
// ====================  Vmx Context State Operations ====================
// File : AsmVmxContextState.asm
//

/**
 * @brief Save state on vmx
 * 
 */
extern void
AsmVmxSaveState();

/**
 * @brief Restore state on vmx
 * 
 */
extern void
AsmVmxRestoreState();

//
// ====================  Vmx VM-Exit Handler ====================
// File : AsmVmexitHandler.asm
//

/**
 * @brief Vm-exit handler
 * 
 */
extern void
AsmVmexitHandler();

/**
 * @brief Save vmxoff state
 * 
 */
extern void inline AsmSaveVmxOffState();

//
// ====================  Extended Page Tables ====================
// File : AsmEpt.asm
//

/**
 * @brief INVEPT wrapper
 * 
 * @param Type 
 * @param Descriptors 
 * @return unsigned char 
 */
extern unsigned char inline AsmInvept(unsigned long Type, void * Descriptors);

/**
 * @brief INVVPID wrapper
 * 
 * @param Type 
 * @param Descriptors 
 * @return unsigned char 
 */
extern unsigned char inline AsmInvvpid(unsigned long Type, void * Descriptors);

//
// ====================  Get segment registers ====================
// File : AsmSegmentRegs.asm
//

/* ********* Segment registers ********* */

/**
 * @brief Get CS Register
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetCs();

/**
 * @brief Get DS Register
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetDs();

/**
 * @brief Get ES Register
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetEs();

/**
 * @brief Get SS Register
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetSs();

/**
 * @brief Get FS Register
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetFs();

/**
 * @brief Get GS Register
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetGs();

/**
 * @brief Get LDTR Register
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetLdtr();

/**
 * @brief Get TR Register
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetTr();

/* ******* Gdt related functions ******* */

/**
 * @brief get GDT base
 * 
 * @return unsigned long long 
 */
extern unsigned long long inline AsmGetGdtBase();

/**
 * @brief Get GDT Limit
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetGdtLimit();

/* ******* Idt related functions ******* */

/**
 * @brief Get IDT base
 * 
 * @return unsigned long long 
 */
extern unsigned long long inline AsmGetIdtBase();

/**
 * @brief Get IDT limit
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetIdtLimit();

//
// ====================  Common Functions ====================
// File : AsmCommon.asm
//

/**
 * @brief Get R/EFLAGS
 * 
 * @return unsigned short 
 */
extern unsigned short
AsmGetRflags();

/**
 * @brief Run CLI Instruction
 * 
 */
extern void inline AsmCliInstruction();

/**
 * @brief Run STI Instruction
 * 
 */
extern void inline AsmStiInstruction();

/**
 * @brief Reload new GDTR
 * 
 * @param GdtBase 
 * @param GdtLimit 
 */
extern void
AsmReloadGdtr(void * GdtBase, unsigned long GdtLimit);

/**
 * @brief Reload new IDTR
 * 
 * @param GdtBase 
 * @param GdtLimit 
 */
extern void
AsmReloadIdtr(void * GdtBase, unsigned long GdtLimit);

//
// ====================  Debugger Functions ====================
// File : AsmDebugger.asm
//

/**
 * @brief Detour hook handler
 * 
 */
extern void
AsmGeneralDetourHook();

/**
 * @brief default custom code handler for debugger
 * 
 * @param Param1 
 * @param Param2 
 * @param Param3 
 * @param Param4 
 * @return unsigned long long 
 */
extern void
AsmDebuggerCustomCodeHandler(unsigned long long Param1, unsigned long long Param2, unsigned long long Param3, unsigned long long Param4);

/**
 * @brief default condition code handler
 * 
 * @param Param1 
 * @param Param2 
 * @param Param3 
 * @return unsigned long long 
 */
extern unsigned long long
AsmDebuggerConditionCodeHandler(unsigned long long Param1, unsigned long long Param2, unsigned long long Param3);

/**
 * @brief Nop loop spin to halt the thread and wait
 * 
 */
extern void
AsmDebuggerSpinOnThread();