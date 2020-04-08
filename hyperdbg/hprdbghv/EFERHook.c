#include <ntddk.h>
#include <Windef.h>
#include "Common.h"
#include "Msr.h"
#include "Hooks.h"
#include "Invept.h"
#include "Events.h"
#include "HypervisorRoutines.h"
#include "GlobalVariables.h"
#include "Vmx.h"
#include "Logging.h"




#define IS_SYSRET_INSTRUCTION(Code) \
    (*((PUINT8)(Code) + 0) == 0x48 && \
     *((PUINT8)(Code) + 1) == 0x0F && \
     *((PUINT8)(Code) + 2) == 0x07)
#define IS_SYSCALL_INSTRUCTION(Code) \
    (*((PUINT8)(Code) + 0) == 0x0F && \
     *((PUINT8)(Code) + 1) == 0x05)

// register for address of syscall handler
#define MSR_EFER        0xc0000080
#define MSR_STAR        0xc0000081
#define MSR_LSTAR       0xc0000082
#define MSR_FMASK       0xc0000084




/* EFLAGS/RFLAGS */
#define X86_FLAGS_CF                    (1<<0)
#define X86_FLAGS_PF                    (1<<2)
#define X86_FLAGS_AF                    (1<<4)
#define X86_FLAGS_ZF                    (1<<6)
#define X86_FLAGS_SF                    (1<<7)
#define X86_FLAGS_TF                    (1<<8)
#define X86_FLAGS_IF                    (1<<9)
#define X86_FLAGS_DF                    (1<<10)
#define X86_FLAGS_OF                    (1<<11)
#define X86_FLAGS_STATUS_MASK           (0xfff)
#define X86_FLAGS_IOPL_MASK             (3<<12)
#define X86_FLAGS_IOPL_SHIFT            (12)
#define X86_FLAGS_NT                    (1<<14)
#define X86_FLAGS_RF                    (1<<16)
#define X86_FLAGS_VM                    (1<<17)
#define X86_FLAGS_AC                    (1<<18)
#define X86_FLAGS_VIF                   (1<<19)
#define X86_FLAGS_VIP                   (1<<20)
#define X86_FLAGS_ID                    (1<<21)
#define X86_FLAGS_RESERVED_ONES         0x2
#define X86_FLAGS_RESERVED              0xffc0802a

#define X86_FLAGS_RESERVED_BITS       0xffc38028
#define X86_FLAGS_FIXED               0x00000002

#define PCID_NONE   0x000
#define PCID_MASK   0x003

typedef union _PAGE_FAULT_ERROR_CODE
{
    struct
    {
        UINT32   P : 1;      // 0: non-present page; 1: page-level protection violation
        UINT32   Wr : 1;     // 0: read access; 1: write access
        UINT32   Us : 1;     // 0: supervisor-mode access; 1: user-mode access
        UINT32   Rsvd : 1;   // 0: reserved bit violation; 1: reserved bit set to 1 in a paging structure entry
        UINT32   Id : 1;     // 0: not an instruction fetch; 1: instruction fetch
        UINT32   Pk : 1;     // 0: not a protection key violation; 1: protection key violation
        UINT32   Ss : 1;     // 0: not caused by a shadow-stack access : 1: caused by a shadow-stack access
        UINT32   Reserved : 8;
        UINT32   Sgx : 1;    // 0: not related to SGX; 1: SGX-specific access-control requirements violation
    } Fields;
    UINT32       ErrorCode;
} PAGE_FAULT_ERROR_CODE, *PPAGE_FAULT_ERROR_CODE;

VOID SyscallHookDisableSCE() {

    EFER_MSR MsrValue;

    // Set the GUEST EFER to use this value as the EFER
    __vmx_vmread(GUEST_EFER, &MsrValue);
    MsrValue.SyscallEnable = FALSE;

    // Set the GUEST EFER to use this value as the EFER
    __vmx_vmwrite(GUEST_EFER, MsrValue.Flags);

}
VOID SyscallHookEnableSCE() {

    EFER_MSR MsrValue;

    // Set the GUEST EFER to use this value as the EFER
    __vmx_vmread(GUEST_EFER, &MsrValue);
    MsrValue.SyscallEnable = TRUE;

    // Set the GUEST EFER to use this value as the EFER
    __vmx_vmwrite(GUEST_EFER, MsrValue.Flags);

}

