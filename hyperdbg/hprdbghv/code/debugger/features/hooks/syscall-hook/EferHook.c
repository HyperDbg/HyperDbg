/**
 * @file EferHook.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Implenetation of the fucntions related to the EFER Syscall Hook
 * @details This is derived by the method demonstrated at
 * - https://revers.engineering/syscall-hooking-via-extended-feature-enable-register-efer/
 * 
 * also some of the functions derived from hvpp
 * - https://github.com/wbenny/hvpp
 * 
 * @version 0.1
 * @date 2020-04-10
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief This function enables or disables EFER syscall hoo
 * @details This function should be called for the first time
 * that we want to enable EFER hook because after calling this 
 * function EFER MSR is loaded from GUEST_EFER instead of loading
 * from the regular EFER MSR.
 * 
 * @param EnableEFERSyscallHook Determines whether we want to enable syscall hook or disable syscall hook
 * @return VOID 
 */
VOID
SyscallHookConfigureEFER(BOOLEAN EnableEFERSyscallHook)
{
    IA32_EFER_REGISTER      MsrValue;
    IA32_VMX_BASIC_REGISTER VmxBasicMsr     = {0};
    UINT32                  VmEntryControls = 0;
    UINT32                  VmExitControls  = 0;

    //
    // Reading IA32_VMX_BASIC_MSR
    //
    VmxBasicMsr.AsUInt = __readmsr(IA32_VMX_BASIC);

    //
    // Read previous VM-Entry and VM-Exit controls
    //
    __vmx_vmread(VMCS_CTRL_VMENTRY_CONTROLS, &VmEntryControls);
    __vmx_vmread(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, &VmExitControls);

    MsrValue.AsUInt = __readmsr(IA32_EFER);

    if (EnableEFERSyscallHook)
    {
        MsrValue.SyscallEnable = FALSE;

        //
        // Set VM-Entry controls to load EFER
        //
        __vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS, HvAdjustControls(VmEntryControls | VM_ENTRY_LOAD_IA32_EFER, VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS));

        //
        // Set VM-Exit controls to save EFER
        //
        __vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, HvAdjustControls(VmExitControls | VM_EXIT_SAVE_IA32_EFER, VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS));

        //
        // Set the GUEST EFER to use this value as the EFER
        //
        __vmx_vmwrite(VMCS_GUEST_EFER, MsrValue.AsUInt);

        //
        // also, we have to set exception bitmap to cause vm-exit on #UDs
        //
        HvSetExceptionBitmap(EXCEPTION_VECTOR_UNDEFINED_OPCODE);
    }
    else
    {
        MsrValue.SyscallEnable = TRUE;

        //
        // Set VM-Entry controls to load EFER
        //
        __vmx_vmwrite(VMCS_CTRL_VMENTRY_CONTROLS, HvAdjustControls(VmEntryControls & ~VM_ENTRY_LOAD_IA32_EFER, VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_ENTRY_CTLS : IA32_VMX_ENTRY_CTLS));

        //
        // Set VM-Exit controls to save EFER
        //
        __vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS, HvAdjustControls(VmExitControls & ~VM_EXIT_SAVE_IA32_EFER, VmxBasicMsr.VmxControls ? IA32_VMX_TRUE_EXIT_CTLS : IA32_VMX_EXIT_CTLS));

        //
        // Set the GUEST EFER to use this value as the EFER
        //
        __vmx_vmwrite(VMCS_GUEST_EFER, MsrValue.AsUInt);

        //
        // Because we're not save or load EFER on vm-exits so
        // we have to set it manually
        //
        __writemsr(IA32_EFER, MsrValue.AsUInt);

        //
        // unset the exception to not cause vm-exit on #UDs
        //
        ProtectedHvRemoveUndefinedInstructionForDisablingSyscallSysretCommands();
    }
}

