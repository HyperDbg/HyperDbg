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

#include "pch.h"
#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <string>
#include <time.h>
#include <vector>

using namespace std;

// Exports
extern "C" {
__declspec(dllexport) int __cdecl HyperdbgInterpreter(const char *Command);
}

bool g_IsConnectedToDebugger = false;
bool g_IsDebuggerModulesLoaded = false;

string SeparateTo64BitValue(UINT64 Value) {
  std::ostringstream ostringStream;
  ostringStream << std::setw(16) << std::setfill('0') << std::hex << Value;
  string temp = ostringStream.str();

  temp.insert(8, 1, '`');
  return temp;
}

/**
 * @brief Read memory and disassembler
 *
 */
void HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_MEMORY_STYLE Style,
                                      UINT64 Address,
                                      DEBUGGER_READ_MEMORY_TYPE MemoryType,
                                      DEBUGGER_READ_READING_TYPE ReadingType,
                                      UINT32 Pid, UINT Size) {

  BOOL Status;
  ULONG ReturnedLength;
  DEBUGGER_READ_MEMORY ReadMem;
  UINT32 OperationCode;
  CHAR Character;

  if (!DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return;
  }

  ReadMem.Address = Address;
  ReadMem.Pid = Pid;
  ReadMem.Size = Size;
  ReadMem.MemoryType = MemoryType;
  ReadMem.ReadingType = ReadingType;

  //
  // allocate buffer for transfering messages
  //
  unsigned char *OutputBuffer = (unsigned char *)malloc(Size);

  ZeroMemory(OutputBuffer, Size);

  Status = DeviceIoControl(DeviceHandle,               // Handle to device
                           IOCTL_DEBUGGER_READ_MEMORY, // IO Control code
                           &ReadMem, // Input Buffer to driver.
                           SIZEOF_DEBUGGER_READ_MEMORY, // Input buffer length
                           OutputBuffer,    // Output Buffer from driver.
                           Size,            // Length of output buffer in bytes.
                           &ReturnedLength, // Bytes placed in buffer.
                           NULL             // synchronous call
  );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return;
  }

  if (Style == DEBUGGER_SHOW_COMMAND_DB) {
    for (int i = 0; i < Size; i += 16) {

      if (MemoryType == DEBUGGER_READ_PHYSICAL_ADDRESS) {
        ShowMessages("#\t");
      }
      //
      // Print address
      //
      ShowMessages("%s  ", SeparateTo64BitValue((UINT64)(Address + i)).c_str());

      //
      // Print the hex code
      //
      for (size_t j = 0; j < 16; j++) {
        //
        // check to see if the address is valid or not
        //
        if (i + j >= ReturnedLength) {
          ShowMessages("?? ");
        } else {
          ShowMessages("%02X ", OutputBuffer[i + j]);
        }
      }
      //
      // Print the character
      //
      ShowMessages(" ");
      for (size_t j = 0; j < 16; j++) {
        Character = (OutputBuffer[i + j]);
        if (isprint(Character)) {
          ShowMessages("%c", Character);
        } else {
          ShowMessages(".");
        }
      }

      //
      // Go to new line
      //
      ShowMessages("\n");
    }
  } else if (Style == DEBUGGER_SHOW_COMMAND_DC ||
             Style == DEBUGGER_SHOW_COMMAND_DD) {
    for (int i = 0; i < Size; i += 16) {

      if (MemoryType == DEBUGGER_READ_PHYSICAL_ADDRESS) {
        ShowMessages("#\t");
      }
      //
      // Print address
      //
      ShowMessages("%s  ", SeparateTo64BitValue((UINT64)(Address + i)).c_str());

      //
      // Print the hex code
      //
      for (size_t j = 0; j < 16; j += 4) {
        //
        // check to see if the address is valid or not
        //
        if (i + j >= ReturnedLength) {
          ShowMessages("???????? ");
        } else {
          UINT32 OutputBufferVar = *((UINT32 *)&OutputBuffer[i + j]);
          ShowMessages("%08X ", OutputBufferVar);
        }
      }
      //
      // Print the character
      //
      if (Style != DEBUGGER_SHOW_COMMAND_DD) {
        ShowMessages(" ");

        for (size_t j = 0; j < 16; j++) {
          Character = (OutputBuffer[i + j]);
          if (isprint(Character)) {
            ShowMessages("%c", Character);
          } else {
            ShowMessages(".");
          }
        }
      }

      //
      // Go to new line
      //
      ShowMessages("\n");
    }
  } else if (Style == DEBUGGER_SHOW_COMMAND_DQ) {
    for (int i = 0; i < Size; i += 16) {

      if (MemoryType == DEBUGGER_READ_PHYSICAL_ADDRESS) {
        ShowMessages("#\t");
      }
      //
      // Print address
      //
      ShowMessages("%s  ", SeparateTo64BitValue((UINT64)(Address + i)).c_str());

      //
      // Print the hex code
      //
      for (size_t j = 0; j < 16; j += 8) {
        //
        // check to see if the address is valid or not
        //
        if (i + j >= ReturnedLength) {
          ShowMessages("???????? ");
        } else {
          UINT32 OutputBufferVar = *((UINT32 *)&OutputBuffer[i + j + 4]);
          ShowMessages("%08X`", OutputBufferVar);

          OutputBufferVar = *((UINT32 *)&OutputBuffer[i + j]);
          ShowMessages("%08X ", OutputBufferVar);
        }
      }

      //
      // Go to new line
      //
      ShowMessages("\n");
    }
  } else if (Style == DEBUGGER_SHOW_COMMAND_DISASSEMBLE) {
    HyperDbgDisassembler(OutputBuffer, Address, ReturnedLength);
  }

  ShowMessages("\n");
}

