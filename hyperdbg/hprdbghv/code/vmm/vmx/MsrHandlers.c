/**
 * @file MsrHandlers.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Handle for MSR-related tasks in VMX-root
 * 
 * @version 0.1
 * @date 2021-12-24
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#include "pch.h"

/**
 * @brief Handles in the cases when RDMSR causes a vm-exit
 * 
 * @param GuestRegs Guest's gp registers
 * @return VOID 
 */
VOID
MsrHandleRdmsrVmexit(PGUEST_REGS GuestRegs)
{
    MSR    Msr = {0};
    UINT32 TargetMsr;

    //
    // RDMSR. The RDMSR instruction causes a VM exit if any of the following are true:
    //
    // The "use MSR bitmaps" VM-execution control is 0.
    // The value of ECX is not in the ranges 00000000H - 00001FFFH and C0000000H - C0001FFFH
    // The value of ECX is in the range 00000000H - 00001FFFH and bit n in read bitmap for low MSRs is 1,
    //   where n is the value of ECX.
    // The value of ECX is in the range C0000000H - C0001FFFH and bit n in read bitmap for high MSRs is 1,
    //   where n is the value of ECX & 00001FFFH.
    //
    TargetMsr = GuestRegs->rcx & 0xffffffff;

    //
    // Execute WRMSR or RDMSR on behalf of the guest. Important that this
    // can cause bug check when the guest tries to access unimplemented MSR
    // even within the SEH block* because the below WRMSR or RDMSR raises
    // #GP and are not protected by the SEH block (or cannot be protected
    // either as this code run outside the thread stack region Windows
    // requires to proceed SEH). Hypervisors typically handle this by noop-ing
    // WRMSR and returning zero for RDMSR with non-architecturally defined
    // MSRs. Alternatively, one can probe which MSRs should cause #GP prior
    // to installation of a hypervisor and the hypervisor can emulate the
    // results.
    //

    //
    // Check for sanity of MSR if they're valid or they're for reserved range for WRMSR and RDMSR
    //
    if ((TargetMsr <= 0x00001FFF) || ((0xC0000000 <= TargetMsr) && (TargetMsr <= 0xC0001FFF)) ||
        (TargetMsr >= RESERVED_MSR_RANGE_LOW && (TargetMsr <= RESERVED_MSR_RANGE_HI)))
    {
        //
        // Apply the RDMS
        //
        switch (TargetMsr)
        {
        case IA32_SYSENTER_CS:
            __vmx_vmread(VMCS_GUEST_SYSENTER_CS, &Msr);
            break;

        case IA32_SYSENTER_ESP:
            __vmx_vmread(VMCS_GUEST_SYSENTER_ESP, &Msr);
            break;

        case IA32_SYSENTER_EIP:
            __vmx_vmread(VMCS_GUEST_SYSENTER_EIP, &Msr);
            break;

        case IA32_GS_BASE:
            __vmx_vmread(VMCS_GUEST_GS_BASE, &Msr);
            break;

        case IA32_FS_BASE:
            __vmx_vmread(VMCS_GUEST_FS_BASE, &Msr);
            break;

        default:

            //
            // Check whether the MSR should cause #GP or not
            //
            if (/* TargetMsr >= 0x0 && */ TargetMsr <= 0xfff && TestBit(TargetMsr, g_MsrBitmapInvalidMsrs) != NULL)
            {
                //
                // Invalid MSR between 0x0 to 0xfff
                //
                EventInjectGeneralProtection();
                return;
            }

            //
            // Msr is valid
            //
            Msr.Flags = __readmsr(TargetMsr);

            //
            // Check if it's EFER MSR then we show a false SCE state
            //
            if (GuestRegs->rcx == IA32_EFER)
            {
                IA32_EFER_REGISTER MsrEFER;
                MsrEFER.AsUInt        = Msr.Flags;
                MsrEFER.SyscallEnable = TRUE;
                Msr.Flags             = MsrEFER.AsUInt;
            }

            break;
        }

        GuestRegs->rax = NULL;
        GuestRegs->rdx = NULL;

        GuestRegs->rax = Msr.Fields.Low;
        GuestRegs->rdx = Msr.Fields.High;
    }
    else
    {
        //
        // MSR is invalid, inject #GP
        //
        EventInjectGeneralProtection();
        return;
    }
}

/**
 * @brief Handles in the cases when RDMSR causes a vm-exit
 * 
 * @param GuestRegs Guest's gp registers
 * @return VOID 
 */
