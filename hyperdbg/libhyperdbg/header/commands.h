/**
 * @file commands.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @author Alee Amini (alee@hyperdbg.org)
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
//                  Settings                    //
//////////////////////////////////////////////////

VOID
CommandSettingsLoadDefaultValuesFromConfigFile();

VOID
CommandSettingsSetValueFromConfigFile(std::string OptionName, std::string OptionValue);

BOOLEAN
CommandSettingsGetValueFromConfigFile(std::string OptionName, std::string & OptionValue);

//////////////////////////////////////////////////
//                  Functions                   //
//////////////////////////////////////////////////

VOID
CpuReadVendorString(CHAR * Result);

int
ReadCpuDetails();

VOID
ShowMessages(const char * Fmt, ...);

string
SeparateTo64BitValue(UINT64 Value);

void
ShowMemoryCommandDB(unsigned char * OutputBuffer, UINT32 Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length);

void
ShowMemoryCommandDD(unsigned char * OutputBuffer, UINT32 Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length);

void
ShowMemoryCommandDC(unsigned char * OutputBuffer, UINT32 Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length);

void
ShowMemoryCommandDQ(unsigned char * OutputBuffer, UINT32 Size, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, UINT64 Length);

VOID
CommandPteShowResults(UINT64 TargetVa, PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PteRead);

DEBUGGER_CONDITIONAL_JUMP_STATUS
HyperDbgIsConditionalJumpTaken(unsigned char * BufferToDisassemble,
                               UINT64          BuffLength,
                               RFLAGS          Rflags,
                               BOOLEAN         Isx86_64);

int
HyperDbgDisassembler64(unsigned char * BufferToDisassemble,
                       UINT64          BaseAddress,
                       UINT64          Size,
                       UINT32          MaximumInstrDecoded,
                       BOOLEAN         ShowBranchIsTakenOrNot,
                       PRFLAGS         Rflags);

int
HyperDbgDisassembler32(unsigned char * BufferToDisassemble,
                       UINT64          BaseAddress,
                       UINT64          Size,
                       UINT32          MaximumInstrDecoded,
                       BOOLEAN         ShowBranchIsTakenOrNot,
                       PRFLAGS         Rflags);

UINT32
HyperDbgLengthDisassemblerEngine(
    unsigned char * BufferToDisassemble,
    UINT64          BuffLength,
    BOOLEAN         Isx86_64);

BOOLEAN
HyperDbgCheckWhetherTheCurrentInstructionIsCall(
    unsigned char * BufferToDisassemble,
    UINT64          BuffLength,
    BOOLEAN         Isx86_64,
    PUINT32         CallLength);

BOOLEAN
HyperDbgCheckWhetherTheCurrentInstructionIsCallOrRet(
    unsigned char * BufferToDisassemble,
    UINT64          CurrentRip,
    UINT32          BuffLength,
    BOOLEAN         Isx86_64,
    PBOOLEAN        IsRet);

BOOLEAN
HyperDbgCheckWhetherTheCurrentInstructionIsRet(
    unsigned char * BufferToDisassemble,
    UINT64          BuffLength,
    BOOLEAN         Isx86_64);

UINT32
HyperDbgGetImmediateValueOnEaxForSyscallNumber(
    unsigned char * BufferToDisassemble,
    UINT64          BuffLength,
    BOOLEAN         Isx86_64);

VOID
HyperDbgShowMemoryOrDisassemble(DEBUGGER_SHOW_MEMORY_STYLE   Style,
                                UINT64                       Address,
                                DEBUGGER_READ_MEMORY_TYPE    MemoryType,
                                DEBUGGER_READ_READING_TYPE   ReadingType,
                                UINT32                       Pid,
                                UINT32                       Size,
                                PDEBUGGER_DT_COMMAND_OPTIONS DtDetails);

BOOLEAN
HyperDbgReadMemory(UINT64                              TargetAddress,
                   DEBUGGER_READ_MEMORY_TYPE           MemoryType,
                   DEBUGGER_READ_READING_TYPE          ReadingType,
                   UINT32                              Pid,
                   UINT32                              Size,
                   BOOLEAN                             GetAddressMode,
                   DEBUGGER_READ_MEMORY_ADDRESS_MODE * AddressMode,
                   BYTE *                              TargetBufferToStore,
                   UINT32 *                            ReturnLength);

VOID
InitializeCommandsDictionary();

VOID
InitializeDebugger();

BOOLEAN
CheckMultilineCommand(CHAR * CurrentCommand, BOOLEAN Reset);

BOOLEAN
ContinuePreviousCommand();

VOID
CommandDumpSaveIntoFile(PVOID Buffer, UINT32 Length);

//////////////////////////////////////////////////
//              Type of Commands                //
//////////////////////////////////////////////////

/**
 * @brief Command's parsing type (enum)
 *
 */