const vector<string> Split(const string &s, const char &c) {
  string buff{""};
  vector<string> v;

  for (auto n : s) {
    if (n != c)
      buff += n;
    else if (n == c && buff != "") {
      v.push_back(buff);
      buff = "";
    }
  }
  if (buff != "")
    v.push_back(buff);

  return v;
}
// check if given string is a numeric string or not
bool IsNumber(const string &str) {
  // std::find_first_not_of searches the string for the first character
  // that does not match any of the characters specified in its arguments
  return !str.empty() &&
         (str.find_first_not_of("[0123456789]") == std::string::npos);
}

// Function to split string str using given delimiter
vector<string> SplitIp(const string &str, char delim) {
  auto i = 0;
  vector<string> list;

  auto pos = str.find(delim);

  while (pos != string::npos) {
    list.push_back(str.substr(i, pos - i));
    i = ++pos;
    pos = str.find(delim, pos);
  }

  list.push_back(str.substr(i, str.length()));

  return list;
}

bool IsHexNotation(std::string const &s) {
  return s.compare(0, 2, "0x") == 0 && s.size() > 2 &&
         s.find_first_not_of("0123456789abcdefABCDEF", 2) == std::string::npos;
}

BOOLEAN ConvertStringToUInt64(const char *Text, PUINT64 Result) {

  if (Text[strspn(Text, "0123456789abcdefABCDEF")] == 0) {
    errno = 0;
    unsigned long long result = strtoull(Text, NULL, 16);

    *Result = result;

    if (errno == EINVAL) {
      return FALSE;
    } else if (errno == ERANGE) {
      return TRUE;
    }
  } else {
    return FALSE;
  }
}

// Function to validate an IP address
bool ValidateIP(string ip) {
  // split the string into tokens
  vector<string> list = SplitIp(ip, '.');

  // if token size is not equal to four
  if (list.size() != 4)
    return false;

  // validate each token
  for (string str : list) {
    // verify that string is number or not and the numbers
    // are in the valid range
    if (!IsNumber(str) || stoi(str) > 255 || stoi(str) < 0)
      return false;
  }

  return true;
}

void CommandClearScreen() { system("cls"); }

void CommandHiddenHook(vector<string> SplittedCommand) {

  //
  // Note : You can use "bh" and "!hiddenhook" in a same way
  // * means optional
  // !hiddenhook
  //				 [Type:(all/process)]
  //				 [Address:(hex) - Address to hook]
  //				*[Pid:(hex number) - only if you choose
  //'process' as the Type] [Action:(break/code/log)]
  //				*[Condition:({ asm in hex })]
  //				*[Code:({asm in hex}) - only if you choose
  //'code' as the Action]
  //				*[Log:(gp regs, pseudo-regs, static address,
  // dereference regs +- value) - only if you choose 'log' as the Action]
  //
  //
  //		e.g :
  //				-	bh all break fffff801deadbeef
  //						Description : Breaks to the
  // debugger in all accesses to the selected address
  //
  //				-	!hiddenhook process 8e34
  // fffff801deadbeef 						Description :
  // Breaks to the debugger in accesses from the selected process by pid to the
  // selected address
  //

  for (auto Section : SplittedCommand) {
    ShowMessages("%s", Section.c_str());
    ShowMessages("\n");
  }
}

