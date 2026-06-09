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
extern VOID inline AsmEnableVmxOperation();

/**
 * @brief Restore in vmxoff state
 *
 */
extern VOID inline AsmRestoreToVmxOffState();

/**
 * @brief Request Vmcall
 *
 * @param VmcallNumber
 * @param OptionalParam1
 * @param OptionalParam2
 * @param OptionalParam3
 * @return NTSTATUS
 */
extern NTSTATUS inline AsmVmxVmcall(UINT64 VmcallNumber,
                                    UINT64 OptionalParam1,
                                    UINT64 OptionalParam2,
                                    UINT64 OptionalParam3);

/**
 * @brief Hyper-v vmcall handler
 *
 * @param GuestRegisters
 * @return VOID
 */
extern VOID inline AsmHypervVmcall(UINT64 GuestRegisters);

/**
 * @brief VMFUNC instruction
 *
 * @param EptpIndex
 * @param Function
 *
 * @return UINT64 I'm not sure what it returns
 */
extern UINT64 inline AsmVmfunc(ULONG EptpIndex, ULONG Function);

//
// ====================  Vmx Context State Operations ====================
// File : AsmVmxContextState.asm
//

/**
 * @brief Save state on vmx
 *
 */
extern VOID
AsmVmxSaveState();

/**
 * @brief Restore state on vmx
 *
 */
extern VOID
AsmVmxRestoreState();

//
// ====================  Vmx VM-Exit Handler ====================
// File : AsmVmexitHandler.asm
//

/**
 * @brief Vm-exit handler
 *
 */
extern VOID
AsmVmexitHandler();

/**
 * @brief Save vmxoff state
 *
 */
extern VOID inline AsmSaveVmxOffState();

/**
 * @brief Restore XMM registers
 *
 */
extern VOID inline AsmVmxoffRestoreXmmRegs(UINT64 XmmRegs);

//
// ====================  Extended Page Tables ====================
// File : AsmEpt.asm
//

/**
 * @brief INVEPT wrapper
 *
 * @param Type
 * @param Descriptors
 * @return UCHAR
 */
extern UCHAR inline AsmInvept(ULONG Type, PVOID Descriptors);

/**
 * @brief INVVPID wrapper
 *
 * @param Type
 * @param Descriptors
 * @return UCHAR
 */
extern UCHAR inline AsmInvvpid(ULONG Type, PVOID Descriptors);

//
// ====================  Get segment registers ====================
// File : AsmSegmentRegs.asm
//

/* ********* Segment registers ********* */

/**
 * @brief Get CS Register
 *
 * @return UINT16
 */
extern UINT16
AsmGetCs();

/**
 * @brief Get DS Register
 *
 * @return UINT16
 */
extern UINT16
AsmGetDs();

extern VOID
AsmSetDs(UINT16 DsSelector);

/**
 * @brief Get ES Register
 *
 * @return UINT16
 */
extern UINT16
AsmGetEs();

extern VOID
AsmSetEs(UINT16 EsSelector);

/**
 * @brief Get SS Register
 *
 * @return UINT16
 */
extern UINT16
AsmGetSs();

extern VOID
AsmSetSs(UINT16 SsSelector);

/**
 * @brief Get FS Register
 *
 * @return UINT16
 */
extern UINT16
AsmGetFs();

extern VOID
AsmSetFs(UINT16 FsSelector);

/**
 * @brief Get GS Register
 *
 * @return UINT16
 */
extern UINT16
AsmGetGs();

/**
 * @brief Get LDTR Register
 *
 * @return UINT16
 */
extern UINT16
AsmGetLdtr();

/**
 * @brief Get TR Register
 *
 * @return UINT16
 */
extern UINT16
AsmGetTr();

/* ******* Gdt related functions ******* */

/**
 * @brief get GDT base
 *
 * @return UINT64
 */
extern UINT64 inline AsmGetGdtBase();

/**
 * @brief Get GDT Limit
 *
 * @return UINT16
 */
extern UINT16
AsmGetGdtLimit();

/* ******* Idt related functions ******* */

/**
 * @brief Get IDT base
 *
 * @return UINT64
 */
extern UINT64 inline AsmGetIdtBase();

/**
 * @brief Get IDT limit
 *
 * @return UINT16
 */
extern UINT16
AsmGetIdtLimit();

extern UINT32
AsmGetAccessRights(UINT16 Selector);
//
// ====================  Common Functions ====================
// File : AsmCommon.asm
//

/**
 * @brief Get R/EFLAGS
 *
 * @return UINT16
 */
extern UINT16
AsmGetRflags();