VOID SyscallHookConfigureEFER(BOOLEAN EnableEFERSyscallHook)
{
    EFER_MSR MsrValue;
    IA32_VMX_BASIC_MSR VmxBasicMsr = { 0 };
    UINT32 VmEntryControls = 0;
    UINT32 VmExitControls = 0;

    // Reading IA32_VMX_BASIC_MSR 
    VmxBasicMsr.All = __readmsr(MSR_IA32_VMX_BASIC);

    MsrValue.Flags = __readmsr(MSR_EFER);
    if (EnableEFERSyscallHook)
    {
        MsrValue.SyscallEnable = FALSE;
    }
    else
    {
        MsrValue.SyscallEnable = TRUE;
    }

    // Read previous VM-Exit and VM-Entry controls
    __vmx_vmread(VM_ENTRY_CONTROLS, &VmEntryControls);
    __vmx_vmread(VM_EXIT_CONTROLS, &VmExitControls);

    // Set VM-Entry controls to load EFER
    __vmx_vmwrite(VM_ENTRY_CONTROLS, HvAdjustControls(VmEntryControls | VM_ENTRY_LOAD_IA32_EFER,
        VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_ENTRY_CTLS : MSR_IA32_VMX_ENTRY_CTLS));

    // Set VM-Exit controls to save EFER
    __vmx_vmwrite(VM_EXIT_CONTROLS, HvAdjustControls(VmExitControls | VM_EXIT_SAVE_IA32_EFER,
        VmxBasicMsr.Fields.VmxCapabilityHint ? MSR_IA32_VMX_TRUE_EXIT_CTLS : MSR_IA32_VMX_EXIT_CTLS));
   
    // Set the GUEST EFER to use this value as the EFER
    __vmx_vmwrite(GUEST_EFER, MsrValue.Flags);

}

VOID SetGuestCs(PSEGMENT_SELECTOR Cs)
{
    __vmx_vmwrite(GUEST_CS_BASE, Cs->BASE);
    __vmx_vmwrite(GUEST_CS_LIMIT, Cs->LIMIT);
    __vmx_vmwrite(GUEST_CS_AR_BYTES, Cs->ATTRIBUTES.UCHARs);
    __vmx_vmwrite(GUEST_CS_SELECTOR, Cs->SEL);
}

VOID SetGuestSs(PSEGMENT_SELECTOR Cs)
{
    __vmx_vmwrite(GUEST_SS_BASE, Cs->BASE);
    __vmx_vmwrite(GUEST_SS_LIMIT, Cs->LIMIT);
    __vmx_vmwrite(GUEST_SS_AR_BYTES, Cs->ATTRIBUTES.UCHARs);
    __vmx_vmwrite(GUEST_SS_SELECTOR, Cs->SEL);
}


/* SYSCALL instruction emulation routine */
BOOLEAN SyscallHookEmulateSYSCALL(PGUEST_REGS Regs)
{
    SEGMENT_SELECTOR Cs, Ss;
    UINT32 InstructionLength;
    UINT64 MsrValue;
    ULONG64 GuestRip;
    ULONG64 GuestRflags;

    // Reading guest's RIP 
    __vmx_vmread(GUEST_RIP, &GuestRip);

    // Reading instruction length 
    __vmx_vmread(VM_EXIT_INSTRUCTION_LEN, &InstructionLength);

    // Reading guest's Rflags
    __vmx_vmread(GUEST_RFLAGS, &GuestRflags);

    // Save the address of the instruction following SYSCALL into RCX and then
    // load RIP from MSR_LSTAR.
    MsrValue = __readmsr(MSR_LSTAR);
    Regs->rcx= GuestRip + InstructionLength;
    GuestRip = MsrValue;
    __vmx_vmwrite(GUEST_RIP, GuestRip);

    // Save RFLAGS into R11 and then mask RFLAGS using MSR_FMASK.
    MsrValue = __readmsr(MSR_FMASK);
    Regs->r11 = GuestRflags;
    GuestRflags &= ~(MsrValue | X86_FLAGS_RF);
    __vmx_vmwrite(GUEST_RFLAGS, GuestRflags);

    // Load the CS and SS selectors with values derived from bits 47:32 of MSR_STAR.
    MsrValue = __readmsr(MSR_STAR);
    Cs.SEL = (UINT16)((MsrValue >> 32) & ~3);          // STAR[47:32] & ~RPL3
    Cs.BASE = 0;                                            // flat segment
    Cs.LIMIT = (UINT32)~0;                                  // 4GB limit
    Cs.ATTRIBUTES.UCHARs = 0xA09B;                              // L+DB+P+S+DPL0+Code
    SetGuestCs(&Cs);


    Ss.SEL = (UINT16)(((MsrValue >> 32) & ~3) + 8);    // STAR[47:32] + 8
    Ss.BASE = 0;                                            // flat segment
    Ss.LIMIT = (UINT32)~0;                                  // 4GB limit
    Ss.ATTRIBUTES.UCHARs = 0xC093;                              // G+DB+P+S+DPL0+Data
    SetGuestSs(&Ss);
    return TRUE;
}