void CommandReadMemoryHelp() {
  ShowMessages("u !u & db dc dd dq !db !dc !dd !dq : read the memory different "
               "shapes (hex) and disassembler\n");
  ShowMessages("d[b]  Byte and ASCII characters\n");
  ShowMessages("d[c]  Double-word values (4 bytes) and ASCII characters\n");
  ShowMessages("d[d]  Double-word values (4 bytes)\n");
  ShowMessages("d[q]  Quad-word values (8 bytes). \n");
  ShowMessages("u  Disassembler at the target address \n");
  ShowMessages("\n If you want to read physical memory then add '!' at the "
               "start of the command\n");
  ShowMessages("You can also disassemble physical memory using '!u'\n");

  ShowMessages("syntax : \t[!]d[b|c|d|q] [address] l [length (hex)] pid "
               "[process id (hex)]"
               "[process id (hex)]\n");
  ShowMessages("\t\te.g : db fffff8077356f010 \n");
  ShowMessages("\t\te.g : !dq 100000\n");
  ShowMessages("\t\te.g : u fffff8077356f010\n");
}
void CommandReadMemoryAndDisassembler(vector<string> SplittedCommand) {

  string FirstCommand = SplittedCommand.front();

  UINT32 Pid = 0;
  UINT32 Length = 0;
  UINT64 TargetAddress = 0;
  bool IsNextProcessId = false;
  bool IsFirstCommand = true;

  bool IsNextLength = false;

  if (SplittedCommand.size() == 1) {
    //
    // Means that user entered just a connect so we have to
    // ask to connect to what ?
    //
    ShowMessages("incorrect use of '%s' command\n\n", FirstCommand.c_str());
    CommandReadMemoryHelp();
    return;
  }

  for (auto Section : SplittedCommand) {
    if (IsFirstCommand) {
      IsFirstCommand = false;
      continue;
    }
    if (IsNextProcessId == true) {
      if (IsHexNotation(Section)) {
        ShowMessages("Err, you should enter a valid proc id\n\n");
        return;
      }
      Pid = stoi(Section, nullptr, 16);
      IsNextProcessId = false;
      continue;
    }

    if (IsNextLength == true) {

      if (IsHexNotation(Section)) {
        ShowMessages("Err, you should enter a valid length\n\n");
        return;
      }
      Length = stoi(Section, nullptr, 16);
      IsNextLength = false;
      continue;
    }

    if (!Section.compare("l")) {
      IsNextLength = true;
      continue;
    }

    if (!Section.compare("pid")) {
      IsNextProcessId = true;
      continue;
    }

    //
    // Probably it's address
    //
    if (TargetAddress == 0) {

      string TempAddress = Section;
      TempAddress.erase(remove(TempAddress.begin(), TempAddress.end(), '`'),
                        TempAddress.end());

      if (!ConvertStringToUInt64(TempAddress.c_str(), &TargetAddress)) {
        ShowMessages("Err, you should enter a valid address\n\n");
        return;
      }
    } else {
      //
      // User inserts two address
      //
      ShowMessages("Err, incorrect use of '%s' command\n\n",
                   FirstCommand.c_str());
      CommandReadMemoryHelp();

      return;
    }
  }
  if (!TargetAddress) {
    //
    // User inserts two address
    //
    ShowMessages("Err, Please enter a valid address.\n\n");

    return;
  }
  if (Length == 0) {
    //
    // Default length (user doesn't specified)
    //
    if (!FirstCommand.compare("u") || !FirstCommand.compare("!u")) {
      Length = 0x40;
    } else {
      Length = 0x80;
    }
  }

  if (Pid == 0) {
    //
    // Default process we read from current process
    //
    Pid = GetCurrentProcessId();
  }

  if (!FirstCommand.compare("db")) {
    HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DB, TargetAddress,
                                     DEBUGGER_READ_VIRTUAL_ADDRESS,
                                     READ_FROM_KERNEL, Pid, Length);

  } else if (!FirstCommand.compare("dc")) {
    HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DC, TargetAddress,
                                     DEBUGGER_READ_VIRTUAL_ADDRESS,
                                     READ_FROM_KERNEL, Pid, Length);
  } else if (!FirstCommand.compare("dd")) {
    HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DD, TargetAddress,
                                     DEBUGGER_READ_VIRTUAL_ADDRESS,
                                     READ_FROM_KERNEL, Pid, Length);
  } else if (!FirstCommand.compare("dq")) {
    HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DQ, TargetAddress,
                                     DEBUGGER_READ_VIRTUAL_ADDRESS,
                                     READ_FROM_KERNEL, Pid, Length);
  } else if (!FirstCommand.compare("!db")) {
    HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DB, TargetAddress,
                                     DEBUGGER_READ_PHYSICAL_ADDRESS,
                                     READ_FROM_KERNEL, Pid, Length);
  } else if (!FirstCommand.compare("!dc")) {
    HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DC, TargetAddress,
                                     DEBUGGER_READ_PHYSICAL_ADDRESS,
                                     READ_FROM_KERNEL, Pid, Length);
  } else if (!FirstCommand.compare("!dd")) {
    HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DD, TargetAddress,
                                     DEBUGGER_READ_PHYSICAL_ADDRESS,
                                     READ_FROM_KERNEL, Pid, Length);
  } else if (!FirstCommand.compare("!dq")) {
    HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DQ, TargetAddress,
                                     DEBUGGER_READ_PHYSICAL_ADDRESS,
                                     READ_FROM_KERNEL, Pid, Length);
  }
  //
  // Disassembler (!u or u)
  //
  else if (!FirstCommand.compare("u")) {
    HyperDbgReadMemoryAndDisassemble(
        DEBUGGER_SHOW_COMMAND_DISASSEMBLE, TargetAddress,
        DEBUGGER_READ_VIRTUAL_ADDRESS, READ_FROM_KERNEL, Pid, Length);
  } else if (!FirstCommand.compare("!u")) {
    HyperDbgReadMemoryAndDisassemble(
        DEBUGGER_SHOW_COMMAND_DISASSEMBLE, TargetAddress,
        DEBUGGER_READ_PHYSICAL_ADDRESS, READ_FROM_KERNEL, Pid, Length);
  }
}