/**
 * @brief Run CLI Instruction
 *
 */
extern VOID inline AsmCliInstruction();

/**
 * @brief Run STI Instruction
 *
 */
extern VOID inline AsmStiInstruction();

/**
 * @brief Reload new GDTR
 *
 * @param GdtBase
 * @param GdtLimit
 */
extern VOID
AsmReloadGdtr(PVOID GdtBase, ULONG GdtLimit);

/**
 * @brief Reload new IDTR
 *
 * @param IdtrBase
 * @param IdtrLimit
 */
extern VOID
AsmReloadIdtr(PVOID IdtrBase, ULONG IdtrLimit);

/**
 * @brief Read SSP
 *
 * @return UINT64
 */
extern UINT64
AsmReadSsp();

//
// ====================  Hook Functions ====================
// File : AsmHooks.asm
//

/**
 * @brief Detour hook handler
 *
 */
extern VOID
    AsmGeneralDetourHook(VOID);

//
// ====================  Kernel Test Functions ====================
// File : AsmKernelSideTests.asm
//

/**
 * @brief Tests with test tags wrapper
 *
 */
extern UINT64
AsmTestWrapperWithTestTags(UINT64 Param1,
                           UINT64 Param2,
                           UINT64 Param3,
                           UINT64 Param4);

//
// ====================  Interrupt Handler Functions ====================
// File : Interrupt Handlers.asm.asm
//

/**
 * @brief The 0th entry in IDT
 *
 */
extern VOID
InterruptHandler0();

/**
 * @brief The 1st entry in IDT
 *
 */
extern VOID
InterruptHandler1();

/**
 * @brief The 2nd entry in IDT
 *
 */
extern VOID
InterruptHandler2();

/**
 * @brief The 3rd entry in IDT
 *
 */
extern VOID
InterruptHandler3();

/**
 * @brief The 4th entry in IDT
 *
 */
extern VOID
InterruptHandler4();

/**
 * @brief The 5th entry in IDT
 *
 */
extern VOID
InterruptHandler5();

/**
 * @brief The 6th entry in IDT
 *
 */
extern VOID
InterruptHandler6();

/**
 * @brief The 7th entry in IDT
 *
 */
extern VOID
InterruptHandler7();

/**
 * @brief The 8th entry in IDT
 *
 */
extern VOID
InterruptHandler8();

/**
 * @brief The 9th entry in IDT
 *
 */
extern VOID
InterruptHandler9();

/**
 * @brief The 10th entry in IDT
 *
 */
extern VOID
InterruptHandler10();

/**
 * @brief The 11th entry in IDT
 *
 */
extern VOID
InterruptHandler11();

/**
 * @brief The 12th entry in IDT
 *
 */
extern VOID
InterruptHandler12();

/**
 * @brief The 13th entry in IDT
 *
 */
extern VOID
InterruptHandler13();

/**
 * @brief The 14th entry in IDT
 *
 */
extern VOID
InterruptHandler14();

/**
 * @brief The 15th entry in IDT
 *
 */
extern VOID
InterruptHandler15();

/**
 * @brief The 16th entry in IDT
 *
 */
extern VOID
InterruptHandler16();

/**
 * @brief The 17th entry in IDT
 *
 */
extern VOID
InterruptHandler17();

/**
 * @brief The 18th entry in IDT
 *
 */
extern VOID
InterruptHandler18();

/**
 * @brief The 19th entry in IDT
 *
 */
extern VOID
InterruptHandler19();

/**
 * @brief The 20th entry in IDT
 *
 */
extern VOID
InterruptHandler20();

/**
 * @brief The 21st entry in IDT
 *
 */
extern VOID
InterruptHandler21();

/**
 * @brief The 22nd entry in IDT
 *
 */
extern VOID
InterruptHandler22();

/**
 * @brief The 23rd entry in IDT
 *
 */
extern VOID
InterruptHandler23();

/**
 * @brief The 24th entry in IDT
 *
 */
extern VOID
InterruptHandler24();

/**
 * @brief The 25th entry in IDT
 *
 */
extern VOID
InterruptHandler25();

/**
 * @brief The 26th entry in IDT
 *
 */
extern VOID
InterruptHandler26();

/**
 * @brief The 27th entry in IDT
 *
 */
extern VOID
InterruptHandler27();

/**
 * @brief The 28th entry in IDT
 *
 */
extern VOID
InterruptHandler28();

/**
 * @brief The 29th entry in IDT
 *
 */
extern VOID
InterruptHandler29();

/**
 * @brief The 30th entry in IDT
 *
 */
extern VOID
InterruptHandler30();
