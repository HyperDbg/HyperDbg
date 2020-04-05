#include <ntddk.h>
#include <Windef.h>
#include "Common.h"
#include "Hooks.h"
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

VOID SyscallHookConfigureEFER(BOOLEAN EnableEFERSyscallHook)
{
    EFER_MSR MsrValue;

    MsrValue.Flags = __readmsr(MSR_EFER);
    if (EnableEFERSyscallHook)
    {
        MsrValue.SyscallEnable = FALSE;
    }
    else
    {
        MsrValue.SyscallEnable = TRUE;
    }

    __writemsr(MSR_EFER, MsrValue.Flags);

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
    UINT64 MsrValue;
    ULONG64 GuestRip;
    ULONG64 GuestRflags;

    // Reading guest's RIP 
    __vmx_vmread(GUEST_RIP, &GuestRip);

    // Reading guest's Rflags
    __vmx_vmread(GUEST_RFLAGS, &GuestRflags);

    // Save the address of the instruction following SYSCALL into RCX and then
    // load RIP from MSR_LSTAR.
    MsrValue = __readmsr(MSR_LSTAR);
    Regs->rcx= GuestRip;
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

    // Due to KVA Shadowing, we need to switch to a different directory table base 
    // if the PCID indicates this is a user mode directory table base.

    __vmx_vmread(GUEST_CR3, &GuestCr3);
    if ((GuestCr3 & PCID_MASK) != PCID_NONE)
    {
        DbgBreakPoint();
        OriginalCr3 = __readcr3();
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

    // Emulate SYSRET instruction.
EmulateSYSRET:
    LogInfo("SYSRET instruction => 0x%llX", Rip);
    DbgBreakPoint();
    return SyscallHookEmulateSYSRET(Regs);
    // Emulate SYSCALL instruction.
EmulateSYSCALL:
    LogInfo("SYSCALL instruction => 0x%llX", Rip);
    DbgBreakPoint();
    return SyscallHookEmulateSYSCALL(Regs);
}