VOID
MsrHandleWrmsrVmexit(PGUEST_REGS GuestRegs)
{
    MSR     Msr = {0};
    UINT32  TargetMsr;
    BOOLEAN UnusedIsKernel;

    //
    // Execute WRMSR or RDMSR on behalf of the guest. Important that this
    // can cause bug check when the guest tries to access unimplemented MSR
    // even within the SEH block* because the below WRMSR or RDMSR raises
    // #GP and are not protected by the SEH block (or cannot be protected
    // either as this code run outside the thread stack region Windows
    // requires to proceed SEH). Hypervisors typically handle this by noop-ing
    // WRMSR and returning zero for RDMSR with non-architecturally defined
    // MSRs. Alternatively, one can probe which MSRs should cause #GP prior
    // to installation of a hypervisor and the hypervisor can emulate the
    // results.
    //
    TargetMsr = GuestRegs->rcx & 0xffffffff;

    Msr.Fields.Low  = (ULONG)GuestRegs->rax;
    Msr.Fields.High = (ULONG)GuestRegs->rdx;

    //
    // Check for sanity of MSR if they're valid or they're for reserved range for WRMSR and RDMSR
    //
    if ((TargetMsr <= 0x00001FFF) || ((0xC0000000 <= TargetMsr) && (TargetMsr <= 0xC0001FFF)) ||
        (TargetMsr >= RESERVED_MSR_RANGE_LOW && (TargetMsr <= RESERVED_MSR_RANGE_HI)))
    {
        //
        // If the source register contains a non-canonical address and ECX specifies
        // one of the following MSRs:
        //
        // IA32_DS_AREA, IA32_FS_BASE, IA32_GS_BASE, IA32_KERNEL_GSBASE, IA32_LSTAR,
        // IA32_SYSENTER_EIP, IA32_SYSENTER_ESP
        //
        switch (TargetMsr)
        {
        case IA32_DS_AREA:
        case IA32_FS_BASE:
        case IA32_GS_BASE:
        case IA32_KERNEL_GS_BASE:
        case IA32_LSTAR:
        case IA32_SYSENTER_EIP:
        case IA32_SYSENTER_ESP:

            if (!CheckCanonicalVirtualAddress(Msr.Flags, &UnusedIsKernel))
            {
                //
                // Address is not canonical, inject #GP
                //
                EventInjectGeneralProtection();

                return;
            }

            break;
        }

        //
        // Perform MSR change
        //
        switch (TargetMsr)
        {
        case IA32_SYSENTER_CS:
            __vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, Msr.Flags);
            break;

        case IA32_SYSENTER_ESP:
            __vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, Msr.Flags);
            break;

        case IA32_SYSENTER_EIP:
            __vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, Msr.Flags);
            break;

        case IA32_GS_BASE:
            __vmx_vmwrite(VMCS_GUEST_GS_BASE, Msr.Flags);
            break;

        case IA32_FS_BASE:
            __vmx_vmwrite(VMCS_GUEST_FS_BASE, Msr.Flags);
            break;

        default:

            //
            // Perform the WRMSR
            //
            __writemsr(GuestRegs->rcx, Msr.Flags);
            break;
        }
    }
    else
    {
        //
        // Msr is invalid, inject #GP
        //
        EventInjectGeneralProtection();
        return;
    }
}

/**
 * @brief Set bits in Msr Bitmap
 * 
 * @param Msr MSR Address
 * @param ProcessorID Processor to set MSR Bitmap for it
 * @param ReadDetection set read bit 
 * @param WriteDetection set write bit
 * @return BOOLEAN Returns true if the MSR Bitmap is succcessfully applied or false if not applied
 */
