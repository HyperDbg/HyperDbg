/**
 * @file InlineAsm.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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
extern NTSTATUS inline AsmVmxVmcall(unsigned long long VmcallNumber,
                                    unsigned long long OptionalParam1,
                                    unsigned long long OptionalParam2,
                                    long long          OptionalParam3);

/**
 * @brief Hyper-v vmcall handler
 *
 * @param GuestRegisters
 * @return void
 */
extern void inline AsmHypervVmcall(unsigned long long GuestRegisters);

/**
 * @brief VMFUNC instruction
 *
 * @param EptpIndex
 * @param Function
 *
 * @return unsigned long long I'm not sure what it returns
 */
extern unsigned long long inline AsmVmfunc(unsigned long EptpIndex, unsigned long Function);

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

/**
 * @brief Restore XMM registers
 *
 */
extern void inline AsmVmxoffRestoreXmmRegs(unsigned long long XmmRegs);

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

extern void
AsmSetDs(unsigned short DsSelector);

/**
 * @brief Get ES Register
 *
 * @return unsigned short
 */
extern unsigned short
AsmGetEs();

extern void
AsmSetEs(unsigned short EsSelector);

/**
 * @brief Get SS Register
 *
 * @return unsigned short
 */
extern unsigned short
AsmGetSs();

extern void
AsmSetSs(unsigned short SsSelector);

/**
 * @brief Get FS Register
 *
 * @return unsigned short
 */
extern unsigned short
AsmGetFs();

extern void
AsmSetFs(unsigned short FsSelector);

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

extern UINT32
AsmGetAccessRights(unsigned short Selector);
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

/**
 * @brief Read SSP
 *
 */
extern unsigned long long
AsmReadSsp();

//
// ====================  Hook Functions ====================
// File : AsmHooks.asm
//

/**
 * @brief Detour hook handler
 *
 */
extern void
AsmGeneralDetourHook(void);

//
// ====================  Kernel Test Functions ====================
// File : AsmKernelSideTests.asm
//

/**
 * @brief Tests with test tags wrapper
 *
 */
extern unsigned long long
AsmTestWrapperWithTestTags(unsigned long long Param1,
                           unsigned long long Param2,
                           unsigned long long Param3,
                           unsigned long long Param4);

//
// ====================  Interrupt Handler Functions ====================
// File : Interrupt Handlers.asm.asm
//

/**
 * @brief The 0th entry in IDT
 *
 */
extern void
InterruptHandler0();

/**
 * @brief The 1st entry in IDT
 *
 */
extern void
InterruptHandler1();

/**
 * @brief The 2nd entry in IDT
 *
 */
extern void
InterruptHandler2();

/**
 * @brief The 3rd entry in IDT
 *
 */
extern void
InterruptHandler3();

/**
 * @brief The 4th entry in IDT
 *
 */
extern void
InterruptHandler4();

/**
 * @brief The 5th entry in IDT
 *
 */
extern void
InterruptHandler5();

/**
 * @brief The 6th entry in IDT
 *
 */
extern void
InterruptHandler6();

/**
 * @brief The 7th entry in IDT
 *
 */
extern void
InterruptHandler7();

/**
 * @brief The 8th entry in IDT
 *
 */
extern void
InterruptHandler8();

/**
 * @brief The 9th entry in IDT
 *
 */
extern void
InterruptHandler9();

/**
 * @brief The 10th entry in IDT
 *
 */
extern void
InterruptHandler10();

/**
 * @brief The 11th entry in IDT
 *
 */
extern void
InterruptHandler11();

/**
 * @brief The 12th entry in IDT
 *
 */
extern void
InterruptHandler12();

/**
 * @brief The 13th entry in IDT
 *
 */
extern void
InterruptHandler13();

/**
 * @brief The 14th entry in IDT
 *
 */
extern void
InterruptHandler14();

/**
 * @brief The 15th entry in IDT
 *
 */
extern void
InterruptHandler15();

/**
 * @brief The 16th entry in IDT
 *
 */
extern void
InterruptHandler16();

/**
 * @brief The 17th entry in IDT
 *
 */
extern void
InterruptHandler17();

/**
 * @brief The 18th entry in IDT
 *
 */
extern void
InterruptHandler18();

/**
 * @brief The 19th entry in IDT
 *
 */
extern void
InterruptHandler19();

/**
 * @brief The 20th entry in IDT
 *
 */
extern void
InterruptHandler20();

/**
 * @brief The 21st entry in IDT
 *
 */
extern void
InterruptHandler21();

/**
 * @brief The 22nd entry in IDT
 *
 */
extern void
InterruptHandler22();

/**
 * @brief The 23rd entry in IDT
 *
 */
extern void
InterruptHandler23();

/**
 * @brief The 24th entry in IDT
 *
 */
extern void
InterruptHandler24();

/**
 * @brief The 25th entry in IDT
 *
 */
extern void
InterruptHandler25();

/**
 * @brief The 26th entry in IDT
 *
 */
extern void
InterruptHandler26();

/**
 * @brief The 27th entry in IDT
 *
 */
extern void
InterruptHandler27();

/**
 * @brief The 28th entry in IDT
 *
 */
extern void
InterruptHandler28();

/**
 * @brief The 29th entry in IDT
 *
 */
extern void
InterruptHandler29();

/**
 * @brief The 30th entry in IDT
 *
 */
extern void
InterruptHandler30();
