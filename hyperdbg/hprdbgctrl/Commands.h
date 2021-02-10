/**
 * @file commands.h
 * @author Sina Karvandi (sina@rayanfam.com)
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

int ReadCpuDetails();

string ReadVendorString();

VOID ShowMessages(const char *Fmt, ...);

VOID HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_MEMORY_STYLE Style,
                                      UINT64 Address,
                                      DEBUGGER_READ_MEMORY_TYPE MemoryType,
                                      DEBUGGER_READ_READING_TYPE ReadingType,
                                      UINT32 Pid, UINT Size);
string SeparateTo64BitValue(UINT64 Value);

int HyperDbgDisassembler64(unsigned char *BufferToDisassemble,
                           UINT64 BaseAddress, UINT64 Size,
                           UINT32 MaximumInstrDecoded);

int HyperDbgDisassembler32(unsigned char *BufferToDisassemble,
                           UINT64 BaseAddress, UINT64 Size,
                           UINT32 MaximumInstrDecoded);

VOID HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_MEMORY_STYLE Style,
                                      UINT64 Address,
                                      DEBUGGER_READ_MEMORY_TYPE MemoryType,
                                      DEBUGGER_READ_READING_TYPE ReadingType,
                                      UINT32 Pid, UINT Size);

VOID InitializeCommandsDictionary();

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
typedef struct _COMMAND_DETAIL {

  CommandFuncType CommandFunction;
  CommandHelpFuncType CommandHelpFunction;
  UINT64 CommandAttrib;

} COMMAND_DETAIL, *PCOMMAND_DETAIL;

/**
 * @brief Type saving commands and mapping to command string
 *
 */
typedef std::map<std::string, COMMAND_DETAIL> CommandType;

//////////////////////////////////////////////////
//             Command Functions                //
//////////////////////////////////////////////////

VOID CommandTest(vector<string> SplittedCommand, string Command);

VOID CommandClearScreen(vector<string> SplittedCommand, string Command);

VOID CommandReadMemoryAndDisassembler(vector<string> SplittedCommand,
                                      string Command);

VOID CommandConnect(vector<string> SplittedCommand, string Command);

VOID CommandConnect(vector<string> SplittedCommand, string Command);

VOID CommandLoad(vector<string> SplittedCommand, string Command);

VOID CommandUnload(vector<string> SplittedCommand, string Command);

VOID CommandScript(vector<string> SplittedCommand, string Command);

VOID CommandCpu(vector<string> SplittedCommand, string Command);

VOID CommandExit(vector<string> SplittedCommand, string Command);

VOID CommandDisconnect(vector<string> SplittedCommand, string Command);

VOID CommandFormats(vector<string> SplittedCommand, string Command);

VOID CommandRdmsr(vector<string> SplittedCommand, string Command);

VOID CommandWrmsr(vector<string> SplittedCommand, string Command);

VOID CommandPte(vector<string> SplittedCommand, string Command);

VOID CommandMonitor(vector<string> SplittedCommand, string Command);

VOID CommandSyscallAndSysret(vector<string> SplittedCommand, string Command);

VOID CommandEptHook(vector<string> SplittedCommand, string Command);

VOID CommandEptHook2(vector<string> SplittedCommand, string Command);

VOID CommandCpuid(vector<string> SplittedCommand, string Command);

VOID CommandMsrread(vector<string> SplittedCommand, string Command);

VOID CommandMsrwrite(vector<string> SplittedCommand, string Command);

VOID CommandTsc(vector<string> SplittedCommand, string Command);

VOID CommandPmc(vector<string> SplittedCommand, string Command);

VOID CommandException(vector<string> SplittedCommand, string Command);

VOID CommandDr(vector<string> SplittedCommand, string Command);

VOID CommandInterrupt(vector<string> SplittedCommand, string Command);

VOID CommandIoin(vector<string> SplittedCommand, string Command);

VOID CommandIoout(vector<string> SplittedCommand, string Command);

VOID CommandVmcall(vector<string> SplittedCommand, string Command);

VOID CommandHide(vector<string> SplittedCommand, string Command);

VOID CommandUnhide(vector<string> SplittedCommand, string Command);

VOID CommandLogopen(vector<string> SplittedCommand, string Command);

VOID CommandLogclose(vector<string> SplittedCommand, string Command);

VOID CommandVa2pa(vector<string> SplittedCommand, string Command);

VOID CommandPa2va(vector<string> SplittedCommand, string Command);

VOID CommandEvents(vector<string> SplittedCommand, string Command);

VOID CommandG(vector<string> SplittedCommand, string Command);

VOID CommandLm(vector<string> SplittedCommand, string Command);

VOID CommandSleep(vector<string> SplittedCommand, string Command);

VOID CommandEditMemory(vector<string> SplittedCommand, string Command);

VOID CommandSearchMemory(vector<string> SplittedCommand, string Command);

VOID CommandMeasure(vector<string> SplittedCommand, string Command);

VOID CommandSettings(vector<string> SplittedCommand, string Command);

VOID CommandFlush(vector<string> SplittedCommand, string Command);

VOID CommandPause(vector<string> SplittedCommand, string Command);

VOID CommandListen(vector<string> SplittedCommand, string Command);

VOID CommandStatus(vector<string> SplittedCommand, string Command);

VOID CommandAttach(vector<string> SplittedCommand, string Command);

VOID CommandDetach(vector<string> SplittedCommand, string Command);

VOID CommandT(vector<string> SplittedCommand, string Command);

VOID CommandPrint(vector<string> SplittedCommand, string Command);

VOID CommandOutput(vector<string> SplittedCommand, string Command);

VOID CommandDebug(vector<string> SplittedCommand, string Command);

VOID CommandP(vector<string> SplittedCommand, string Command);

VOID CommandCore(vector<string> SplittedCommand, string Command);

VOID CommandProcess(vector<string> SplittedCommand, string Command);

VOID CommandEval(vector<string> SplittedCommand, string Command);
