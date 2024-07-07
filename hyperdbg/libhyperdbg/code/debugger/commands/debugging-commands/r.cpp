/**
 * @file r.cpp
 * @author Alee Amini (alee@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief r command
 * @details
 * @version 0.1
 * @date 2021-02-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

// using namespace std;

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

std::map<std::string, REGS_ENUM> RegistersMap = {
    {"rax", REGISTER_RAX},
    {"eax", REGISTER_EAX},
    {"ax", REGISTER_AX},
    {"ah", REGISTER_AH},
    {"al", REGISTER_AL},
    {"rbx", REGISTER_RBX},
    {"ebx", REGISTER_EBX},
    {"bx", REGISTER_BX},
    {"bh", REGISTER_BH},
    {"bl", REGISTER_BL},
    {"rcx", REGISTER_RCX},
    {"ecx", REGISTER_ECX},
    {"cx", REGISTER_CX},
    {"ch", REGISTER_CH},
    {"cl", REGISTER_CL},
    {"rdx", REGISTER_RDX},
    {"edx", REGISTER_EDX},
    {"dx", REGISTER_DX},
    {"dh", REGISTER_DH},
    {"dl", REGISTER_DL},
    {"rsi", REGISTER_RSI},
    {"esi", REGISTER_ESI},
    {"si", REGISTER_SI},
    {"sil", REGISTER_SIL},
    {"rdi", REGISTER_RDI},
    {"edi", REGISTER_EDI},
    {"di", REGISTER_DI},
    {"dil", REGISTER_DIL},
    {"rbp", REGISTER_RBP},
    {"ebp", REGISTER_EBP},
    {"bp", REGISTER_BP},
    {"bpl", REGISTER_BPL},
    {"rsp", REGISTER_RSP},
    {"esp", REGISTER_ESP},
    {"sp", REGISTER_SP},
    {"spl", REGISTER_SPL},
    {"r8", REGISTER_R8},
    {"r8d", REGISTER_R8D},
    {"r8w", REGISTER_R8W},
    {"r8h", REGISTER_R8H},
    {"r8l", REGISTER_R8L},
    {"r9", REGISTER_R9},
    {"r9d", REGISTER_R9D},
    {"r9w", REGISTER_R9W},
    {"r9h", REGISTER_R9H},
    {"r9l", REGISTER_R9L},
    {"r10", REGISTER_R10},
    {"r10d", REGISTER_R10D},
    {"r10w", REGISTER_R10W},
    {"r10h", REGISTER_R10H},
    {"r10l", REGISTER_R10L},
    {"r11", REGISTER_R11},
    {"r11d", REGISTER_R11D},
    {"r11w", REGISTER_R11W},
    {"r11h", REGISTER_R11H},
    {"r11l", REGISTER_R11L},
    {"r12", REGISTER_R12},
    {"r12d", REGISTER_R12D},
    {"r12w", REGISTER_R12W},
    {"r12h", REGISTER_R12H},
    {"r12l", REGISTER_R12L},
    {"r13", REGISTER_R13},
    {"r13d", REGISTER_R13D},
    {"r13w", REGISTER_R13W},
    {"r13h", REGISTER_R13H},
    {"r13l", REGISTER_R13L},
    {"r14", REGISTER_R14},
    {"r14d", REGISTER_R14D},
    {"r14w", REGISTER_R14W},
    {"r14h", REGISTER_R14H},
    {"r14l", REGISTER_R14L},
    {"r15", REGISTER_R15},
    {"r15d", REGISTER_R15D},
    {"r15w", REGISTER_R15W},
    {"r15h", REGISTER_R15H},
    {"r15l", REGISTER_R15L},
    {"ds", REGISTER_DS},
    {"es", REGISTER_ES},
    {"fs", REGISTER_FS},
    {"gs", REGISTER_GS},
    {"cs", REGISTER_CS},
    {"ss", REGISTER_SS},
    {"rflags", REGISTER_RFLAGS},
    {"eflags", REGISTER_EFLAGS},
    {"flags", REGISTER_FLAGS},
    {"cf", REGISTER_CF},
    {"pf", REGISTER_PF},
    {"af", REGISTER_AF},
    {"zf", REGISTER_ZF},
    {"sf", REGISTER_SF},
    {"tf", REGISTER_TF},
    {"if", REGISTER_IF},
    {"df", REGISTER_DF},
    {"of", REGISTER_OF},
    {"iopl", REGISTER_IOPL},
    {"nt", REGISTER_NT},
    {"rf", REGISTER_RF},
    {"vm", REGISTER_VM},
    {"ac", REGISTER_AC},
    {"vif", REGISTER_VIF},
    {"vip", REGISTER_VIP},
    {"id", REGISTER_ID},
    {"idtr", REGISTER_IDTR},
    {"gdtr", REGISTER_GDTR},
    {"ldtr", REGISTER_LDTR},
    {"tr", REGISTER_TR},
    {"cr0", REGISTER_CR0},
    {"cr2", REGISTER_CR2},
    {"cr3", REGISTER_CR3},
    {"cr4", REGISTER_CR4},
    {"cr8", REGISTER_CR8},
    {"dr0", REGISTER_DR0},
    {"dr1", REGISTER_DR1},
    {"dr2", REGISTER_DR2},
    {"dr3", REGISTER_DR3},
    {"dr6", REGISTER_DR6},
    {"dr7", REGISTER_DR7},
    {"rip", REGISTER_RIP},
    {"eip", REGISTER_EIP},
    {"ip", REGISTER_IP},
};

/**
 * @brief help of the r command
 *
 * @return VOID
 */
