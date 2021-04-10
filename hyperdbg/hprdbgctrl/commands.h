/**
 * @file commands.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @brief The hyperdbg command interpreter and driver connector
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

using namespace std;

//////////////////////////////////////////////////
//                    Externs                   //
//////////////////////////////////////////////////

extern HANDLE g_DeviceHandle;

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

int
ReadCpuDetails();

string
ReadVendorString();

VOID
ShowMessages(const char * Fmt, ...);

VOID
HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_MEMORY_STYLE Style,
                                 UINT64                     Address,
                                 DEBUGGER_READ_MEMORY_TYPE  MemoryType,
                                 DEBUGGER_READ_READING_TYPE ReadingType,
                                 UINT32                     Pid,
                                 UINT                       Size);

string
SeparateTo64BitValue(UINT64 Value);

int
HyperDbgDisassembler64(unsigned char * BufferToDisassemble,
                       UINT64          BaseAddress,
                       UINT64          Size,
                       UINT32          MaximumInstrDecoded,
                       BOOLEAN         ShowBranchIsTakenOrNot,
                       PRFLAGS         Rflags);

void
ShowMemoryCommandDB(unsigned char * OutputBuffer, UINT Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length);

void
ShowMemoryCommandDD(unsigned char * OutputBuffer, UINT Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length);

void
ShowMemoryCommandDC(unsigned char * OutputBuffer, UINT Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length);

void
ShowMemoryCommandDQ(unsigned char * OutputBuffer, UINT Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length);

DEBUGGER_CONDITIONAL_JUMP_STATUS
HyperDbgIsConditionalJumpTaken(unsigned char * BufferToDisassemble,
                               UINT64          BuffLength,
                               RFLAGS          Rflags,
                               BOOLEAN         Isx86_64);

int
HyperDbgDisassembler32(unsigned char * BufferToDisassemble,
                       UINT64          BaseAddress,
                       UINT64          Size,
                       UINT32          MaximumInstrDecoded,
                       BOOLEAN         ShowBranchIsTakenOrNot,
                       PRFLAGS         Rflags);

BOOLEAN
HyperDbgCheckWhetherTheCurrentInstructionIsCall(
    unsigned char * BufferToDisassemble,
    UINT64          BuffLength,
    BOOLEAN         Isx86_64,
    PUINT32         CallLength);

VOID
HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_MEMORY_STYLE Style,
                                 UINT64                     Address,
                                 DEBUGGER_READ_MEMORY_TYPE  MemoryType,
                                 DEBUGGER_READ_READING_TYPE ReadingType,
                                 UINT32                     Pid,
                                 UINT                       Size);

VOID
InitializeCommandsDictionary();

//////////////////////////////////////////////////
//              Type of Commands                //
//////////////////////////////////////////////////

/**
 * @brief Command's function type
 *
 */
typedef VOID (*CommandFuncType)(vector<string> SplittedCommand, string Command);

/**
 * @brief Command's help function type
 *
 */
typedef VOID (*CommandHelpFuncType)();

/**
 * @brief Details of each command
 *
 */
typedef struct _COMMAND_DETAIL
{
    CommandFuncType     CommandFunction;
    CommandHelpFuncType CommandHelpFunction;
    UINT64              CommandAttrib;

} COMMAND_DETAIL, *PCOMMAND_DETAIL;

/**
 * @brief Type saving commands and mapping to command string
 *
 */
typedef std::map<std::string, COMMAND_DETAIL> CommandType;

/**
 * @brief Different attributes of commands
 *
 */
#define DEBUGGER_COMMAND_ATTRIBUTE_EVENT \
    0x1 | DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE
#define DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE     0x2
#define DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_REMOTE_CONNECTION 0x4

/**
 * @brief Absolute local commands
 *
 */
#define DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL               \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | \
        DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_REMOTE_CONNECTION

/**
 * @brief Command's attributes
 *
 */
#define DEBUGGER_COMMAND_CLEAR_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_HELP_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_CONNECT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_LISTEN_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_G_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_ATTACH_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_DETACH_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_PROCESS_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_SLEEP_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_EVENTS_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_SETTINGS_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_DISCONNECT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_DEBUG_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_DOT_STATUS_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_STATUS_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_LOAD_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_EXIT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_FLUSH_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PAUSE_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_UNLOAD_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_SCRIPT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_OUTPUT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PRINT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_EVAL_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_LOGOPEN_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_LOGCLOSE_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_TEST_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_CPU_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_WRMSR_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_RDMSR_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_VA2PA_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_PA2VA_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_FORMATS_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PTE_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_CORE_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_MONITOR_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_VMCALL_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_EPTHOOK_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_EPTHOOK2_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_CPUID_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_MSRREAD_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_MSRWRITE_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_TSC_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_PMC_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_DR_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_IOIN_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_IOOUT_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_EXCEPTION_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_INTERRUPT_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_SYSCALL_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_SYSRET_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_HIDE_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_UNHIDE_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_MEASURE_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_LM_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_P_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_T_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_I_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_E_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_S_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_R_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_BP_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_BE_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_BD_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_BC_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_BL_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