/**
 * @brief This function emulates the SYSCALL execution 
 * 
 * @param Regs Guest registers
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
SyscallHookEmulateSYSCALL(PGUEST_REGS Regs)
{
    VMX_SEGMENT_SELECTOR Cs, Ss;
    UINT32               InstructionLength;
    UINT64               MsrValue;
    UINT64               GuestRip;
    UINT64               GuestRflags;

    //
    // Reading guest's RIP
    //
    __vmx_vmread(VMCS_GUEST_RIP, &GuestRip);

    //
    // Reading instruction length
    //
    __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &InstructionLength);

    //
    // Reading guest's Rflags
    //
    __vmx_vmread(VMCS_GUEST_RFLAGS, &GuestRflags);

    //
    // Save the address of the instruction following SYSCALL into RCX and then
    // load RIP from IA32_LSTAR.
    //
    MsrValue  = __readmsr(IA32_LSTAR);
    Regs->rcx = GuestRip + InstructionLength;
    GuestRip  = MsrValue;
    __vmx_vmwrite(VMCS_GUEST_RIP, GuestRip);

    //
    // Save RFLAGS into R11 and then mask RFLAGS using IA32_FMASK
    //
    MsrValue  = __readmsr(IA32_FMASK);
    Regs->r11 = GuestRflags;
    GuestRflags &= ~(MsrValue | X86_FLAGS_RF);
    __vmx_vmwrite(VMCS_GUEST_RFLAGS, GuestRflags);

    //
    // Load the CS and SS selectors with values derived from bits 47:32 of IA32_STAR
    //
    MsrValue             = __readmsr(IA32_STAR);
    Cs.Selector          = (UINT16)((MsrValue >> 32) & ~3); // STAR[47:32] & ~RPL3
    Cs.Base              = 0;                               // flat segment
    Cs.Limit             = (UINT32)~0;                      // 4GB limit
    Cs.Attributes.AsUInt = 0xA09B;                          // L+DB+P+S+DPL0+Code
    SetGuestCs(&Cs);

    Ss.Selector          = (UINT16)(((MsrValue >> 32) & ~3) + 8); // STAR[47:32] + 8
    Ss.Base              = 0;                                     // flat segment
    Ss.Limit             = (UINT32)~0;                            // 4GB limit
    Ss.Attributes.AsUInt = 0xC093;                                // G+DB+P+S+DPL0+Data
    SetGuestSs(&Ss);

    return TRUE;
}

/**
 * @brief This function emulates the SYSRET execution 
 * 
 * @param Regs Guest registers
 * @return BOOLEAN
 */
_Use_decl_annotations_
BOOLEAN
SyscallHookEmulateSYSRET(PGUEST_REGS Regs)
{
    VMX_SEGMENT_SELECTOR Cs, Ss;
    UINT64               MsrValue;
    UINT64               GuestRip;
    UINT64               GuestRflags;

    //
    // Load RIP from RCX
    //
    GuestRip = Regs->rcx;
    __vmx_vmwrite(VMCS_GUEST_RIP, GuestRip);

    //
    // Load RFLAGS from R11. Clear RF, VM, reserved bits
    //
    GuestRflags = (Regs->r11 & ~(X86_FLAGS_RF | X86_FLAGS_VM | X86_FLAGS_RESERVED_BITS)) | X86_FLAGS_FIXED;
    __vmx_vmwrite(VMCS_GUEST_RFLAGS, GuestRflags);

    //
    // SYSRET loads the CS and SS selectors with values derived from bits 63:48 of IA32_STAR
    //
    MsrValue             = __readmsr(IA32_STAR);
    Cs.Selector          = (UINT16)(((MsrValue >> 48) + 16) | 3); // (STAR[63:48]+16) | 3 (* RPL forced to 3 *)
    Cs.Base              = 0;                                     // Flat segment
    Cs.Limit             = (UINT32)~0;                            // 4GB limit
    Cs.Attributes.AsUInt = 0xA0FB;                                // L+DB+P+S+DPL3+Code
    SetGuestCs(&Cs);

    Ss.Selector          = (UINT16)(((MsrValue >> 48) + 8) | 3); // (STAR[63:48]+8) | 3 (* RPL forced to 3 *)
    Ss.Base              = 0;                                    // Flat segment
    Ss.Limit             = (UINT32)~0;                           // 4GB limit
    Ss.Attributes.AsUInt = 0xC0F3;                               // G+DB+P+S+DPL3+Data
    SetGuestSs(&Ss);

    return TRUE;
}