/* SYSRET instruction emulation routine */
BOOLEAN SyscallHookEmulateSYSRET(PGUEST_REGS Regs)
{
    SEGMENT_SELECTOR Cs, Ss;
    UINT64 MsrValue;

    ULONG64 GuestRip;
    ULONG64 GuestRflags;

    // Load RIP from RCX.
    GuestRip = Regs->rcx;
    __vmx_vmwrite(GUEST_RIP, GuestRip);

    // Load RFLAGS from R11. Clear RF, VM, reserved bits.
    GuestRflags = (Regs->r11 & ~(X86_FLAGS_RF | X86_FLAGS_VM | X86_FLAGS_RESERVED_BITS)) | X86_FLAGS_FIXED;
    __vmx_vmwrite(GUEST_RFLAGS, GuestRflags);

    // SYSRET loads the CS and SS selectors with values derived from bits 63:48 of MSR_STAR.
    MsrValue = __readmsr(MSR_STAR);
    Cs.SEL = (UINT16)(((MsrValue >> 48) + 16) | 3);    // (STAR[63:48]+16) | 3 (* RPL forced to 3 *)
    Cs.BASE = 0;                                            // Flat segment
    Cs.LIMIT = (UINT32)~0;                                  // 4GB limit
    Cs.ATTRIBUTES.UCHARs = 0xA0FB;                              // L+DB+P+S+DPL3+Code
    SetGuestCs(&Cs);


    Ss.SEL = (UINT16)(((MsrValue >> 48) + 8) | 3);     // (STAR[63:48]+8) | 3 (* RPL forced to 3 *)
    Ss.BASE = 0;                                            // Flat segment
    Ss.LIMIT = (UINT32)~0;                                  // 4GB limit
    Ss.ATTRIBUTES.UCHARs = 0xC0F3;                                  // G+DB+P+S+DPL3+Data
    SetGuestSs(&Ss);
    return TRUE;
}

BOOLEAN SyscallHookHandleUD(PGUEST_REGS Regs)
{
    UINT64 GuestCr3;
    UINT64 OriginalCr3;
    UINT64 Rip;


    // Reading guest's RIP 
    __vmx_vmread(GUEST_RIP, &Rip);

    if (Rip & 0xff00000000000000)
    {
        // Means that it's a sysret
        goto EmulateSYSRET;
    }
    else
    {
        goto EmulateSYSCALL;
    }

    /*
    // Due to KVA Shadowing, we need to switch to a different directory table base 
    // if the PCID indicates this is a user mode directory table base.

    __vmx_vmread(GUEST_CR3, &GuestCr3);
    if ((GuestCr3 & PCID_MASK) != PCID_NONE)
    {
        OriginalCr3 = __readcr3();
        // ------------------------------------------------------------------------
        DbgBreakPoint();
       // SyscallHookEmulateSYSRET(Regs);
        __writecr2(Rip);

        PAGE_FAULT_ERROR_CODE PageFaultErrorCode = { 0 };
        PageFaultErrorCode.Fields.P = 0; // The fault was caused by a non-present page.
        PageFaultErrorCode.Fields.Wr = 0; // The access causing the fault was a read.
        PageFaultErrorCode.Fields.Us = 0; // A supervisor-mode access caused the fault.
        PageFaultErrorCode.Fields.Rsvd = 0; // The fault was not caused by reserved bit violation.
        PageFaultErrorCode.Fields.Id = 1; // 1 The fault was caused by an instruction fetch.
        PageFaultErrorCode.Fields.Pk = 0; // The fault was not caused by protection keys.
        PageFaultErrorCode.Fields.Sgx = 0; // The fault was caused by a non-present page.

        // Insert the fault
        EventInjectPageFault(PageFaultErrorCode.ErrorCode);
        GuestState[KeGetCurrentProcessorIndex()].IncrementRip = FALSE;
        DbgBreakPoint();
        return FALSE;
        DbgBreakPoint();
        // ------------------------------------------------------------------------
        NT_KPROCESS* CurrentProcess = (NT_KPROCESS*)(PsGetCurrentProcess());
        __writecr3(CurrentProcess->DirectoryTableBase);

        if (IS_SYSRET_INSTRUCTION(Rip))
        {
            __writecr3(OriginalCr3);
            goto EmulateSYSRET;
        }
        if (IS_SYSCALL_INSTRUCTION(Rip))
        {
            __writecr3(OriginalCr3);
            goto EmulateSYSCALL;
        }
        __writecr3(OriginalCr3);
        return FALSE;
    }

    else
    {
        if (IS_SYSRET_INSTRUCTION(Rip))
            goto EmulateSYSRET;
        if (IS_SYSCALL_INSTRUCTION(Rip))
            goto EmulateSYSCALL;
        return FALSE;
    }
    */

    // Emulate SYSRET instruction.
EmulateSYSRET:
    LogInfo("SYSRET instruction => 0x%llX", Rip);
    BOOLEAN Result = SyscallHookEmulateSYSRET(Regs);
    GuestState[KeGetCurrentProcessorIndex()].IncrementRip = FALSE;
    DbgBreakPoint();
    return Result;
    // Emulate SYSCALL instruction.
EmulateSYSCALL:
    LogInfo("SYSCALL instruction => 0x%llX , process id : 0x%x , rax = 0x%llx", Rip, PsGetCurrentProcessId(), Regs->rax);
    // Result = SyscallHookEmulateSYSCALL(Regs);
    Result = TRUE;
    SyscallHookEnableSCE();
    HvSetMonitorTrapFlag(TRUE);
    GuestState[KeGetCurrentProcessorIndex()].IncrementRip = FALSE;
    DbgBreakPoint();
    return Result;
}