//////////////////////////////////////////////////
//             Command Functions                //
//////////////////////////////////////////////////

VOID
CommandTest(vector<string> SplittedCommand, string Command);

VOID
CommandClearScreen(vector<string> SplittedCommand, string Command);

VOID
CommandReadMemoryAndDisassembler(vector<string> SplittedCommand,
                                 string         Command);

VOID
CommandConnect(vector<string> SplittedCommand, string Command);

VOID
CommandConnect(vector<string> SplittedCommand, string Command);

VOID
CommandLoad(vector<string> SplittedCommand, string Command);

VOID
CommandUnload(vector<string> SplittedCommand, string Command);

VOID
CommandScript(vector<string> SplittedCommand, string Command);

VOID
CommandCpu(vector<string> SplittedCommand, string Command);

VOID
CommandExit(vector<string> SplittedCommand, string Command);

VOID
CommandDisconnect(vector<string> SplittedCommand, string Command);

VOID
CommandFormats(vector<string> SplittedCommand, string Command);

VOID
CommandRdmsr(vector<string> SplittedCommand, string Command);

VOID
CommandWrmsr(vector<string> SplittedCommand, string Command);

VOID
CommandPte(vector<string> SplittedCommand, string Command);

VOID
CommandMonitor(vector<string> SplittedCommand, string Command);

VOID
CommandSyscallAndSysret(vector<string> SplittedCommand, string Command);

VOID
CommandEptHook(vector<string> SplittedCommand, string Command);

VOID
CommandEptHook2(vector<string> SplittedCommand, string Command);

VOID
CommandCpuid(vector<string> SplittedCommand, string Command);

VOID
CommandMsrread(vector<string> SplittedCommand, string Command);

VOID
CommandMsrwrite(vector<string> SplittedCommand, string Command);

VOID
CommandTsc(vector<string> SplittedCommand, string Command);

VOID
CommandPmc(vector<string> SplittedCommand, string Command);

VOID
CommandException(vector<string> SplittedCommand, string Command);

VOID
CommandDr(vector<string> SplittedCommand, string Command);

VOID
CommandInterrupt(vector<string> SplittedCommand, string Command);

VOID
CommandIoin(vector<string> SplittedCommand, string Command);

VOID
CommandIoout(vector<string> SplittedCommand, string Command);

VOID
CommandVmcall(vector<string> SplittedCommand, string Command);

VOID
CommandHide(vector<string> SplittedCommand, string Command);

VOID
CommandUnhide(vector<string> SplittedCommand, string Command);

VOID
CommandLogopen(vector<string> SplittedCommand, string Command);

VOID
CommandLogclose(vector<string> SplittedCommand, string Command);

VOID
CommandVa2pa(vector<string> SplittedCommand, string Command);

VOID
CommandPa2va(vector<string> SplittedCommand, string Command);

VOID
CommandEvents(vector<string> SplittedCommand, string Command);

VOID
CommandG(vector<string> SplittedCommand, string Command);

VOID
CommandLm(vector<string> SplittedCommand, string Command);

VOID
CommandSleep(vector<string> SplittedCommand, string Command);

VOID
CommandEditMemory(vector<string> SplittedCommand, string Command);

VOID
CommandSearchMemory(vector<string> SplittedCommand, string Command);

VOID
CommandMeasure(vector<string> SplittedCommand, string Command);

VOID
CommandSettings(vector<string> SplittedCommand, string Command);

VOID
CommandFlush(vector<string> SplittedCommand, string Command);

VOID
CommandPause(vector<string> SplittedCommand, string Command);

VOID
CommandListen(vector<string> SplittedCommand, string Command);

VOID
CommandStatus(vector<string> SplittedCommand, string Command);

VOID
CommandAttach(vector<string> SplittedCommand, string Command);

VOID
CommandDetach(vector<string> SplittedCommand, string Command);

VOID
CommandT(vector<string> SplittedCommand, string Command);

VOID
CommandI(vector<string> SplittedCommand, string Command);

VOID
CommandPrint(vector<string> SplittedCommand, string Command);

VOID
CommandOutput(vector<string> SplittedCommand, string Command);

VOID
CommandDebug(vector<string> SplittedCommand, string Command);

VOID
CommandP(vector<string> SplittedCommand, string Command);

VOID
CommandCore(vector<string> SplittedCommand, string Command);

VOID
CommandProcess(vector<string> SplittedCommand, string Command);

VOID
CommandEval(vector<string> SplittedCommand, string Command);

VOID
CommandR(vector<string> SplittedCommand, string Command);

VOID
CommandBp(vector<string> SplittedCommand, string Command);

VOID
CommandBl(vector<string> SplittedCommand, string Command);

VOID
CommandBe(vector<string> SplittedCommand, string Command);

VOID
CommandBd(vector<string> SplittedCommand, string Command);

VOID
CommandBc(vector<string> SplittedCommand, string Command);