void CommandConnectHelp() {
  ShowMessages(".connect : connects to a remote or local machine to start "
               "debugging.\n\n");
  ShowMessages("syntax : \t.connect [ip] [port]\n");
  ShowMessages("\t\te.g : .connect 192.168.1.5 50000\n");
  ShowMessages("\t\te.g : .connect local\n");
}
void CommandConnect(vector<string> SplittedCommand) {

  if (SplittedCommand.size() == 1) {
    //
    // Means that user entered just a connect so we have to
    // ask to connect to what ?
    //
    ShowMessages("incorrect use of '.connect'\n\n");
    CommandConnectHelp();
    return;
  } else if (SplittedCommand.at(1) == "local" && SplittedCommand.size() == 2) {
    //
    // connect to local debugger
    //
    ShowMessages("local debug current system\n");
    g_IsConnectedToDebugger = true;

    return;
  } else if (SplittedCommand.size() == 3) {

    string ip = SplittedCommand.at(1);
    string port = SplittedCommand.at(2);

    //
    // means that probably wants to connect to a remote
    // system, let's first check the if the parameters are
    // valid
    //
    if (!ValidateIP(ip)) {
      ShowMessages("incorrect ip address\n");
      return;
    }
    if (!IsNumber(port) || stoi(port) > 65535 || stoi(port) < 0) {
      ShowMessages("incorrect port\n");
      return;
    }

    //
    // connect to remote debugger
    //
    ShowMessages("local debug remote system\n");
    g_IsConnectedToDebugger = true;

    return;
  } else {
    ShowMessages("incorrect use of '.connect'\n\n");
    CommandConnectHelp();
    return;
  }
}

void CommandDisconnectHelp() {
  ShowMessages(".disconnect : disconnect from a debugging session (it won't "
               "unload the modules).\n\n");
  ShowMessages("syntax : \.disconnect\n");
}
void CommandDisconnect(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of '.disconnect'\n\n");
    CommandDisconnectHelp();
    return;
  }
  if (!g_IsConnectedToDebugger) {
    ShowMessages("You're not connected to any instance of HyperDbg, did you "
                 "use '.connect'? \n");
    return;
  }
  //
  // Disconnect the session
  //
  g_IsConnectedToDebugger = false;
  ShowMessages("successfully disconnected\n");
}

void CommandLoadHelp() {
  ShowMessages("load : installs the driver and load the kernel modules.\n\n");
  ShowMessages("syntax : \tload\n");
}
void CommandLoad(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'load'\n\n");
    CommandLoadHelp();
    return;
  }
  if (!g_IsConnectedToDebugger) {
    ShowMessages("You're not connected to any instance of HyperDbg, did you "
                 "use '.connect'? \n");
    return;
  }
  ShowMessages("try to install driver...\n");
  if (HyperdbgInstallDriver()) {
    ShowMessages("Failed to install driver\n");
    return;
  }

  ShowMessages("try to install load kernel modules...\n");
  if (HyperdbgLoad()) {
    ShowMessages("Failed to load driver\n");
    return;
  }

  //
  // If we reach here so the module are loaded
  //
  g_IsDebuggerModulesLoaded = true;
}