/**
 * @brief Detect whether the #UD was because of Syscall or Sysret or not
 * 
 * @param Regs Guest register
 * @param CoreIndex Logical core index
 * @return BOOLEAN Shows whther the caller should inject #UD on the guest or not
 */
_Use_decl_annotations_
BOOLEAN
SyscallHookHandleUD(PGUEST_REGS Regs, UINT32 CoreIndex)
{
    CR3_TYPE                GuestCr3;
    UINT64                  OriginalCr3;
    UINT64                  Rip;
    BOOLEAN                 Result;
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CoreIndex];

    //
    // Reading guest's RIP
    //
    __vmx_vmread(VMCS_GUEST_RIP, &Rip);

    if (g_IsUnsafeSyscallOrSysretHandling)
    {
        //
        // In some computers, we realized that safe accessing to memory
        // is problematic. It means that our syscall approach might not
        // working properly.
        // Based on our tests, we realized that system doesn't generate #UD
        // regularly. So, we can imagine that only kernel addresses are SYSRET
        // instruction and SYSCALL is on a user-mode RIP.
        // It's faster than our safe methods but if the system generates a #UD
        // then a BSOD will happen. But if the system is working regularly, then
        // no BSOD happens. For more information, see documentation at !syscall2
        // or !sysret2 commands
        //
        if (Rip & 0xff00000000000000)
        {
            goto EmulateSYSRET;
        }
        else
        {
            goto EmulateSYSCALL;
        }
    }
    else
    {
        //
        // Get the guest's running process's cr3
        //
        GuestCr3.Flags = GetRunningCr3OnTargetProcess().Flags;

        //
        // No, longer needs to be checked because we're sticking to system process
        // and we have to change the cr3
        //
        // if ((GuestCr3.Flags & PCID_MASK) != PCID_NONE)

        OriginalCr3 = __readcr3();

        __writecr3(GuestCr3.Flags);

        //
        // Read the memory
        //
        UCHAR InstructionBuffer[3] = {0};

        if (MemoryMapperCheckIfPageIsPresentByCr3(Rip, GuestCr3))
        {
            //
            // The page is safe to read (present)
            // It's not necessary to use MemoryMapperReadMemorySafeOnTargetProcess
            // because we already switched to the process's cr3
            //
            MemoryMapperReadMemorySafe(Rip, InstructionBuffer, 3);
        }
        else
        {
            //
            // The page is not present, we have to inject a #PF
            //
            CurrentVmState->IncrementRip = FALSE;

            //
            // For testing purpose
            //
            // LogInfo("#PF Injected");

            //
            // Inject #PF
            //
            EventInjectPageFault(Rip);

            //
            // We should not inject #UD
            //
            return FALSE;
        }

        __writecr3(OriginalCr3);

        if (InstructionBuffer[0] == 0x0F &&
            InstructionBuffer[1] == 0x05)
        {
            goto EmulateSYSCALL;
        }

        if (InstructionBuffer[0] == 0x48 &&
            InstructionBuffer[1] == 0x0F &&
            InstructionBuffer[2] == 0x07)
        {
            goto EmulateSYSRET;
        }

        return FALSE;
    }

    //----------------------------------------------------------------------------------------

    //
    // Emulate SYSRET instruction
    //
EmulateSYSRET:
    //
    // Test
    //
    // LogInfo("SYSRET instruction => 0x%llX", Rip);
    //

    //
    // We should trigger the event of SYSRET here
    //
    DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSRET, Regs, Rip);

    Result                       = SyscallHookEmulateSYSRET(Regs);
    CurrentVmState->IncrementRip = FALSE;
    return Result;

    //
    // Emulate SYSCALL instruction
    //

EmulateSYSCALL:

    //
    // Test
    //
    // LogInfo("SYSCALL instruction => 0x%llX , Process Id : 0x%x", Rip, PsGetCurrentProcessId());
    //

    //
    // We should trigger the event of SYSCALL here, we send the
    // syscall number in rax
    //
    DebuggerTriggerEvents(SYSCALL_HOOK_EFER_SYSCALL, Regs, Regs->rax);

    Result = SyscallHookEmulateSYSCALL(Regs);

    CurrentVmState->IncrementRip = FALSE;
    return Result;
}
