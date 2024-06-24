/**
 * @file ManageRegs.c
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author Alee Amini (alee@hyperdbg.org)
 * @brief Manage Registers
 * @details
 * @version 0.1
 * @date 2020-06-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Set just the Guest Cs selector
 *
 * @param Cs The CS Selector for the guest
 * @return VOID
 */
VOID
SetGuestCsSel(PVMX_SEGMENT_SELECTOR Cs)
{
    __vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, Cs->Selector);
}

/**
 * @brief Set the Guest Cs
 *
 * @param Cs The CS Selector for the guest
 * @return VOID
 */

VOID
SetGuestCs(PVMX_SEGMENT_SELECTOR Cs)
{
    __vmx_vmwrite(VMCS_GUEST_CS_BASE, Cs->Base);
    __vmx_vmwrite(VMCS_GUEST_CS_LIMIT, Cs->Limit);
    __vmx_vmwrite(VMCS_GUEST_CS_ACCESS_RIGHTS, Cs->Attributes.AsUInt);
    __vmx_vmwrite(VMCS_GUEST_CS_SELECTOR, Cs->Selector);
}

/**
 * @brief Get the Guest Cs Selector
 *
 * @return SEGMENT_SELECTOR
 */
VMX_SEGMENT_SELECTOR
GetGuestCs()
{
    VMX_SEGMENT_SELECTOR Cs;

    __vmx_vmread(VMCS_GUEST_CS_BASE, &Cs.Base);
    VmxVmread32P(VMCS_GUEST_CS_LIMIT, &Cs.Limit);
    VmxVmread32P(VMCS_GUEST_CS_ACCESS_RIGHTS, &Cs.Attributes.AsUInt);
    VmxVmread16P(VMCS_GUEST_CS_SELECTOR, &Cs.Selector);

    return Cs;
}

/**
 * @brief Set just the Guest Ss selector
 *
 * @param Ss The SS Selector for the guest
 * @return VOID
 */
VOID
SetGuestSsSel(PVMX_SEGMENT_SELECTOR Ss)
{
    __vmx_vmwrite(VMCS_GUEST_SS_SELECTOR, Ss->Selector);
}

/**
 * @brief Set the Guest Ss selector
 *
 * @param Ss The SS Selector for the guest
 * @return VOID
 */
VOID
SetGuestSs(PVMX_SEGMENT_SELECTOR Ss)
{
    VmxVmwrite64(VMCS_GUEST_SS_BASE, Ss->Base);
    VmxVmwrite64(VMCS_GUEST_SS_LIMIT, Ss->Limit);
    VmxVmwrite64(VMCS_GUEST_SS_ACCESS_RIGHTS, Ss->Attributes.AsUInt);
    VmxVmwrite64(VMCS_GUEST_SS_SELECTOR, Ss->Selector);
}

/**
 * @brief Get the Guest Ss Selector
 *
 * @return SEGMENT_SELECTOR
 */
VMX_SEGMENT_SELECTOR
GetGuestSs()
{
    VMX_SEGMENT_SELECTOR Ss;

    __vmx_vmread(VMCS_GUEST_SS_BASE, &Ss.Base);
    VmxVmread32P(VMCS_GUEST_SS_LIMIT, &Ss.Limit);
    VmxVmread32P(VMCS_GUEST_SS_ACCESS_RIGHTS, &Ss.Attributes.AsUInt);
    VmxVmread16P(VMCS_GUEST_SS_SELECTOR, &Ss.Selector);

    return Ss;
}

/**
 * @brief Set just the Guest Ds selector
 *
 * @param Ds The DS Selector for the guest
 * @return VOID
 */
VOID
SetGuestDsSel(PVMX_SEGMENT_SELECTOR Ds)
{
    VmxVmwrite64(VMCS_GUEST_DS_SELECTOR, Ds->Selector);
}

/**
 * @brief Set the Guest Ds selector
 *
 * @param Ds The DS Selector for the guest
 * @return VOID
 */
VOID
SetGuestDs(PVMX_SEGMENT_SELECTOR Ds)
{
    VmxVmwrite64(VMCS_GUEST_DS_BASE, Ds->Base);
    VmxVmwrite64(VMCS_GUEST_DS_LIMIT, Ds->Limit);
    VmxVmwrite64(VMCS_GUEST_DS_ACCESS_RIGHTS, Ds->Attributes.AsUInt);
    VmxVmwrite64(VMCS_GUEST_DS_SELECTOR, Ds->Selector);
}

/**
 * @brief Get the Guest Ds Selector
 *
 * @return SEGMENT_SELECTOR
 */