typedef enum
{
    Num,
    String,
    StringLiteral,
    BracketString
} CommandParsingTokenType;

/**
 * @brief Command's parsing type
 *
 */
typedef std::tuple<CommandParsingTokenType, std::string, std::string> CommandToken;

/**
 * @brief Command's function type
 *
 */
typedef VOID (*CommandFuncTypeParser)(vector<CommandToken> CommandTokens, string Command);

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
    CommandFuncTypeParser CommandFunctionNewParser;
    CommandHelpFuncType   CommandHelpFunction;
    UINT64                CommandAttrib;

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
#define DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE     0x1
#define DEBUGGER_COMMAND_ATTRIBUTE_EVENT                              0x2 | DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE
#define DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_REMOTE_CONNECTION 0x4
#define DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER                    0x8
#define DEBUGGER_COMMAND_ATTRIBUTE_WONT_STOP_DEBUGGER_AGAIN           0x10
#define DEBUGGER_COMMAND_ATTRIBUTE_HWDBG                              0x20

/**
 * @brief Absolute local commands
 *
 */
#define DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_REMOTE_CONNECTION

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

#define DEBUGGER_COMMAND_G_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_GG_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_ATTACH_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_WONT_STOP_DEBUGGER_AGAIN

#define DEBUGGER_COMMAND_DETACH_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_WONT_STOP_DEBUGGER_AGAIN

#define DEBUGGER_COMMAND_SWITCH_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_CONTINUE_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_WONT_STOP_DEBUGGER_AGAIN

#define DEBUGGER_COMMAND_PAUSE_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_WONT_STOP_DEBUGGER_AGAIN

#define DEBUGGER_COMMAND_START_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_WONT_STOP_DEBUGGER_AGAIN

#define DEBUGGER_COMMAND_RESTART_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_WONT_STOP_DEBUGGER_AGAIN

#define DEBUGGER_COMMAND_KILL_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_PROCESS_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_THREAD_ATTRIBUTES \
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

#define DEBUGGER_COMMAND_UNLOAD_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_SCRIPT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_OUTPUT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PRINT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_EVAL_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_LOGOPEN_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_LOGCLOSE_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_ABSOLUTE_LOCAL

#define DEBUGGER_COMMAND_TEST_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_CPU_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_WRMSR_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_RDMSR_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_VA2PA_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PA2VA_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_FORMATS_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_PTE_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_APIC_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_IOAPIC_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

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

#define DEBUGGER_COMMAND_CRWRITE_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_DR_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_IOIN_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_IOOUT_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_EXCEPTION_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_INTERRUPT_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_SYSCALL_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_SYSRET_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_MODE_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_TRACE_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_EVENT

#define DEBUGGER_COMMAND_HIDE_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_UNHIDE_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_MEASURE_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_LM_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_P_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_T_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_I_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_D_AND_U_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_E_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_S_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_R_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

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

#define DEBUGGER_COMMAND_SYMPATH_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_SYM_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_X_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PREALLOC_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_PREACTIVATE_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_K_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_DT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_STRUCT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PE_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_REV_ATTRIBUTES NULL