BOOLEAN
MsrHandleSetMsrBitmap(UINT64 Msr, INT ProcessorID, BOOLEAN ReadDetection, BOOLEAN WriteDetection)
{
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[ProcessorID];

    if (!ReadDetection && !WriteDetection)
    {
        //
        // Invalid Command
        //
        return FALSE;
    }

    if (Msr <= 0x00001FFF)
    {
        if (ReadDetection)
        {
            SetBit(Msr, CurrentVmState->MsrBitmapVirtualAddress);
        }
        if (WriteDetection)
        {
            SetBit(Msr, CurrentVmState->MsrBitmapVirtualAddress + 2048);
        }
    }
    else if ((0xC0000000 <= Msr) && (Msr <= 0xC0001FFF))
    {
        if (ReadDetection)
        {
            SetBit(Msr - 0xC0000000, CurrentVmState->MsrBitmapVirtualAddress + 1024);
        }
        if (WriteDetection)
        {
            SetBit(Msr - 0xC0000000, CurrentVmState->MsrBitmapVirtualAddress + 3072);
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief UnSet bits in Msr Bitmap
 * 
 * @param Msr MSR Address
 * @param ProcessorID Processor to set MSR Bitmap for it
 * @param ReadDetection Unset read bit 
 * @param WriteDetection Unset write bit
 * @return BOOLEAN Returns true if the MSR Bitmap is succcessfully applied or false if not applied
 */
BOOLEAN
MsrHandleUnSetMsrBitmap(UINT64 Msr, INT ProcessorID, BOOLEAN ReadDetection, BOOLEAN WriteDetection)
{
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[ProcessorID];

    if (!ReadDetection && !WriteDetection)
    {
        //
        // Invalid Command
        //
        return FALSE;
    }

    if (Msr <= 0x00001FFF)
    {
        if (ReadDetection)
        {
            ClearBit(Msr, CurrentVmState->MsrBitmapVirtualAddress);
        }
        if (WriteDetection)
        {
            ClearBit(Msr, CurrentVmState->MsrBitmapVirtualAddress + 2048);
        }
    }
    else if ((0xC0000000 <= Msr) && (Msr <= 0xC0001FFF))
    {
        if (ReadDetection)
        {
            ClearBit(Msr - 0xC0000000, CurrentVmState->MsrBitmapVirtualAddress + 1024);
        }
        if (WriteDetection)
        {
            ClearBit(Msr - 0xC0000000, CurrentVmState->MsrBitmapVirtualAddress + 3072);
        }
    }
    else
    {
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Filter to avoid msr set for MSRs that are
 * not valid or should be ignored (RDMSR)
 * @param CoreIndex
 * @return VOID 
 */
VOID
MsrHandleFilterMsrReadBitmap(UINT32 CoreIndex)
{
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CoreIndex];
    //
    // Ignore IA32_KERNEL_GSBASE (0xC0000102)
    //
    ClearBit(0x102, CurrentVmState->MsrBitmapVirtualAddress + 1024);

    //
    // Ignore IA32_MPERF (0x000000e7), and IA32_APERF (0x000000e8)
    //
    ClearBit(0xe7, CurrentVmState->MsrBitmapVirtualAddress);
    ClearBit(0xe8, CurrentVmState->MsrBitmapVirtualAddress);
}

/**
 * @brief Filter to avoid msr set for MSRs that are
 * not valid or should be ignored (wrmsr)
 * @param CoreIndex
 * @return VOID 
 */
VOID
MsrHandleFilterMsrWriteBitmap(UINT32 CoreIndex)
{
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CoreIndex];

    //
    // Ignore IA32_KERNEL_GSBASE (0xC0000102)
    //
    ClearBit(0x102, CurrentVmState->MsrBitmapVirtualAddress + 3072);

    //
    // Ignore IA32_MPERF (0x000000e7), and IA32_APERF (0x000000e8)
    //
    ClearBit(0xe7, CurrentVmState->MsrBitmapVirtualAddress + 2048);
    ClearBit(0xe8, CurrentVmState->MsrBitmapVirtualAddress + 2048);

    //
    // Ignore IA32_SPEC_CTRL (0x00000048), and IA32_PRED_CMD (0x00000049)
    //
    ClearBit(0x48, CurrentVmState->MsrBitmapVirtualAddress + 2048);
    ClearBit(0x49, CurrentVmState->MsrBitmapVirtualAddress + 2048);
}

/**
 * @brief Change MSR Bitmap for read
 * @details should be called in vmx-root mode
 * @param 
 * @return VOID 
 */
VOID
MsrHandlePerformMsrBitmapReadChange(UINT64 MsrMask)
{
    UINT32                  CoreIndex      = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CoreIndex];

    if (MsrMask == DEBUGGER_EVENT_MSR_READ_OR_WRITE_ALL_MSRS)
    {
        //
        // Means all the bitmaps should be put to 1
        //
        memset(CurrentVmState->MsrBitmapVirtualAddress, 0xff, 2048);

        //
        // Filter MSR Bitmap for special MSRs
        //
        MsrHandleFilterMsrReadBitmap(CoreIndex);
    }
    else
    {
        //
        // Means only one msr bitmap is target
        //
        MsrHandleSetMsrBitmap(MsrMask, CoreIndex, TRUE, FALSE);
    }
}

/**
 * @brief Reset MSR Bitmap for read
 * @details should be called in vmx-root mode
 * @return VOID 
 */
VOID
MsrHandlePerformMsrBitmapReadReset()
{
    UINT32                  CoreIndex      = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CoreIndex];

    //
    // Means all the bitmaps should be put to 0
    //
    memset(CurrentVmState->MsrBitmapVirtualAddress, 0x0, 2048);
}
/**
 * @brief Change MSR Bitmap for write
 * @details should be called in vmx-root mode
 * @param MsrMask MSR
 * @return VOID 
 */
VOID
MsrHandlePerformMsrBitmapWriteChange(UINT64 MsrMask)
{
    UINT32                  CoreIndex      = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CoreIndex];

    if (MsrMask == DEBUGGER_EVENT_MSR_READ_OR_WRITE_ALL_MSRS)
    {
        //
        // Means all the bitmaps should be put to 1
        //
        memset((UINT64)CurrentVmState->MsrBitmapVirtualAddress + 2048, 0xff, 2048);

        //
        // Filter MSR Bitmap for special MSRs
        //
        MsrHandleFilterMsrWriteBitmap(CoreIndex);
    }
    else
    {
        //
        // Means only one msr bitmap is target
        //
        MsrHandleSetMsrBitmap(MsrMask, CoreIndex, FALSE, TRUE);
    }
}

/**
 * @brief Reset MSR Bitmap for write
 * @details should be called in vmx-root mode
 * @return VOID 
 */
VOID
MsrHandlePerformMsrBitmapWriteReset()
{
    UINT32                  CoreIndex      = KeGetCurrentProcessorNumber();
    VIRTUAL_MACHINE_STATE * CurrentVmState = &g_GuestState[CoreIndex];

    //
    // Means all the bitmaps should be put to 0
    //
    memset((UINT64)CurrentVmState->MsrBitmapVirtualAddress + 2048, 0x0, 2048);
}
