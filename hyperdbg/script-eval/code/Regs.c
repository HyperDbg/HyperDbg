/**
 * @file Regs.c
 * @author Alee Amini (alee@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Script engine registers implementations
 * @details
 * @version 0.2
 * @date 2022-06-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Get the register value 
 * 
 * @param GuestRegs 
 * @param RegId 
 * @return UINT64 
 */
UINT64
GetRegValue(PGUEST_REGS GuestRegs, REGS_ENUM RegId)
{
    switch (RegId)
    {
    case REGISTER_RAX:
        return GuestRegs->rax;

        break;

    case REGISTER_EAX:
        return (GuestRegs->rax & LOWER_32_BITS);

        break;

    case REGISTER_AX:
        return (GuestRegs->rax & LOWER_16_BITS);

        break;

    case REGISTER_AH:
        return (GuestRegs->rax & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_AL:
        return (GuestRegs->rax & LOWER_8_BITS);

        break;

    case REGISTER_RCX:
        return GuestRegs->rcx;

        break;

    case REGISTER_ECX:
        return (GuestRegs->rcx & LOWER_32_BITS);

        break;

    case REGISTER_CX:
        return (GuestRegs->rcx & LOWER_16_BITS);

        break;

    case REGISTER_CH:
        return (GuestRegs->rcx & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_CL:
        return (GuestRegs->rcx & LOWER_8_BITS);

        break;

    case REGISTER_RDX:
        return GuestRegs->rdx;

        break;

    case REGISTER_EDX:
        return (GuestRegs->rdx & LOWER_32_BITS);

        break;

    case REGISTER_DX:
        return (GuestRegs->rdx & LOWER_16_BITS);

        break;

    case REGISTER_DH:
        return (GuestRegs->rdx & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_DL:
        return (GuestRegs->rdx & LOWER_8_BITS);

        break;

    case REGISTER_RBX:
        return GuestRegs->rbx;

        break;

    case REGISTER_EBX:
        return (GuestRegs->rbx & LOWER_32_BITS);

        break;

    case REGISTER_BX:
        return (GuestRegs->rbx & LOWER_16_BITS);

        break;

    case REGISTER_BH:
        return (GuestRegs->rbx & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_BL:
        return (GuestRegs->rbx & LOWER_8_BITS);

        break;

    case REGISTER_RSP:
        return GuestRegs->rsp;

        break;

    case REGISTER_ESP:
        return (GuestRegs->rsp & LOWER_32_BITS);

        break;

    case REGISTER_SP:
        return (GuestRegs->rsp & LOWER_16_BITS);

        break;

    case REGISTER_SPL:
        return (GuestRegs->rsp & LOWER_8_BITS);

        break;

    case REGISTER_RBP:
        return GuestRegs->rbp;

        break;

    case REGISTER_EBP:
        return (GuestRegs->rbp & LOWER_32_BITS);

        break;

    case REGISTER_BP:
        return (GuestRegs->rbp & LOWER_16_BITS);

        break;
    case REGISTER_BPL:
        return (GuestRegs->rbp & LOWER_8_BITS);

        break;

    case REGISTER_RSI:
        return GuestRegs->rsi;

        break;

    case REGISTER_ESI:
        return (GuestRegs->rsi & LOWER_32_BITS);

        break;

    case REGISTER_SI:
        return (GuestRegs->rsi & LOWER_16_BITS);

        break;

    case REGISTER_SIL:
        return (GuestRegs->rsi & LOWER_8_BITS);

        break;

    case REGISTER_RDI:
        return GuestRegs->rdi;

        break;

    case REGISTER_EDI:
        return (GuestRegs->rdi & LOWER_32_BITS);

        break;

    case REGISTER_DI:
        return (GuestRegs->rdi & LOWER_16_BITS);

        break;

    case REGISTER_DIL:
        return (GuestRegs->rdi & LOWER_8_BITS);

        break;

    case REGISTER_R8:
        return GuestRegs->r8;

        break;

    case REGISTER_R8D:
        return (GuestRegs->r8 & LOWER_32_BITS);

        break;

    case REGISTER_R8W:
        return (GuestRegs->r8 & LOWER_16_BITS);

        break;

    case REGISTER_R8H:
        return (GuestRegs->r8 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R8L:
        return (GuestRegs->r8 & LOWER_8_BITS);

        break;
    case REGISTER_R9:
        return GuestRegs->r9;

        break;

    case REGISTER_R9D:
        return (GuestRegs->r9 & LOWER_32_BITS);

        break;

    case REGISTER_R9W:
        return (GuestRegs->r9 & LOWER_16_BITS);

        break;

    case REGISTER_R9H:
        return (GuestRegs->r9 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R9L:
        return (GuestRegs->r9 & LOWER_8_BITS);

        break;

    case REGISTER_R10:
        return GuestRegs->r10;

        break;

    case REGISTER_R10D:
        return (GuestRegs->r10 & LOWER_32_BITS);

        break;

    case REGISTER_R10W:
        return (GuestRegs->r10 & LOWER_16_BITS);

        break;

    case REGISTER_R10H:
        return (GuestRegs->r10 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R10L:
        return (GuestRegs->r10 & LOWER_8_BITS);

        break;

    case REGISTER_R11:
        return GuestRegs->r11;

        break;

    case REGISTER_R11D:
        return (GuestRegs->r11 & LOWER_32_BITS);

        break;

    case REGISTER_R11W:
        return (GuestRegs->r11 & LOWER_16_BITS);

        break;

    case REGISTER_R11H:
        return (GuestRegs->r11 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R11L:
        return (GuestRegs->r11 & LOWER_8_BITS);

        break;

    case REGISTER_R12:
        return GuestRegs->r12;

        break;

    case REGISTER_R12D:
        return (GuestRegs->r12 & LOWER_32_BITS);

        break;

    case REGISTER_R12W:
        return (GuestRegs->r12 & LOWER_16_BITS);

        break;

    case REGISTER_R12H:
        return (GuestRegs->r12 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R12L:
        return (GuestRegs->r12 & LOWER_8_BITS);

        break;

    case REGISTER_R13:
        return GuestRegs->r13;

        break;

    case REGISTER_R13D:
        return (GuestRegs->r13 & LOWER_32_BITS);

        break;

    case REGISTER_R13W:
        return (GuestRegs->r13 & LOWER_16_BITS);

        break;

    case REGISTER_R13H:
        return (GuestRegs->r13 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R13L:
        return (GuestRegs->r13 & LOWER_8_BITS);

        break;

    case REGISTER_R14:
        return GuestRegs->r14;

        break;

    case REGISTER_R14D:
        return (GuestRegs->r14 & LOWER_32_BITS);

        break;

    case REGISTER_R14W:
        return (GuestRegs->r14 & LOWER_16_BITS);

        break;

    case REGISTER_R14H:
        return (GuestRegs->r14 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R14L:
        return (GuestRegs->r14 & LOWER_8_BITS);

        break;

    case REGISTER_R15:
        return GuestRegs->r15;

        break;

    case REGISTER_R15D:
        return (GuestRegs->r15 & LOWER_32_BITS);

        break;

    case REGISTER_R15W:
        return (GuestRegs->r15 & LOWER_16_BITS);

        break;

    case REGISTER_R15H:
        return (GuestRegs->r15 & SECOND_LOWER_8_BITS) >> 8;

        break;

    case REGISTER_R15L:
        return (GuestRegs->r15 & LOWER_8_BITS);

        break;

    case REGISTER_DS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ES:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestEs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_FS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestFs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_GS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestGs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestSs().Selector;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RFLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestRFlags();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_EFLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & LOWER_32_BITS);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_FLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & LOWER_16_BITS);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & X86_FLAGS_CF) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_PF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_PF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_AF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_AF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ZF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_ZF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_SF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_TF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_TF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_IF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_DF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_OF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_OF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IOPL:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return ((GetGuestRFlags() & (0b11 << X86_FLAGS_IOPL_SHIFT)) >> 12);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_NT:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_NT)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_RF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VM:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_VM)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_AC:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_AC)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VIF:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_VIF)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_VIP)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ID:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRFlags() & (X86_FLAGS_ID)) != NULL ? TRUE : FALSE;
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestRIP();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_EIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRIP() & LOWER_32_BITS);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IP:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return (GetGuestRIP() & LOWER_16_BITS);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

        return GetGuestIdtr();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_LDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

        return GetGuestLdtr();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_TR:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE

        return GetGuestTr();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_GDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestGdtr();

#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR0:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr0();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR2:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr2();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR3:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr3();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR4:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr4();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR8:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestCr8();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR0:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr0();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR1:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr1();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR2:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr2();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR3:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr3();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR6:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr6();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR7:

#ifdef SCRIPT_ENGINE_USER_MODE
        return NULL;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        return GetGuestDr7();
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case INVALID:

#ifdef SCRIPT_ENGINE_USER_MODE
        ShowMessages("error in reading register");
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        LogInfo("Error in reading register");
#endif // SCRIPT_ENGINE_KERNEL_MODE

        return INVALID;

        break;
    }
}

/**
 * @brief Set the register value
 * 
 * @param GuestRegs 
 * @param Symbol 
 * @param Value 
 * @return VOID 
 */
VOID
SetRegValue(PGUEST_REGS GuestRegs, PSYMBOL Symbol, UINT64 Value)
{
    switch (Symbol->Value)
    {
    case REGISTER_RAX:
        GuestRegs->rax = Value;

        break;

    case REGISTER_EAX:
        GuestRegs->rax = (GuestRegs->rax & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_AX:
        GuestRegs->rax = (GuestRegs->rax & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_AH:
        GuestRegs->rax = (GuestRegs->rax & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_AL:
        GuestRegs->rax = (GuestRegs->rax & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;

    case REGISTER_RCX:
        GuestRegs->rcx = Value;

        break;
    case REGISTER_ECX:
        GuestRegs->rcx = (GuestRegs->rcx & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_CX:
        GuestRegs->rcx = (GuestRegs->rcx & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_CH:
        GuestRegs->rcx = (GuestRegs->rcx & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_CL:
        GuestRegs->rcx = (GuestRegs->rcx & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RDX:
        GuestRegs->rdx = Value;

        break;
    case REGISTER_EDX:
        GuestRegs->rdx = (GuestRegs->rdx & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_DX:
        GuestRegs->rdx = (GuestRegs->rdx & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_DH:
        GuestRegs->rdx = (GuestRegs->rdx & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_DL:
        GuestRegs->rdx = (GuestRegs->rdx & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RBX:
        GuestRegs->rbx = Value;

        break;
    case REGISTER_EBX:
        GuestRegs->rbx = (GuestRegs->rbx & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_BX:
        GuestRegs->rbx = (GuestRegs->rbx & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_BH:
        GuestRegs->rbx = (GuestRegs->rbx & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_BL:
        GuestRegs->rbx = (GuestRegs->rbx & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RSP:

#ifdef SCRIPT_ENGINE_USER_MODE
        GuestRegs->rsp = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        GuestRegs->rsp = Value;
        SetGuestRSP(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ESP:

#ifdef SCRIPT_ENGINE_USER_MODE
        GuestRegs->rsp = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        GuestRegs->rsp = (GuestRegs->rsp & UPPER_32_BITS) | (Value & LOWER_32_BITS);
        SetGuestRSP(GuestRegs->rsp);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SP:

#ifdef SCRIPT_ENGINE_USER_MODE
        GuestRegs->rsp = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        GuestRegs->rsp = (GuestRegs->rsp & UPPER_48_BITS) | (Value & LOWER_16_BITS);
        SetGuestRSP(GuestRegs->rsp);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SPL:

#ifdef SCRIPT_ENGINE_USER_MODE
        GuestRegs->rsp = Value;
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        GuestRegs->rsp = (GuestRegs->rsp & UPPER_56_BITS) | (Value & LOWER_8_BITS);
        SetGuestRSP(GuestRegs->rsp);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RBP:
        GuestRegs->rbp = Value;

        break;
    case REGISTER_EBP:
        GuestRegs->rbp = (GuestRegs->rbp & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_BP:
        GuestRegs->rbp = (GuestRegs->rbp & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_BPL:
        GuestRegs->rbp = (GuestRegs->rbp & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RSI:
        GuestRegs->rsi = Value;

        break;
    case REGISTER_ESI:
        GuestRegs->rsi = (GuestRegs->rsi & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_SI:
        GuestRegs->rsi = (GuestRegs->rsi & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_SIL:
        GuestRegs->rsi = (GuestRegs->rsi & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_RDI:
        GuestRegs->rdi = Value;

        break;
    case REGISTER_EDI:
        GuestRegs->rdi = (GuestRegs->rdi & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_DI:
        GuestRegs->rdi = (GuestRegs->rdi & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_DIL:
        GuestRegs->rdi = (GuestRegs->rdi & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R8:
        GuestRegs->r8 = Value;

        break;
    case REGISTER_R8D:
        GuestRegs->r8 = (GuestRegs->r8 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R8W:
        GuestRegs->r8 = (GuestRegs->r8 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R8H:
        GuestRegs->r8 = (GuestRegs->r8 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R8L:
        GuestRegs->r8 = (GuestRegs->r8 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R9:
        GuestRegs->r9 = Value;

        break;
    case REGISTER_R9D:
        GuestRegs->r9 = (GuestRegs->r9 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R9W:
        GuestRegs->r9 = (GuestRegs->r9 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R9H:
        GuestRegs->r9 = (GuestRegs->r9 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R9L:
        GuestRegs->r9 = (GuestRegs->r9 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R10:
        GuestRegs->r10 = Value;

        break;
    case REGISTER_R10D:
        GuestRegs->r10 = (GuestRegs->r10 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R10W:
        GuestRegs->r10 = (GuestRegs->r10 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R10H:
        GuestRegs->r10 = (GuestRegs->r10 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R10L:
        GuestRegs->r10 = (GuestRegs->r10 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R11:
        GuestRegs->r11 = Value;

        break;
    case REGISTER_R11D:
        GuestRegs->r11 = (GuestRegs->r11 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R11W:
        GuestRegs->r11 = (GuestRegs->r11 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R11H:
        GuestRegs->r11 = (GuestRegs->r11 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R11L:
        GuestRegs->r11 = (GuestRegs->r11 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R12:
        GuestRegs->r12 = Value;

        break;
    case REGISTER_R12D:
        GuestRegs->r12 = (GuestRegs->r12 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R12W:
        GuestRegs->r12 = (GuestRegs->r12 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R12H:
        GuestRegs->r12 = (GuestRegs->r12 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R12L:
        GuestRegs->r12 = (GuestRegs->r12 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R13:
        GuestRegs->r13 = Value;

        break;
    case REGISTER_R13D:
        GuestRegs->r13 = (GuestRegs->r13 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R13W:
        GuestRegs->r13 = (GuestRegs->r13 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R13H:
        GuestRegs->r13 = (GuestRegs->r13 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R13L:
        GuestRegs->r13 = (GuestRegs->r13 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R14:
        GuestRegs->r14 = Value;

        break;
    case REGISTER_R14D:
        GuestRegs->r14 = (GuestRegs->r14 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R14W:
        GuestRegs->r14 = (GuestRegs->r14 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R14H:
        GuestRegs->r14 = (GuestRegs->r14 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R14L:
        GuestRegs->r14 = (GuestRegs->r14 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_R15:
        GuestRegs->r15 = Value;

        break;
    case REGISTER_R15D:
        GuestRegs->r15 = (GuestRegs->r15 & UPPER_32_BITS) | (Value & LOWER_32_BITS);

        break;

    case REGISTER_R15W:
        GuestRegs->r15 = (GuestRegs->r15 & UPPER_48_BITS) | (Value & LOWER_16_BITS);

        break;

    case REGISTER_R15H:
        GuestRegs->r15 = (GuestRegs->r15 & UPPER_48_BITS_AND_LOWER_8_BITS) | ((Value << 8) & SECOND_LOWER_8_BITS);

        break;

    case REGISTER_R15L:
        GuestRegs->r15 = (GuestRegs->r15 & UPPER_56_BITS) | (Value & LOWER_8_BITS);

        break;
    case REGISTER_DS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestDsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ES:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestEsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_FS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestFsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_GS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestGsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestCsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        Value = Value & LOWER_16_BITS;
        SetGuestSsSel((PVMX_SEGMENT_SELECTOR)&Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RFLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_EFLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags((GetGuestRFlags() & UPPER_32_BITS) | (Value & LOWER_32_BITS));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_FLAGS:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags((GetGuestRFlags() & UPPER_48_BITS) | (Value & LOWER_16_BITS));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_CF) : GetGuestRFlags() & (~(X86_FLAGS_CF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_PF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_PF) : GetGuestRFlags() & (~(X86_FLAGS_PF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_AF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_AF) : GetGuestRFlags() & (~(X86_FLAGS_AF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ZF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_ZF) : GetGuestRFlags() & (~(X86_FLAGS_ZF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_SF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_SF) : GetGuestRFlags() & (~(X86_FLAGS_SF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_TF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_TF) : GetGuestRFlags() & (~(X86_FLAGS_TF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_IF) : GetGuestRFlags() & (~(X86_FLAGS_IF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_DF) : GetGuestRFlags() & (~(X86_FLAGS_DF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_OF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_OF) : GetGuestRFlags() & (~(X86_FLAGS_OF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IOPL:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (1 << X86_FLAGS_IOPL_SHIFT) : GetGuestRFlags() & (~(1 << X86_FLAGS_IOPL_SHIFT)));
        Value = (Value >> 4) & 1;
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (1 << X86_FLAGS_IOPL_SHIFT_2ND_BIT) : GetGuestRFlags() & (~(1 << X86_FLAGS_IOPL_SHIFT_2ND_BIT)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_NT:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_NT) : GetGuestRFlags() & (~(X86_FLAGS_NT)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_RF) : GetGuestRFlags() & (~(X86_FLAGS_RF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VM:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_VM) : GetGuestRFlags() & (~(X86_FLAGS_VM)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_AC:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_AC) : GetGuestRFlags() & (~(X86_FLAGS_AC)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VIF:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_VIF) : GetGuestRFlags() & (~(X86_FLAGS_VIF)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_VIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_VIP) : GetGuestRFlags() & (~(X86_FLAGS_VIP)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_ID:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRFlags(Value & TRUE ? GetGuestRFlags() | (X86_FLAGS_ID) : GetGuestRFlags() & (~(X86_FLAGS_ID)));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_RIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRIP(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_EIP:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRIP((GetGuestRIP() & UPPER_32_BITS) | (Value & LOWER_32_BITS));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IP:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestRIP((GetGuestRIP() & UPPER_48_BITS) | (Value & LOWER_16_BITS));
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_IDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestIdtr(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_LDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestLdtr(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_GDTR:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestGdtr(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_TR:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestTr(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR0:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr0(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR2:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr2(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR3:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr3(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR4:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr4(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_CR8:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestCr8(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR0:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr0(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR1:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr1(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR2:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr2(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR3:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr3(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR6:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr6(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;

    case REGISTER_DR7:

#ifdef SCRIPT_ENGINE_USER_MODE
        //
        // Nothing to do
        //
#endif // SCRIPT_ENGINE_USER_MODE

#ifdef SCRIPT_ENGINE_KERNEL_MODE
        SetGuestDr7(Value);
#endif // SCRIPT_ENGINE_KERNEL_MODE

        break;
    }
}