#define DEBUGGER_COMMAND_TRACK_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PAGEIN_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_DUMP_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_GU_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE | DEBUGGER_COMMAND_ATTRIBUTE_REPEAT_ON_ENTER

#define DEBUGGER_COMMAND_HWDBG_HW_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_HWDBG

#define DEBUGGER_COMMAND_HWDBG_HW_CLK_ATTRIBUTES DEBUGGER_COMMAND_ATTRIBUTE_HWDBG

#define DEBUGGER_COMMAND_A_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PCITREE_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_PCICAM_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

#define DEBUGGER_COMMAND_IDT_ATTRIBUTES \
    DEBUGGER_COMMAND_ATTRIBUTE_LOCAL_COMMAND_IN_DEBUGGER_MODE

//////////////////////////////////////////////////
//             Command Functions                //
//////////////////////////////////////////////////

VOID
CommandTest(vector<CommandToken> CommandTokens, string Command);

VOID
CommandCls(vector<CommandToken> CommandTokens, string Command);

VOID
CommandReadMemoryAndDisassembler(vector<CommandToken> CommandTokens, string Command);

VOID
CommandConnect(vector<CommandToken> CommandTokens, string Command);

VOID
CommandLoad(vector<CommandToken> CommandTokens, string Command);

VOID
CommandUnload(vector<CommandToken> CommandTokens, string Command);

VOID
CommandScript(vector<CommandToken> CommandTokens, string Command);

VOID
CommandCpu(vector<CommandToken> CommandTokens, string Command);

VOID
CommandExit(vector<CommandToken> CommandTokens, string Command);

VOID
CommandDisconnect(vector<CommandToken> CommandTokens, string Command);

VOID
CommandFormats(vector<CommandToken> CommandTokens, string Command);

VOID
CommandRdmsr(vector<CommandToken> CommandTokens, string Command);