VMX_SEGMENT_SELECTOR
GetGuestDs()
{
    VMX_SEGMENT_SELECTOR Ds;

    __vmx_vmread(VMCS_GUEST_DS_BASE, &Ds.Base);
    VmxVmread32P(VMCS_GUEST_DS_LIMIT, &Ds.Limit);
    VmxVmread32P(VMCS_GUEST_DS_ACCESS_RIGHTS, &Ds.Attributes.AsUInt);
    VmxVmread16P(VMCS_GUEST_DS_SELECTOR, &Ds.Selector);

    return Ds;
}

/**
 * @brief Set just the Guest Fs selector
 *
 * @param Fs The FS Selector for the guest
 * @return VOID
 */
VOID
SetGuestFsSel(PVMX_SEGMENT_SELECTOR Fs)
{
    VmxVmwrite64(VMCS_GUEST_FS_SELECTOR, Fs->Selector);
}

/**
 * @brief Set the Guest Fs selector
 *
 * @param Fs The FS Selector for the guest
 * @return VOID
 */
VOID
SetGuestFs(PVMX_SEGMENT_SELECTOR Fs)
{
    VmxVmwrite64(VMCS_GUEST_FS_BASE, Fs->Base);
    VmxVmwrite64(VMCS_GUEST_FS_LIMIT, Fs->Limit);
    VmxVmwrite64(VMCS_GUEST_FS_ACCESS_RIGHTS, Fs->Attributes.AsUInt);
    VmxVmwrite64(VMCS_GUEST_FS_SELECTOR, Fs->Selector);
}

/**
 * @brief Get the Guest Fs Selector
 *
 * @return SEGMENT_SELECTOR
 */
VMX_SEGMENT_SELECTOR
GetGuestFs()
{
    VMX_SEGMENT_SELECTOR Fs;

    __vmx_vmread(VMCS_GUEST_FS_BASE, &Fs.Base);
    VmxVmread32P(VMCS_GUEST_FS_LIMIT, &Fs.Limit);
    VmxVmread32P(VMCS_GUEST_FS_ACCESS_RIGHTS, &Fs.Attributes.AsUInt);
    VmxVmread16P(VMCS_GUEST_FS_SELECTOR, &Fs.Selector);

    return Fs;
}

/**
 * @brief Set just the Guest Gs selector
 *
 * @param Gs The GS Selector for the guest
 * @return VOID
 */
VOID
SetGuestGsSel(PVMX_SEGMENT_SELECTOR Gs)
{
    VmxVmwrite64(VMCS_GUEST_GS_SELECTOR, Gs->Selector);
}

/**
 * @brief Set the Guest Gs selector
 *
 * @param Gs The GS Selector for the guest
 * @return VOID
 */
VOID
SetGuestGs(PVMX_SEGMENT_SELECTOR Gs)
{
    VmxVmwrite64(VMCS_GUEST_GS_BASE, Gs->Base);
    VmxVmwrite64(VMCS_GUEST_GS_LIMIT, Gs->Limit);
    VmxVmwrite64(VMCS_GUEST_GS_ACCESS_RIGHTS, Gs->Attributes.AsUInt);
    VmxVmwrite64(VMCS_GUEST_GS_SELECTOR, Gs->Selector);
}

/**
 * @brief Get the Guest Gs Selector
 *
 * @return SEGMENT_SELECTOR
 */
VMX_SEGMENT_SELECTOR
GetGuestGs()
{
    VMX_SEGMENT_SELECTOR Gs;

    __vmx_vmread(VMCS_GUEST_GS_BASE, &Gs.Base);
    VmxVmread32P(VMCS_GUEST_GS_LIMIT, &Gs.Limit);
    VmxVmread32P(VMCS_GUEST_GS_ACCESS_RIGHTS, &Gs.Attributes.AsUInt);
    VmxVmread16P(VMCS_GUEST_GS_SELECTOR, &Gs.Selector);

    return Gs;
}

/**
 * @brief Set just the Guest Es selector
 *
 * @param Es The ES Selector for the guest
 * @return VOID
 */
VOID
SetGuestEsSel(PVMX_SEGMENT_SELECTOR Es)
{
    VmxVmwrite64(VMCS_GUEST_ES_SELECTOR, Es->Selector);
}

/**
 * @brief Set the Guest Es selector
 *
 * @param Es The ES Selector for the guest
 * @return VOID
 */
VOID
SetGuestEs(PVMX_SEGMENT_SELECTOR Es)
{
    VmxVmwrite64(VMCS_GUEST_ES_BASE, Es->Base);
    VmxVmwrite64(VMCS_GUEST_ES_LIMIT, Es->Limit);
    VmxVmwrite64(VMCS_GUEST_ES_ACCESS_RIGHTS, Es->Attributes.AsUInt);
    VmxVmwrite64(VMCS_GUEST_ES_SELECTOR, Es->Selector);
}

/**
 * @brief Get the Guest Es Selector
 *
 * @return SEGMENT_SELECTOR
 */
