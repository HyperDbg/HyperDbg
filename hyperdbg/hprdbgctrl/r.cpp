/**
 * @file r.cpp
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief r command
 * @details
 * @version 0.1
 * @date 2021-02-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

//
// Global Variables
//
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

map<string, REGS_ENUM> RegistersMap = {
    {"rax", REGISTER_RAX},
    {"rbx", REGISTER_RBX},
    {"rcx", REGISTER_RCX},
    {"rdx", REGISTER_RDX},
    {"rsi", REGISTER_RSI},
    {"rdi", REGISTER_RDI},
    {"rbp", REGISTER_RBP},
    {"rsp", REGISTER_RSP},
    {"r8", REGISTER_R8},
    {"r9", REGISTER_R9},
    {"r10", REGISTER_R10},
    {"r11", REGISTER_R11},
    {"r12", REGISTER_R12},
    {"r13", REGISTER_R13},
    {"r14", REGISTER_R14},
    {"r15", REGISTER_R15},
    {"ds", REGISTER_DS},
    {"es", REGISTER_ES},
    {"fs", REGISTER_FS},
    {"gs", REGISTER_GS},
    {"cs", REGISTER_CS},
    {"ss", REGISTER_SS},
    {"rflags", REGISTER_RFLAGS},
    {"rip", REGISTER_RIP},
    {"idtr", REGISTER_IDTR},
    {"gdtr", REGISTER_GDTR},
    {"cr0", REGISTER_CR0},
    {"cr2", REGISTER_CR2},
    {"cr3", REGISTER_CR3},
    {"cr4", REGISTER_CR4},
    {"cr8", REGISTER_CR8},
    {"eax", REGISTER_EAX},
    {"ax", REGISTER_AX},
    {"ah", REGISTER_AH},
    {"al", REGISTER_AL},
    {"ebx", REGISTER_EBX},
    {"bx", REGISTER_BX},
    {"bh", REGISTER_BH},
    {"bl", REGISTER_BL},
    {"ecx", REGISTER_ECX},
    {"cx", REGISTER_CX},
    {"ch", REGISTER_CH},
    {"cl", REGISTER_CL},
    {"edx", REGISTER_EDX},
    {"dx", REGISTER_DX},
    {"dh", REGISTER_DH},
    {"dl", REGISTER_DL},
    {"esp", REGISTER_ESP},
    {"sp", REGISTER_SP},
    {"spl", REGISTER_SPL},
    {"ebp", REGISTER_EBP},
    {"bp", REGISTER_BP},
    {"bpl", REGISTER_BPL},
    {"esi", REGISTER_ESI},
    {"si", REGISTER_SI},
    {"sil", REGISTER_SIL},
    {"edi", REGISTER_EDI},
    {"di", REGISTER_DI},
    {"dil", REGISTER_DIL},
    {"r8d", REGISTER_R8D},
    {"r8w", REGISTER_R8W},
    {"r8h", REGISTER_R8H},
    {"r8l", REGISTER_R8L},
    {"r9d", REGISTER_R9D},
    {"r9w", REGISTER_R9W},
    {"r9h", REGISTER_R9H},
    {"r9l", REGISTER_R9L},
    {"r10d", REGISTER_R10D},
    {"r10w", REGISTER_R10W},
    {"r10h", REGISTER_R10H},
    {"r10l", REGISTER_R10L},
    {"r11d", REGISTER_R11D},
    {"r11w", REGISTER_R11W},
    {"r11h", REGISTER_R11H},
    {"r11l", REGISTER_R11L},
    {"r12d", REGISTER_R12D},
    {"r12w", REGISTER_R12W},
    {"r12h", REGISTER_R12H},
    {"r12l", REGISTER_R12L},
    {"r13d", REGISTER_R13D},
    {"r13w", REGISTER_R13W},
    {"r13h", REGISTER_R13H},
    {"r13l", REGISTER_R13L},
    {"r14d", REGISTER_R14D},
    {"r14w", REGISTER_R14W},
    {"r14h", REGISTER_R14H},
    {"r14l", REGISTER_R14L},
    {"r15d", REGISTER_R15D},
    {"r15w", REGISTER_R15W},
    {"r15h", REGISTER_R15H},
    {"r15l", REGISTER_R15L},
    {"eflags", REGISTER_EFLAGS},
    {"flags", REGISTER_FLAGS},
    {"eip", REGISTER_EIP},
    {"ip", REGISTER_IP}};

/**
 * @brief help of r command
 *
 * @return VOID
 */
