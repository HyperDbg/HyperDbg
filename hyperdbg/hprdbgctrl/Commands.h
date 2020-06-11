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
#include "pch.h"

using namespace std;

//////////////////////////////////////////////////
//					Externs
////
//////////////////////////////////////////////////

extern HANDLE g_DeviceHandle;

//////////////////////////////////////////////////
//					Functions
////
//////////////////////////////////////////////////

int ReadCpuDetails();

string ReadVendorString();

VOID ShowMessages(const char *Fmt, ...);

int CommandLm(vector<string> SplittedCommand);

VOID HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_MEMORY_STYLE Style,
                                      UINT64 Address,
                                      DEBUGGER_READ_MEMORY_TYPE MemoryType,
                                      DEBUGGER_READ_READING_TYPE ReadingType,
                                      UINT32 Pid, UINT Size);
string SeparateTo64BitValue(UINT64 Value);

int HyperDbgDisassembler(unsigned char *BufferToDisassemble, UINT64 BaseAddress,
                         UINT64 Size);

VOID HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_MEMORY_STYLE Style,
                                      UINT64 Address,
                                      DEBUGGER_READ_MEMORY_TYPE MemoryType,
                                      DEBUGGER_READ_READING_TYPE ReadingType,
                                      UINT32 Pid, UINT Size);
VOID CommandClearScreen();

VOID CommandReadMemoryAndDisassembler(vector<string> SplittedCommand);

VOID CommandConnect(vector<string> SplittedCommand);

VOID CommandConnect(vector<string> SplittedCommand);

VOID CommandLoad(vector<string> SplittedCommand);

VOID CommandUnload(vector<string> SplittedCommand);

VOID CommandCpu(vector<string> SplittedCommand);

VOID CommandExit(vector<string> SplittedCommand);

VOID CommandDisconnect(vector<string> SplittedCommand);

VOID CommandFormats(vector<string> SplittedCommand);

VOID CommandRdmsr(vector<string> SplittedCommand);

VOID CommandWrmsr(vector<string> SplittedCommand);

VOID CommandPte(vector<string> SplittedCommand);

VOID CommandMonitor(vector<string> SplittedCommand);

VOID CommandSyscallAndSysret(vector<string> SplittedCommand);

VOID CommandHiddenHook(vector<string> SplittedCommand);

VOID CommandCpuid(vector<string> SplittedCommand);

VOID CommandMsrread(vector<string> SplittedCommand);

VOID CommandMsrwrite(vector<string> SplittedCommand);

VOID CommandTsc(vector<string> SplittedCommand);

VOID CommandPmc(vector<string> SplittedCommand);

VOID CommandException(vector<string> SplittedCommand);

VOID CommandDr(vector<string> SplittedCommand);

VOID CommandInterrupt(vector<string> SplittedCommand);

VOID CommandIoin(vector<string> SplittedCommand);

VOID CommandIoout(vector<string> SplittedCommand);