VMX_SEGMENT_SELECTOR
GetGuestEs()
{
    VMX_SEGMENT_SELECTOR Es;

    __vmx_vmread(VMCS_GUEST_ES_BASE, &Es.Base);
    VmxVmread32P(VMCS_GUEST_ES_LIMIT, &Es.Limit);
    VmxVmread32P(VMCS_GUEST_ES_ACCESS_RIGHTS, &Es.Attributes.AsUInt);
    VmxVmread16P(VMCS_GUEST_ES_SELECTOR, &Es.Selector);

    return Es;
}

/**
 * @brief Set the Guest Idtr
 *
 * @param Idtr The Idtr Selector for the guest
 * @return VOID
 */
VOID
SetGuestIdtr(UINT64 Idtr)
{
    VmxVmwrite64(VMCS_GUEST_IDTR_BASE, Idtr);
}

/**
 * @brief Get the Guest Idtr
 *
 * @return UINT64
 */
UINT64
GetGuestIdtr()
{
    UINT64 Idtr;

    __vmx_vmread(VMCS_GUEST_IDTR_BASE, &Idtr);

    return Idtr;
}

/**
 * @brief Set the Guest Ldtr
 *
 * @param Ldtr The Idtr Selector for the guest
 * @return VOID
 */
VOID
SetGuestLdtr(UINT64 Ldtr)
{
    VmxVmwrite64(VMCS_GUEST_LDTR_BASE, Ldtr);
}

/**
 * @brief Get the Guest Ldtr
 *
 * @return UINT64
 */
UINT64
GetGuestLdtr()
{
    UINT64 Ldtr;

    __vmx_vmread(VMCS_GUEST_LDTR_BASE, &Ldtr);

    return Ldtr;
}

/**
 * @brief Set the Guest Gdtr
 *
 * @param Gdtr The Gdtr Selector for the guest
 * @return VOID
 */
VOID
SetGuestGdtr(UINT64 Gdtr)
{
    VmxVmwrite64(VMCS_GUEST_GDTR_BASE, Gdtr);
}

/**
 * @brief Get the Guest Gdtr
 *
 * @return UINT64
 */
UINT64
GetGuestGdtr()
{
    UINT64 Gdtr;

    __vmx_vmread(VMCS_GUEST_GDTR_BASE, &Gdtr);

    return Gdtr;
}

/**
 * @param Tr The Tr Selector for the guest
 * @return VOID
 */
VOID
SetGuestTr(UINT64 Tr)
{
    VmxVmwrite64(VMCS_GUEST_TR_BASE, Tr);
}

/**
 * @brief Get the Guest Tr
 *
 * @return UINT64
 */
UINT64
GetGuestTr()
{
    UINT64 Tr;

    __vmx_vmread(VMCS_GUEST_TR_BASE, &Tr);

    return Tr;
}
/**
 * @brief Set the Guest RFLAGS Register
 *
 * @param Rflags The Rflags Value for the guest
 * @return VOID
 */
VOID
SetGuestRFlags(UINT64 RFlags)
{
    VmxVmwrite64(VMCS_GUEST_RFLAGS, RFlags);
}

/**
 * @brief Get the Guest Rflags value
 *
 * @return UINT64
 */
UINT64
GetGuestRFlags()
{
    UINT64 RFlags;
    __vmx_vmread(VMCS_GUEST_RFLAGS, &RFlags);
    return RFlags;
}

/**
 * @brief Set the Guest RIP Register
 *
 * @param RIP The RIP Value for the guest
 * @return VOID
 */
VOID
SetGuestRIP(UINT64 RIP)
{
    VmxVmwrite64(VMCS_GUEST_RIP, RIP);
}

/**
 * @brief Set the Guest RSP Register
 *
 * @param RSP The RSP Value for the guest
 * @return VOID
 */
VOID
SetGuestRSP(UINT64 RSP)
{
    VmxVmwrite64(VMCS_GUEST_RSP, RSP);
}

/**
 * @brief Get the Guest RIP value
 *
 * @return UINT64
 */
UINT64
GetGuestRIP()
{
    UINT64 RIP;

    __vmx_vmread(VMCS_GUEST_RIP, &RIP);
    return RIP;
}

/**
 * @brief Get the Guest Cr0 value
 *
 * @return UINT64
 */
UINT64
GetGuestCr0()
{
    UINT64 Cr0;

    __vmx_vmread(VMCS_GUEST_CR0, &Cr0);
    return Cr0;
}

/**
 * @brief Get the Guest Cr2 value
 *
 * @return UINT64
 */
UINT64
GetGuestCr2()
{
    UINT64 Cr2;

    Cr2 = __readcr2();
    return Cr2;
}

/**
 * @brief Get the Guest Cr3 value
 *
 * @return UINT64
 */
UINT64
GetGuestCr3()
{
    UINT64 Cr3;

    __vmx_vmread(VMCS_GUEST_CR3, &Cr3);
    return Cr3;
}