VOID
CommandRHelp()
{
    ShowMessages("r : read or modify registers.\n\n");
    ShowMessages("syntax : \tr [register] [= expr]\n");
    ShowMessages("\t\te.g : r @rax\n");
    ShowMessages("\t\te.g : r rax\n");
    ShowMessages("\t\te.g : r rax = 0x55\n");
    ShowMessages("\t\te.g : r rax = @rbx + @rcx + 0n10\n");
}

/**
 * @brief handler of r show all registers command
 *
 * @return BOOLEAN
 */

VOID
ShowAllRegisters()
{
    PDEBUGGEE_REGISTER_READ_DESCRIPTION RegD =
        new DEBUGGEE_REGISTER_READ_DESCRIPTION;
    RegD->RegisterID = DEBUGGEE_SHOW_ALL_REGISTERS;
    KdSendReadRegisterPacketToDebuggee(RegD);
    delete (RegD);
}
/**
 * @brief handler of r command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandR(vector<string> SplittedCommand, string Command)
{
    //
    // Interpret here
    //
    PVOID          CodeBuffer;
    UINT64         BufferAddress;
    UINT32         BufferLength;
    UINT32         Pointer;
    REGS_ENUM      Reg;
    vector<string> Tmp;

    string SetRegValue = "SetRegValue";

    if (SplittedCommand[0] != "r")
    {
        return;
    }

    if (SplittedCommand.size() == 1)
    {
        //
        // show all registers
        //
        if (g_IsSerialConnectedToRemoteDebuggee)
        {
            ShowAllRegisters();
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
        //erase '=' from the string now we have just the name of register
        //
        Command.erase(0, 1);
        PDEBUGGEE_REGISTER_READ_DESCRIPTION RegD =
            new DEBUGGEE_REGISTER_READ_DESCRIPTION;
        ReplaceAll(Command, "@", "");
        ReplaceAll(Command, " ", "");
        if (RegistersMap.find(Command) != RegistersMap.end())
        {
            Reg = RegistersMap[Command];
        }
        else
        {
            //
            //set the Reg to -1(invalid register)
            //
            Reg = (REGS_ENUM)-1;
        }
        if (Reg != -1)
        {
            RegD->RegisterID = Reg;

            //
            // send the request
            //
            if (g_IsSerialConnectedToRemoteDebuggee)
            {
                KdSendReadRegisterPacketToDebuggee(RegD);
            }
            else
            {
                ShowMessages("err, reading registers (r) is not valid in the current "
                             "context, you should connect to a debuggee\n");
            }
        }
        else
        {
            ShowMessages("err, register %s is invalid\n", Command.c_str());
        }
        delete (RegD);
    }

    //
    // if command contains a '=' means user wants modify the register
    //

    else if (Command.find("=", 0))
    {
        PDEBUGGEE_REGISTER_READ_DESCRIPTION RegD =
            new DEBUGGEE_REGISTER_READ_DESCRIPTION;
        Command.erase(0, 1);
        Tmp = Split(Command, '=');
        if (Tmp.size() == 2)
        {
            ReplaceAll(Tmp[0], " ", "");
            string tmp = Tmp[0];
            if (RegistersMap.find(Tmp[0]) != RegistersMap.end())
            {
                Reg = RegistersMap[Tmp[0]];
            }
            else
            {
                ReplaceAll(tmp, "@", "");
                if (RegistersMap.find(tmp) != RegistersMap.end())
                {
                    Reg = RegistersMap[tmp];
                }
                else
                {
                    Reg = (REGS_ENUM)-1;
                }
            }
            if (Reg != -1)
            {
                RegD->RegisterID = Reg;

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
                    CodeBuffer = ScriptEngineParseWrapper((char *)SetRegValue.c_str());
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
        }
    }
}