VOID
CommandRHelp()
{
    ShowMessages("r : reads or modifies registers.\n\n");

    ShowMessages("syntax : \tr\n");
    ShowMessages("syntax : \tr [Register (string)] [= Expr (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : r\n");
    ShowMessages("\t\te.g : r @rax\n");
    ShowMessages("\t\te.g : r rax\n");
    ShowMessages("\t\te.g : r rax = 0x55\n");
    ShowMessages("\t\te.g : r rax = @rbx + @rcx + 0n10\n");
}

/**
 * @brief Read all registers
 * @param GuestRegisters The guest registers
 * @param ExtraRegisters The extra registers
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
HyperDbgReadAllRegisters(GUEST_REGS * GuestRegisters, GUEST_EXTRA_REGISTERS * ExtraRegisters)
{
    PGUEST_REGS                          Regs;
    PGUEST_EXTRA_REGISTERS               ExtraRegs;
    DEBUGGEE_REGISTER_READ_DESCRIPTION * RegState       = NULL;
    UINT32                               SizeOfRegState = 0;

    //
    // Calculate the size of the register state
    //
    SizeOfRegState = sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION) + sizeof(GUEST_REGS) + sizeof(GUEST_EXTRA_REGISTERS);

    //
    // Allocate memory for the register state
    //
    RegState = (DEBUGGEE_REGISTER_READ_DESCRIPTION *)malloc(SizeOfRegState);

    //
    // Check if the memory allocation was successful
    //
    if (RegState == NULL)
    {
        return FALSE;
    }

    //
    // Set the register ID to show all registers
    //
    RegState->RegisterID = DEBUGGEE_SHOW_ALL_REGISTERS;

    if (!KdSendReadRegisterPacketToDebuggee(RegState, SizeOfRegState))
    {
        free(RegState);
        return FALSE;
    }

    if (RegState->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        //
        // Copy the registers and extra registers to the output
        //
        Regs      = (GUEST_REGS *)(((CHAR *)RegState) + sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION));
        ExtraRegs = (GUEST_EXTRA_REGISTERS *)(((CHAR *)RegState) + sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION) + sizeof(GUEST_REGS));

        if (GuestRegisters != NULL)
        {
            memcpy(GuestRegisters, Regs, sizeof(GUEST_REGS));
        }

        if (ExtraRegisters != NULL)
        {
            memcpy(ExtraRegisters, ExtraRegs, sizeof(GUEST_EXTRA_REGISTERS));
        }
    }
    else
    {
        ShowErrorMessage(RegState->KernelStatus);
        free(RegState);
        return FALSE;
    }

    free(RegState);
    return TRUE;
}

/**
 * @brief Read target register
 * @param RegisterId The register ID
 * @param TargetRegister The value of the target register
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
HyperDbgReadTargetRegister(UINT32 RegisterId, UINT64 * TargetRegister)
{
    DEBUGGEE_REGISTER_READ_DESCRIPTION RegState = {0};

    //
    // Set the register ID
    //
    RegState.RegisterID = RegisterId;

    if (!KdSendReadRegisterPacketToDebuggee(&RegState, sizeof(DEBUGGEE_REGISTER_READ_DESCRIPTION)))
    {
        return FALSE;
    }

    if (RegState.KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
    {
        if (TargetRegister != NULL)
        {
            *TargetRegister = RegState.Value;
        }
    }
    else
    {
        ShowErrorMessage(RegState.KernelStatus);
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief handler of r show all registers
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperDbgRegisterShowAll()
{
    GUEST_REGS            Regs      = {0};
    GUEST_EXTRA_REGISTERS ExtraRegs = {0};
    RFLAGS                Rflags    = {0};

    if (!HyperDbgReadAllRegisters(&Regs, &ExtraRegs))
    {
        return FALSE;
    }

    //
    // Show the result of reading registers like rax=0000000000018b01
    //
    Rflags.AsUInt = ExtraRegs.RFLAGS;

    ShowMessages(
        "RAX=%016llx RBX=%016llx RCX=%016llx\n"
        "RDX=%016llx RSI=% 016llx RDI=%016llx\n"
        "RIP=%016llx RSP=%016llx RBP=%016llx\n"
        "R8 =%016llx R9 =%016llx R10=%016llx\n"
        "R11=%016llx R12=%016llx R13=%016llx\n"
        "R14=%016llx R15=%016llx IOPL=%02x\n"
        "%s  %s  %s  %s\n%s  %s  %s  %s\n"
        "CS %04x SS %04x DS %04x ES %04x FS %04x GS %04x\n"
        "RFLAGS=%016llx\n",
        Regs.rax,
        Regs.rbx,
        Regs.rcx,
        Regs.rdx,
        Regs.rsi,
        Regs.rdi,
        ExtraRegs.RIP,
        Regs.rsp,
        Regs.rbp,
        Regs.r8,
        Regs.r9,
        Regs.r10,
        Regs.r11,
        Regs.r12,
        Regs.r13,
        Regs.r14,
        Regs.r15,
        Rflags.IoPrivilegeLevel,
        Rflags.OverflowFlag ? "OF 1" : "OF 0",
        Rflags.DirectionFlag ? "DF 1" : "DF 0",
        Rflags.InterruptEnableFlag ? "IF 1" : "IF 0",
        Rflags.SignFlag ? "SF  1" : "SF  0",
        Rflags.ZeroFlag ? "ZF 1" : "ZF 0",
        Rflags.ParityFlag ? "PF 1" : "PF 0",
        Rflags.CarryFlag ? "CF 1" : "CF 0",
        Rflags.AuxiliaryCarryFlag ? "AXF 1" : "AXF 0",
        ExtraRegs.CS,
        ExtraRegs.SS,
        ExtraRegs.DS,
        ExtraRegs.ES,
        ExtraRegs.FS,
        ExtraRegs.GS,
        ExtraRegs.RFLAGS);

    return TRUE;
}

/**
 * @brief handler of r show the target register
 * @param RegisterId The register ID
 *
 * @return BOOLEAN Returns true if it was successful
 */