void CommandUnloadHelp() {
  ShowMessages(
      "unload : unloads the kernel modules and uninstalls the drivers.\n\n");
  ShowMessages("syntax : \tunload\n");
}
void CommandUnload(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'unload'\n\n");
    CommandLoadHelp();
    return;
  }
  if (!g_IsConnectedToDebugger) {
    ShowMessages("You're not connected to any instance of HyperDbg, did you "
                 "use '.connect'? \n");
    return;
  }

  if (g_IsDebuggerModulesLoaded) {
    HyperdbgUnload();

    //
    // Installing Driver
    //
    if (HyperdbgUninstallDriver()) {
      ShowMessages("Failed to uninstall driver\n");
    }
  } else {
    ShowMessages("there is nothing to unload\n");
  }
}

void CommandCpuHelp() {
  ShowMessages("cpu : collects a report from cpu features.\n\n");
  ShowMessages("syntax : \tcpu\n");
}
void CommandCpu(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'cpu'\n\n");
    CommandCpuHelp();
    return;
  }
  ReadCpuDetails();
}

void CommandExitHelp() {
  ShowMessages(
      "exit : unload and uninstalls the drivers and closes the debugger.\n\n");
  ShowMessages("syntax : \texit\n");
}
void CommandExit(vector<string> SplittedCommand) {

  if (SplittedCommand.size() != 1) {
    ShowMessages("incorrect use of 'exit'\n\n");
    CommandExitHelp();
    return;
  }

  //
  // unload and exit
  //
  if (g_IsDebuggerModulesLoaded) {
    HyperdbgUnload();

    //
    // Installing Driver
    //
    if (HyperdbgUninstallDriver()) {
      ShowMessages("Failed to uninstall driver\n");
    }
  }

  exit(0);
}

/**
 * @brief Interpret commands
 *
 * @param Command The text of command
 * @return int returns return zero if it was successful or non-zero if there was
 * error
 */
int _cdecl HyperdbgInterpreter(const char *Command) {

  string CommandString(Command);

  //
  // Convert to lower case
  //
  transform(CommandString.begin(), CommandString.end(), CommandString.begin(),
            [](unsigned char c) { return std::tolower(c); });

  vector<string> SplittedCommand{Split(CommandString, ' ')};

  //
  // Check if user entered an empty imput
  //
  if (SplittedCommand.empty()) {
    ShowMessages("\n");
    return 0;
  }

  string FirstCommand = SplittedCommand.front();

  if (!FirstCommand.compare("clear") || !FirstCommand.compare("cls") ||
      !FirstCommand.compare(".cls")) {
    CommandClearScreen();
  } else if (!FirstCommand.compare(".connect")) {
    CommandConnect(SplittedCommand);
  } else if (!FirstCommand.compare("connect")) {
    ShowMessages("Couldn't resolve error at '%s', did you mean '.connect'?",
                 FirstCommand.c_str());
  } else if (!FirstCommand.compare("disconnect")) {
    ShowMessages("Couldn't resolve error at '%s', did you mean '.disconnect'?",
                 FirstCommand.c_str());
  } else if (!FirstCommand.compare(".disconnect")) {
    CommandDisconnect(SplittedCommand);
  } else if (!FirstCommand.compare("load")) {
    CommandLoad(SplittedCommand);
  } else if (!FirstCommand.compare("exit") || !FirstCommand.compare(".exit")) {
    CommandExit(SplittedCommand);
  } else if (!FirstCommand.compare("unload")) {
    CommandUnload(SplittedCommand);
  } else if (!FirstCommand.compare("cpu")) {
    CommandCpu(SplittedCommand);
  } else if (!FirstCommand.compare("lm")) {
    CommandLm(SplittedCommand);
  } else if (!FirstCommand.compare("db") || !FirstCommand.compare("dc") ||
             !FirstCommand.compare("dd") || !FirstCommand.compare("dq") ||
             !FirstCommand.compare("!db") || !FirstCommand.compare("!dc") ||
             !FirstCommand.compare("!dd") || !FirstCommand.compare("!dq") ||
             !FirstCommand.compare("!u") || !FirstCommand.compare("u")) {
    CommandReadMemoryAndDisassembler(SplittedCommand);
  } else if (!FirstCommand.compare("!hiddenhook") ||
             !FirstCommand.compare("bh")) {
    CommandHiddenHook(SplittedCommand);
  } else {
    ShowMessages("Couldn't resolve error at '%s'", FirstCommand.c_str());
    ShowMessages("\n");
  }

  return 0;
}