/**
 * @brief Get the Guest Cr4 value
 *
 * @return UINT64
 */
UINT64
GetGuestCr4()
{
    UINT64 Cr4;

    __vmx_vmread(VMCS_GUEST_CR4, &Cr4);
    return Cr4;
}

/**
 * @brief Get the Guest Cr8 value
 *
 * @return UINT64
 */
UINT64
GetGuestCr8()
{
    UINT64 Cr8;

    Cr8 = __readcr8();
    return Cr8;
}

/**
 * @brief Set the Guest Cr0 Register
 *
 * @param Cr0 The Cr0 Value for the guest
 * @return VOID
 */
VOID
SetGuestCr0(UINT64 Cr0)
{
    VmxVmwrite64(VMCS_GUEST_CR0, Cr0);
}

/**
 * @brief Set the Guest Cr2 Register
 *
 * @param Cr2 The Cr2 Value for the guest
 * @return VOID
 */
VOID
SetGuestCr2(UINT64 Cr2)
{
    __writecr2(Cr2);
}

/**
 * @brief Set the Guest Cr3 Register
 *
 * @param Cr3 The Cr3 Value for the guest
 * @return VOID
 */
VOID
SetGuestCr3(UINT64 Cr3)
{
    VmxVmwrite64(VMCS_GUEST_CR3, Cr3);
}

/**
 * @brief Set the Guest Cr4 Register
 *
 * @param Cr4 The Cr4 Value for the guest
 * @return VOID
 */
VOID
SetGuestCr4(UINT64 Cr4)
{
    VmxVmwrite64(VMCS_GUEST_CR4, Cr4);
}

/**
 * @brief Set the Guest Cr8 Register
 *
 * @param Cr8 The Cr8 Value for the guest
 * @return VOID
 */
VOID
SetGuestCr8(UINT64 Cr8)
{
    __writecr8(Cr8);
}

/**
 * @brief Set the Guest Dr0 Register
 *
 * @param Dr0 The Dr0 Value for the guest
 * @return VOID
 */
VOID
SetGuestDr0(UINT64 value)
{
    __writedr(0, value);
}

/**
 * @brief Set the Guest Dr1 Register
 *
 * @param Dr1 The Dr1 Value for the guest
 * @return VOID
 */
VOID
SetGuestDr1(UINT64 value)
{
    __writedr(1, value);
}

/**
 * @brief Set the Guest Dr2 Register
 *
 * @param Dr2 The Dr2 Value for the guest
 * @return VOID
 */
VOID
SetGuestDr2(UINT64 value)
{
    __writedr(2, value);
}

/**
 * @brief Set the Guest Dr3 Register
 *
 * @param Dr3 The Dr3 Value for the guest
 * @return VOID
 */
VOID
SetGuestDr3(UINT64 value)
{
    __writedr(3, value);
}

/**
 * @brief Set the Guest Dr6 Register
 *
 * @param Dr6 The Dr6 Value for the guest
 * @return VOID
 */
VOID
SetGuestDr6(UINT64 value)
{
    __writedr(6, value);
}

/**
 * @brief Set the Guest Dr7 Register
 *
 * @param Dr7 The Dr7 Value for the guest
 * @return VOID
 */
VOID
SetGuestDr7(UINT64 value)
{
    __writedr(7, value);
}

/**
 * @brief Get the Guest Dr0 value
 *
 * @return UINT64
 */
UINT64
GetGuestDr0()
{
    UINT64 Dr0 = 0;
    Dr0        = __readdr(0);
    return Dr0;
}

/**
 * @brief Get the Guest Dr1 value
 *
 * @return UINT64
 */
UINT64
GetGuestDr1()
{
    UINT64 Dr1 = 0;
    Dr1        = __readdr(1);
    return Dr1;
}

/**
 * @brief Get the Guest Dr2 value
 *
 * @return UINT64
 */
UINT64
GetGuestDr2()
{
    UINT64 Dr2 = 0;
    Dr2        = __readdr(2);
    return Dr2;
}

/**
 * @brief Get the Guest Dr3 value
 *
 * @return UINT64
 */
UINT64
GetGuestDr3()
{
    UINT64 Dr3 = 0;
    Dr3        = __readdr(3);
    return Dr3;
}

/**
 * @brief Get the Guest Dr6 (breakpoint status) value
 *
 * @return UINT64
 */
UINT64
GetGuestDr6()
{
    UINT64 Dr6 = 0;
    Dr6        = __readdr(6);
    return Dr6;
}

/**
 * @brief Get the Guest Dr7 (breakpoint trigger) value
 *
 * @return UINT64
 */
UINT64
GetGuestDr7()
{
    UINT64 Dr7 = 0;
    Dr7        = __readdr(7);
    return Dr7;
}
