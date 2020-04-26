/**
 * @file interpreter.cpp
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

extern HANDLE DeviceHandle;

int ReadCpuDetails();
std::string ReadVendorString();
void ShowMessages(const char* Fmt, ...);
int CommandLm(vector<string> SplittedCommand);

void HyperDbgReadMemory(DEBUGGER_SHOW_MEMORY_STYLE Style, UINT64 Address, DEBUGGER_READ_MEMORY_TYPE MemoryType, DEBUGGER_READ_READING_TYPE ReadingType, UINT32 Pid, UINT Size);
int HyperDbgDisassembler(unsigned char* BufferToDisassemble, UINT64 BaseAddress, UINT64 Size);

// Exports
extern "C"
{
    extern bool inline AsmVmxSupportDetection();
    __declspec (dllexport) int __cdecl  HyperdbgLoad();
    __declspec (dllexport) int __cdecl  HyperdbgUnload();
    __declspec (dllexport) int __cdecl  HyperdbgInstallDriver();
    __declspec (dllexport) int __cdecl  HyperdbgUninstallDriver();
    __declspec (dllexport) void __stdcall HyperdbgSetTextMessageCallback(Callback handler);

}



//
// Each command is like the following struct
//
typedef struct _DEBUGGER_COMMANDS_TRACE
{
    UINT64 Tag; // is same as operation code 
    time_t CreationTime;

    LIST_ENTRY CommandsList; // Linked-list of commands list
    DEBUGGER_EVENT_TYPE_ENUM EventType;
    BOOLEAN Enabled;
    UINT32 CoreId; // determines the core index to apply this event to, if it's
                   // 0xffffffff means that we have to apply it to all cores

    LIST_ENTRY ActionsListHead;   // Each entry is in DEBUGGER_EVENT_ACTION struct

    UINT32 CountOfActions;        // The total count of actions

    UINT32 ConditionsBufferSize;  // if null, means uncoditional

    PVOID ConditionBufferAddress; // Address of the condition buffer (most of the
                                  // time at the end of this buffer)
    
} DEBUGGER_COMMANDS_TRACE, * PDEBUGGER_COMMANDS_TRACE;