BOOLEAN
HyperDbgRegisterShowTargetRegister(UINT32 RegisterId)
{
    UINT64 TargetRegister = 0;

    //
    // Read target register
    //
    if (!HyperDbgReadTargetRegister(RegisterId, &TargetRegister))
    {
        return FALSE;
    }

    ShowMessages("%s=%016llx\n",
                 RegistersNames[RegisterId],
                 TargetRegister);

    return TRUE;
}

/**
 * @brief handler of r command
 *
 * @param SplitCommand
 * @param Command
 * @return VOID
 */
VOID
CommandR(std::vector<std::string> SplitCommand, std::string Command)
{
    //
    // Interpret here
    //
    PVOID                    CodeBuffer;
    UINT64                   BufferAddress;
    UINT32                   BufferLength;
    UINT32                   Pointer;
    REGS_ENUM                RegKind;
    std::vector<std::string> Tmp;

    std::string SetRegValue = "SetRegValue";

    if (SplitCommand[0] != "r")
    {
        return;
    }

    if (SplitCommand.size() == 1)
    {
        //
        // show all registers
        //
        if (g_IsSerialConnectedToRemoteDebuggee)
        {
            HyperDbgRegisterShowAll();
        }
        else
        {
            ShowMessages("err, reading registers (r) is not valid in the current "
                         "context, you should connect to a debuggee\n");
        }

        return;
    }
    //
    // clear additional space of the command string
    //

    //
    // if command does not contain a '=' means user wants to read it
    //
    if (Command.find('=', 0) == string::npos)
    {
        //
        // erase '=' from the string now we have just the name of register
        //
        Command.erase(0, 1);
        ReplaceAll(Command, "@", "");
        ReplaceAll(Command, " ", "");
        if (RegistersMap.find(Command) != RegistersMap.end())
        {
            RegKind = RegistersMap[Command];
        }
        else
        {
            //
            // set the Reg to -1(invalid register)
            //
            RegKind = (REGS_ENUM)-1;
        }

        if (RegKind != -1)
        {
            //
            // send the request
            //
            if (g_IsSerialConnectedToRemoteDebuggee)
            {
                HyperDbgRegisterShowTargetRegister((UINT32)RegKind);
            }
            else
            {
                ShowMessages("err, reading registers (r) is not valid in the current "
                             "context, you should connect to a debuggee\n");
            }
        }
        else
        {
            ShowMessages("err, invalid register\n");
        }
    }

    //
    // if command contains a '=' means user wants modify the register
    //

    else if (Command.find('=', 0) != string::npos)
    {
        Command.erase(0, 1);
        Tmp = Split(Command, '=');
        if (Tmp.size() == 2)
        {
            ReplaceAll(Tmp[0], " ", "");
            string tmp = Tmp[0];
            if (RegistersMap.find(Tmp[0]) != RegistersMap.end())
            {
                RegKind = RegistersMap[Tmp[0]];
            }
            else
            {
                ReplaceAll(tmp, "@", "");
                if (RegistersMap.find(tmp) != RegistersMap.end())
                {
                    RegKind = RegistersMap[tmp];
                }
                else
                {
                    RegKind = (REGS_ENUM)-1;
                }
            }
            if (RegKind != -1)
            {
                //
                // send the request
                //

                SetRegValue = "@" + tmp + '=' + Tmp[1] + "; ";
                if (g_IsSerialConnectedToRemoteDebuggee)
                {
                    //
                    // Send over serial
                    //

                    //
                    // Run script engine handler
                    //
                    CodeBuffer = ScriptEngineParseWrapper((char *)SetRegValue.c_str(), TRUE);
                    if (CodeBuffer == NULL)
                    {
                        //
                        // return to show that this item contains an script
                        //
                        return;
                    }

                    //
                    // Print symbols (test)
                    //
                    // PrintSymbolBufferWrapper(CodeBuffer);

                    //
                    // Set the buffer and length
                    //
                    BufferAddress = ScriptEngineWrapperGetHead(CodeBuffer);
                    BufferLength  = ScriptEngineWrapperGetSize(CodeBuffer);
                    Pointer       = ScriptEngineWrapperGetPointer(CodeBuffer);

                    //
                    // Send it to the remote debuggee
                    //
                    KdSendScriptPacketToDebuggee(BufferAddress, BufferLength, Pointer, FALSE);

                    //
                    // Remove the buffer of script engine interpreted code
                    //
                    ScriptEngineWrapperRemoveSymbolBuffer(CodeBuffer);
                }
                else
                {
                    //
                    // error
                    //
                    ShowMessages("err, you're not connected to any debuggee\n");
                }
            }
            else
            {
                //
                // error
                //
                ShowMessages("err, invalid register\n");
            }
        }
    }
}