VOID
CommandWrmsr(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPte(vector<CommandToken> CommandTokens, string Command);

VOID
CommandMonitor(vector<CommandToken> CommandTokens, string Command);

VOID
CommandSyscallAndSysret(vector<CommandToken> CommandTokens, string Command);

VOID
CommandEptHook(vector<CommandToken> CommandTokens, string Command);

VOID
CommandEptHook2(vector<CommandToken> CommandTokens, string Command);

VOID
CommandCpuid(vector<CommandToken> CommandTokens, string Command);

VOID
CommandMsrread(vector<CommandToken> CommandTokens, string Command);

VOID
CommandMsrwrite(vector<CommandToken> CommandTokens, string Command);

VOID
CommandTsc(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPmc(vector<CommandToken> CommandTokens, string Command);

VOID
CommandException(vector<CommandToken> CommandTokens, string Command);

VOID
CommandCrwrite(vector<CommandToken> CommandTokens, string Command);

VOID
CommandDr(vector<CommandToken> CommandTokens, string Command);

VOID
CommandInterrupt(vector<CommandToken> CommandTokens, string Command);

VOID
CommandIoin(vector<CommandToken> CommandTokens, string Command);

VOID
CommandIoout(vector<CommandToken> CommandTokens, string Command);

VOID
CommandVmcall(vector<CommandToken> CommandTokens, string Command);

VOID
CommandMode(vector<CommandToken> CommandTokens, string Command);

VOID
CommandTrace(vector<CommandToken> CommandTokens, string Command);

VOID
CommandHide(vector<CommandToken> CommandTokens, string Command);

VOID
CommandUnhide(vector<CommandToken> CommandTokens, string Command);

VOID
CommandLogopen(vector<CommandToken> CommandTokens, string Command);

VOID
CommandLogclose(vector<CommandToken> CommandTokens, string Command);

VOID
CommandVa2pa(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPa2va(vector<CommandToken> CommandTokens, string Command);

VOID
CommandEvents(vector<CommandToken> CommandTokens, string Command);

VOID
CommandG(vector<CommandToken> CommandTokens, string Command);

VOID
CommandContinue(vector<CommandToken> CommandTokens, string Command);

VOID
CommandGg(vector<CommandToken> CommandTokens, string Command);

VOID
CommandLm(vector<CommandToken> CommandTokens, string Command);

VOID
CommandSleep(vector<CommandToken> CommandTokens, string Command);

VOID
CommandEditMemory(vector<CommandToken> CommandTokens, string Command);

VOID
CommandSearchMemory(vector<CommandToken> CommandTokens, string Command);

VOID
CommandMeasure(vector<CommandToken> CommandTokens, string Command);

VOID
CommandSettings(vector<CommandToken> CommandTokens, string Command);

VOID
CommandFlush(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPause(vector<CommandToken> CommandTokens, string Command);

VOID
CommandListen(vector<CommandToken> CommandTokens, string Command);

VOID
CommandStatus(vector<CommandToken> CommandTokens, string Command);

VOID
CommandAttach(vector<CommandToken> CommandTokens, string Command);

VOID
CommandDetach(vector<CommandToken> CommandTokens, string Command);

VOID
CommandStart(vector<CommandToken> CommandTokens, string Command);

VOID
CommandRestart(vector<CommandToken> CommandTokens, string Command);

VOID
CommandSwitch(vector<CommandToken> CommandTokens, string Command);

VOID
CommandKill(vector<CommandToken> CommandTokens, string Command);

VOID
CommandT(vector<CommandToken> CommandTokens, string Command);

VOID
CommandI(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPrint(vector<CommandToken> CommandTokens, string Command);

VOID
CommandOutput(vector<CommandToken> CommandTokens, string Command);

VOID
CommandDebug(vector<CommandToken> CommandTokens, string Command);

VOID
CommandP(vector<CommandToken> CommandTokens, string Command);

VOID
CommandCore(vector<CommandToken> CommandTokens, string Command);

VOID
CommandProcess(vector<CommandToken> CommandTokens, string Command);

VOID
CommandThread(vector<CommandToken> CommandTokens, string Command);

VOID
CommandEval(vector<CommandToken> CommandTokens, string Command);

VOID
CommandR(vector<CommandToken> CommandTokens, string Command);

VOID
CommandBp(vector<CommandToken> CommandTokens, string Command);

VOID
CommandBl(vector<CommandToken> CommandTokens, string Command);

VOID
CommandBe(vector<CommandToken> CommandTokens, string Command);

VOID
CommandBd(vector<CommandToken> CommandTokens, string Command);

VOID
CommandBc(vector<CommandToken> CommandTokens, string Command);

VOID
CommandSympath(vector<CommandToken> CommandTokens, string Command);

VOID
CommandSym(vector<CommandToken> CommandTokens, string Command);

VOID
CommandX(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPrealloc(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPreactivate(vector<CommandToken> CommandTokens, string Command);

VOID
CommandDtAndStruct(vector<CommandToken> CommandTokens, string Command);

VOID
CommandK(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPe(vector<CommandToken> CommandTokens, string Command);

VOID
CommandRev(vector<CommandToken> CommandTokens, string Command);

VOID
CommandApic(vector<CommandToken> CommandTokens, string Command);

VOID
CommandIoapic(vector<CommandToken> CommandTokens, string Command);

VOID
CommandTrack(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPagein(vector<CommandToken> CommandTokens, string Command);

VOID
CommandDump(vector<CommandToken> CommandTokens, string Command);

VOID
CommandGu(vector<CommandToken> CommandTokens, string Command);

VOID
CommandAssemble(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPcitree(vector<CommandToken> CommandTokens, string Command);

VOID
CommandPcicam(vector<CommandToken> CommandTokens, string Command);

VOID
CommandIdt(vector<CommandToken> CommandTokens, string Command);

//
// hwdbg commands
//
VOID
CommandHwClk(vector<CommandToken> CommandTokens, string Command);

VOID
CommandHw(vector<CommandToken> CommandTokens, string Command);
