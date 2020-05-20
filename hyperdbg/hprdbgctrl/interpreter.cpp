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

UINT64 g_EventTag = DebuggerEventTagStartSeed;
extern LIST_ENTRY g_EventTrace;
extern BOOLEAN g_EventTraceInitialized;

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
void PrintBits(size_t const size, void const *const ptr) {
  unsigned char *b = (unsigned char *)ptr;
  unsigned char byte;
  int i, j;

  for (i = size - 1; i >= 0; i--) {
    for (j = 7; j >= 0; j--) {
      byte = (b[i] >> j) & 1;
      ShowMessages("%u", byte);
    }
    ShowMessages(" ", byte);
  }
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

void ReplaceAll(std::string &str, const std::string &from,
                const std::string &to) {
  if (from.empty())
    return;
  size_t start_pos = 0;
  while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length(); // In case 'to' contains 'from', like replacing
                              // 'x' with 'yx'
  }
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

bool IsHexNotation(std::string s) {
  BOOLEAN IsAnyThing = FALSE;
  for (char &c : s) {
    IsAnyThing = TRUE;
    if (!isxdigit(c)) {
      return FALSE;
    }
  }
  if (IsAnyThing) {
    return TRUE;
  }
  return FALSE;
}

vector<char> HexToBytes(const std::string &hex) {
  std::vector<char> bytes;

  for (unsigned int i = 0; i < hex.length(); i += 2) {
    std::string byteString = hex.substr(i, 2);
    char byte = (char)strtol(byteString.c_str(), NULL, 16);
    bytes.push_back(byte);
  }

  return bytes;
}

BOOLEAN ConvertStringToUInt64(string TextToConvert, PUINT64 Result) {

  if (!IsHexNotation(TextToConvert)) {
    return FALSE;
  }
  const char *Text = TextToConvert.c_str();
  errno = 0;
  unsigned long long result = strtoull(Text, NULL, 16);

  *Result = result;

  if (errno == EINVAL) {
    return FALSE;
  } else if (errno == ERANGE) {
    return TRUE;
  }
}

BOOLEAN ConvertStringToUInt32(string TextToConvert, PUINT32 Result) {

  UINT32 TempResult;
  if (!IsHexNotation(TextToConvert)) {
    return FALSE;
  }
  TempResult = stoi(TextToConvert, nullptr, 16);
  *Result = TempResult;
}

BOOLEAN HasEnding(std::string const &fullString, std::string const &ending) {
  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
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
/* ==============================================================================================
 */
UINT64 GetNewDebuggerEventTag() { return g_EventTag++; }

/* ==============================================================================================
 */

/**
 * @brief Interpret conditions (if an event has condition) and custom code
 * @details If this function returns true then it means that there is a condtion
 * or code buffer in this command split and the details are returned in the
 * input structure
 *
 * @param SplittedCommand All the commands
 */
BOOLEAN
InterpretConditionsAndCodes(vector<string> *SplittedCommand,
                            BOOLEAN IsConditionBuffer, PUINT64 BufferAddrss,
                            PUINT32 BufferLength) {
  BOOLEAN IsTextVisited = FALSE;
  BOOLEAN IsInState = FALSE;
  BOOLEAN IsEnded = FALSE;
  string Temp;
  string AppendedFinalBuffer;
  vector<string> SaveBuffer;
  vector<CHAR> ParsedBytes;
  unsigned char *FinalBuffer;
  vector<int> IndexesToRemove;
  int Index = 0;

  for (auto Section : *SplittedCommand) {
    Index++;

    if (IsInState) {

      //
      // Check if the buffer is ended or not
      //
      if (!Section.compare("}")) {
        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);
        IsEnded = TRUE;
        break;
      }

      //
      // Check if the condition is end or not
      //
      if (HasEnding(Section, "}")) {
        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        //
        // remove the last character and append it to the ConditionBuffer
        //
        SaveBuffer.push_back(Section.substr(0, Section.size() - 1));

        IsEnded = TRUE;
        break;
      }

      //
      // Save to remove this string from the command
      //
      IndexesToRemove.push_back(Index);

      //
      // Add the codes into condition bi
      //
      SaveBuffer.push_back(Section);

      //
      // We want to stay in this condition
      //
      continue;
    }

    if (IsTextVisited && !Section.compare("{")) {
      //
      // Save to remove this string from the command
      //
      IndexesToRemove.push_back(Index);

      IsInState = TRUE;
      continue;
    }
    if (IsTextVisited && Section.rfind("{", 0) == 0) {
      //
      // Section starts with {
      //

      //
      // Check if it ends with }
      //
      if (HasEnding(Section, "}")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        Temp = Section.erase(0, 1);
        SaveBuffer.push_back(Temp.substr(0, Temp.size() - 1));

        IsEnded = TRUE;
        break;
      }
      //
      // Save to remove this string from the command
      //
      IndexesToRemove.push_back(Index);

      SaveBuffer.push_back(Section.erase(0, 1));

      IsInState = TRUE;
      continue;
    }

    if (IsConditionBuffer) {
      if (!Section.compare("condition")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        continue;
      }
    } else {
      //
      // It's code
      //
      if (!Section.compare("code")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        continue;
      }
    }

    if (IsConditionBuffer) {
      if (!Section.compare("condition{")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        IsInState = TRUE;
        continue;
      }
    } else {
      //
      // It's code
      //
      if (!Section.compare("code{")) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        IsInState = TRUE;
        continue;
      }
    }

    if (IsConditionBuffer) {
      if (Section.rfind("condition{", 0) == 0) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        IsInState = TRUE;

        if (!HasEnding(Section, "}")) {
          //
          // Section starts with condition{
          //
          SaveBuffer.push_back(Section.erase(0, 10));
          continue;
        } else {
          //
          // remove the last character and first character append it to the
          // ConditionBuffer
          //
          Temp = Section.erase(0, 10);
          SaveBuffer.push_back(Temp.substr(0, Temp.size() - 1));

          IsEnded = TRUE;
          break;
        }
      }
    } else {
      //
      // It's a code
      //

      if (Section.rfind("code{", 0) == 0) {

        //
        // Save to remove this string from the command
        //
        IndexesToRemove.push_back(Index);

        IsTextVisited = TRUE;
        IsInState = TRUE;

        if (!HasEnding(Section, "}")) {
          //
          // Section starts with condition{
          //
          SaveBuffer.push_back(Section.erase(0, 5));
          continue;
        } else {
          //
          // remove the last character and first character append it to the
          // ConditionBuffer
          //
          Temp = Section.erase(0, 5);
          SaveBuffer.push_back(Temp.substr(0, Temp.size() - 1));

          IsEnded = TRUE;
          break;
        }
      }
    }
  }

  //
  // Now we have everything in condition buffer
  // Check to see if it is empty or not
  //
  if (SaveBuffer.size() == 0) {
    //
    // Nothing in condition buffer, return zero
    //
    return FALSE;
  }

  //
  // Check if we see '}' at the end
  //
  if (!IsEnded) {
    //
    // Nothing in condition buffer, return zero
    //
    return FALSE;
  }

  //
  // Append a 'ret' at the end of the buffer
  //
  SaveBuffer.push_back("c3");

  //
  // If we reach here then there is sth in condition buffer
  //
  for (auto Section : SaveBuffer) {

    //
    // Check if the section is started with '0x'
    //
    if (Section.rfind("0x", 0) == 0 || Section.rfind("0X", 0) == 0 ||
        Section.rfind("\\x", 0) == 0 || Section.rfind("\\X", 0) == 0) {
      Temp = Section.erase(0, 2);
    } else if (Section.rfind("x", 0) == 0 || Section.rfind("X", 0) == 0) {
      Temp = Section.erase(0, 1);
    } else {
      Temp = Section;
    }

    //
    // replace \x s
    //
    ReplaceAll(Temp, "\\x", "");

    //
    // check if the buffer is aligned to 2
    //
    if (Temp.size() % 2 != 0) {

      //
      // Add a zero to the start of the buffer
      //
      Temp.insert(0, 1, '0');
    }

    if (!IsHexNotation(Temp)) {
      ShowMessages("Please enter condition code in a hex notation.\n");
      return FALSE;
    }
    AppendedFinalBuffer.append(Temp);
  }

  //
  // Convert it to vectored bytes
  //
  ParsedBytes = HexToBytes(AppendedFinalBuffer);

  //
  // Convert to a contigues buffer
  //
  FinalBuffer = (unsigned char *)malloc(ParsedBytes.size());
  std::copy(ParsedBytes.begin(), ParsedBytes.end(), FinalBuffer);

  //
  // Set the buffer and length
  //
  *BufferAddrss = (UINT64)FinalBuffer;
  *BufferLength = ParsedBytes.size();

  //
  // Removing the code or condition indexes from the command
  //
  int NewIndexToRemove = 0;
  for (auto IndexToRemove : IndexesToRemove) {
    NewIndexToRemove++;

    SplittedCommand->erase(SplittedCommand->begin() +
                           (IndexToRemove - NewIndexToRemove));
  }
  return TRUE;
}

/* ==============================================================================================
 */

void CommandClearScreen() { system("cls"); }

/* ==============================================================================================
 */

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

/* ==============================================================================================
 */

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
      if (!ConvertStringToUInt32(Section, &Pid)) {
        ShowMessages("Err, you should enter a valid proc id\n\n");
        return;
      }
      IsNextProcessId = false;
      continue;
    }

    if (IsNextLength == true) {

      if (!ConvertStringToUInt32(Section, &Length)) {
        ShowMessages("Err, you should enter a valid length\n\n");
        return;
      }
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

      if (!ConvertStringToUInt64(TempAddress, &TargetAddress)) {
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
  if (IsNextLength || IsNextProcessId) {
    ShowMessages("incorrect use of '%s' command\n\n", FirstCommand.c_str());
    CommandReadMemoryHelp();
    return;
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

/* ==============================================================================================
 */

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

/* ==============================================================================================
 */

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

/* ==============================================================================================
 */

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

/* ==============================================================================================
 */

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

/* ==============================================================================================
 */

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

/* ==============================================================================================
 */

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

/* ==============================================================================================
 */

void CommandFormatsHelp() {
  ShowMessages(".formats : Show a value or register in different formats.\n\n");
  ShowMessages("syntax : \t.formats [hex value | register]\n");
}
void CommandFormats(vector<string> SplittedCommand) {

  UINT64 u64Value;
  time_t t;
  struct tm *tmp;
  char MY_TIME[50];
  char Character;

  if (SplittedCommand.size() != 2) {
    ShowMessages("incorrect use of '.formats'\n\n");
    CommandFormatsHelp();
    return;
  }
  if (!ConvertStringToUInt64(SplittedCommand.at(1), &u64Value)) {
    ShowMessages("incorrect use of '.formats'\n\n");
    CommandFormatsHelp();
    return;
  }

  time(&t);

  //
  // localtime() uses the time pointed by t ,
  // to fill a tm structure with the values that
  // represent the corresponding local time.
  //

  tmp = localtime(&t);

  //
  // using strftime to display time
  //
  strftime(MY_TIME, sizeof(MY_TIME), "%x - %I:%M%p", tmp);

  ShowMessages("Evaluate expression:\n");
  ShowMessages("Hex :        %s\n", SeparateTo64BitValue(u64Value).c_str());
  ShowMessages("Decimal :    %d\n", u64Value);
  ShowMessages("Octal :      %o\n", u64Value);

  ShowMessages("Binary :     ");
  PrintBits(sizeof(UINT64), &u64Value);

  ShowMessages("\nChar :       ");
  //
  // iterate through 8, 8 bits (8*6)
  //
  for (size_t j = 0; j < 8; j++) {

    Character = (char)(((char *)&u64Value)[j]);

    if (isprint(Character)) {
      ShowMessages("%c", Character);
    } else {
      ShowMessages(".");
    }
  }
  ShowMessages("\nTime :       %s\n", MY_TIME);
  ShowMessages("Float :      %4.2f %+.0e %E\n", u64Value, u64Value, u64Value);
  ShowMessages("Double :     %.*e\n", DECIMAL_DIG, u64Value);
}

/* ==============================================================================================
 */

void CommandRdmsrHelp() {
  ShowMessages("rdmsr : Reads a model-specific register (MSR).\n\n");
  ShowMessages("syntax : \trdmsr [rcx (hex value)] core [core index (hex value "
               "- optional)]\n");
  ShowMessages("\t\te.g : rdmsr c0000082\n");
  ShowMessages("\t\te.g : rdmsr c0000082 core 2\n");
}
void CommandRdmsr(vector<string> SplittedCommand) {

  BOOL Status;
  BOOL IsNextCoreId = FALSE;
  BOOL SetMsr = FALSE;
  DEBUGGER_READ_AND_WRITE_ON_MSR MsrReadRequest;
  ULONG ReturnedLength;
  UINT64 Msr;
  UINT32 CoreNumer = DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES;
  SYSTEM_INFO SysInfo;
  DWORD NumCPU;

  if (SplittedCommand.size() >= 5) {
    ShowMessages("incorrect use of 'rdmsr'\n\n");
    CommandRdmsrHelp();
    return;
  }

  for (auto Section : SplittedCommand) {

    if (!Section.compare(SplittedCommand.at(0))) {
      continue;
    }

    if (IsNextCoreId) {
      if (!ConvertStringToUInt32(Section, &CoreNumer)) {
        ShowMessages("please specify a correct hex value for core id\n\n");
        CommandRdmsrHelp();
        return;
      }
      IsNextCoreId = FALSE;
      continue;
    }

    if (!Section.compare("core")) {
      IsNextCoreId = TRUE;
      continue;
    }

    if (SetMsr || !ConvertStringToUInt64(Section, &Msr)) {
      ShowMessages("please specify a correct hex value to be read\n\n");
      CommandRdmsrHelp();
      return;
    }
    SetMsr = TRUE;
  }
  //
  // Check if msr is set or not
  //
  if (!SetMsr) {
    ShowMessages("please specify a correct hex value to be read\n\n");
    CommandRdmsrHelp();
    return;
  }
  if (IsNextCoreId) {
    ShowMessages("please specify a correct hex value for core\n\n");
    CommandRdmsrHelp();
    return;
  }

  if (!DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return;
  }

  MsrReadRequest.ActionType = DEBUGGER_MSR_READ;
  MsrReadRequest.Msr = Msr;
  MsrReadRequest.CoreNumber = CoreNumer;

  //
  // Find logical cores count
  //
  GetSystemInfo(&SysInfo);
  NumCPU = SysInfo.dwNumberOfProcessors;

  //
  // allocate buffer for transfering messages
  //

  UINT64 *OutputBuffer = (UINT64 *)malloc(sizeof(UINT64) * NumCPU);

  ZeroMemory(OutputBuffer, sizeof(UINT64) * NumCPU);

  Status = DeviceIoControl(
      DeviceHandle,                     // Handle to device
      IOCTL_DEBUGGER_READ_OR_WRITE_MSR, // IO Control code
      &MsrReadRequest,                  // Input Buffer to driver.
      SIZEOF_READ_AND_WRITE_ON_MSR,     // Input buffer length
      OutputBuffer,                     // Output Buffer from driver.
      sizeof(UINT64) * NumCPU,          // Length of output buffer in bytes.
      &ReturnedLength,                  // Bytes placed in buffer.
      NULL                              // synchronous call
  );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return;
  }

  //
  // btw, %x is enough, no need to %llx
  //
  if (CoreNumer == DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES) {
    //
    // Show all cores
    //
    for (size_t i = 0; i < NumCPU; i++) {

      ShowMessages("core : 0x%x - msr[%llx] = %s\n", i, Msr,
                   SeparateTo64BitValue((OutputBuffer[i])).c_str());
    }
  } else {
    //
    // Show for a single-core
    //
    ShowMessages("core : 0x%x - msr[%llx] = %s\n", CoreNumer, Msr,
                 SeparateTo64BitValue((OutputBuffer[0])).c_str());
  }
}

/* ==============================================================================================
 */

void CommandWrmsrHelp() {
  ShowMessages("wrmsr : Writes on a model-specific register (MSR).\n\n");
  ShowMessages("syntax : \twrmsr [ecx (hex value)] [value to write - EDX:EAX "
               "(hex value)] core [core index (hex value - optional)]\n");
  ShowMessages("\t\te.g : wrmsr c0000082 fffff8077356f010\n");
  ShowMessages("\t\te.g : wrmsr c0000082 fffff8077356f010 core 2\n");
}
void CommandWrmsr(vector<string> SplittedCommand) {

  BOOL Status;
  BOOL IsNextCoreId = FALSE;
  BOOL SetMsr = FALSE;
  BOOL SetValue = FALSE;
  DEBUGGER_READ_AND_WRITE_ON_MSR MsrWriteRequest;
  UINT64 Msr;
  UINT64 Value = 0;
  UINT32 CoreNumer = DEBUGGER_READ_AND_WRITE_ON_MSR_APPLY_ALL_CORES;

  if (SplittedCommand.size() >= 6) {
    ShowMessages("incorrect use of 'wrmsr'\n\n");
    CommandWrmsrHelp();
    return;
  }

  for (auto Section : SplittedCommand) {

    if (!Section.compare(SplittedCommand.at(0))) {
      continue;
    }

    if (IsNextCoreId) {
      if (!ConvertStringToUInt32(Section, &CoreNumer)) {
        ShowMessages("please specify a correct hex value for core id\n\n");
        CommandWrmsrHelp();
        return;
      }
      IsNextCoreId = FALSE;
      continue;
    }

    if (!Section.compare("core")) {
      IsNextCoreId = TRUE;
      continue;
    }

    if (!SetMsr) {
      if (!ConvertStringToUInt64(Section, &Msr)) {
        ShowMessages("please specify a correct hex value to be read\n\n");
        CommandWrmsrHelp();
        return;
      } else {
        //
        // Means that the MSR is set, next we should read value
        //
        SetMsr = TRUE;
        continue;
      }
    }

    if (SetMsr) {
      if (!ConvertStringToUInt64(Section, &Value)) {
        ShowMessages(
            "please specify a correct hex value to put on the msr\n\n");
        CommandWrmsrHelp();
        return;
      } else {

        SetValue = TRUE;
        continue;
      }
    }
  }
  //
  // Check if msr is set or not
  //
  if (!SetMsr) {
    ShowMessages("please specify a correct hex value to write\n\n");
    CommandWrmsrHelp();
    return;
  }
  if (!SetValue) {
    ShowMessages("please specify a correct hex value to put on msr\n\n");
    CommandWrmsrHelp();
    return;
  }
  if (IsNextCoreId) {
    ShowMessages("please specify a correct hex value for core\n\n");
    CommandWrmsrHelp();
    return;
  }

  if (!DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return;
  }

  MsrWriteRequest.ActionType = DEBUGGER_MSR_WRITE;
  MsrWriteRequest.Msr = Msr;
  MsrWriteRequest.CoreNumber = CoreNumer;
  MsrWriteRequest.Value = Value;

  Status = DeviceIoControl(DeviceHandle,                     // Handle to device
                           IOCTL_DEBUGGER_READ_OR_WRITE_MSR, // IO Control code
                           &MsrWriteRequest, // Input Buffer to driver.
                           SIZEOF_READ_AND_WRITE_ON_MSR, // Input buffer length
                           NULL, // Output Buffer from driver.
                           NULL, // Length of output buffer in bytes.
                           NULL, // Bytes placed in buffer.
                           NULL  // synchronous call
  );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return;
  }

  ShowMessages("\n");
}

/* ==============================================================================================
 */

void CommandPteHelp() {
  ShowMessages("!pte : Find virtual address of different paging-levels.\n\n");
  ShowMessages("syntax : \t!pte [Virtual Address (hex value)]\n");
  ShowMessages("\t\te.g : !pte fffff801deadbeef\n");
}
void CommandPte(vector<string> SplittedCommand) {

  BOOL Status;
  ULONG ReturnedLength;
  UINT64 TargetVa;
  DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PteRead = {0};

  if (SplittedCommand.size() > 2) {
    ShowMessages("incorrect use of '!pte'\n\n");
    CommandPteHelp();
    return;
  }

  if (!ConvertStringToUInt64(SplittedCommand.at(1), &TargetVa)) {
    ShowMessages("incorrect address, please enter a valid virtual address\n\n");
    return;
  }

  if (!DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return;
  }
  //
  // Prepare the buffer
  // We use same buffer for input and output
  //
  PteRead.VirtualAddress = TargetVa;

  //
  // Send IOCTL
  //

  Status = DeviceIoControl(
      DeviceHandle,                                   // Handle to device
      IOCTL_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS, // IO Control code
      &PteRead,                                       // Input Buffer to driver.
      SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS, // Input buffer length
      &PteRead, // Output Buffer from driver.
      SIZEOF_DEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS, // Length of output
                                                       // buffer in bytes.
      &ReturnedLength, // Bytes placed in buffer.
      NULL             // synchronous call
  );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return;
  }

  //
  // Show the results
  //
  /*
    VA fffff8003abc9370
    PXE at FFFF83C1E0F07F80    PPE at FFFF83C1E0FF0000    PDE at
    FFFF83C1FE000EA8    PTE at FFFF83FC001D5E48 contains 0000000004108063
    contains 0000000004109063  contains 00000000026008E3  contains
    0000000000000000 pfn 4108      ---DA--KWEV  pfn 4109      ---DA--KWEV  pfn
    2600      --LDA--KWEV  LARGE PAGE pfn 27c9
  */
  ShowMessages("VA %llx\n", TargetVa);
  ShowMessages("PML4E (PXE) at %016llx\tcontains %016llx\nPDPT (PPE) at "
               "%016llx\tcontains "
               "%016llx\nPDE at %016llx\tcontains %016llx\n",
               PteRead.Pml4eVirtualAddress, PteRead.Pml4eValue,
               PteRead.PdpteVirtualAddress, PteRead.PdpteValue,
               PteRead.PdeVirtualAddress, PteRead.PdeValue);

  //
  // Check if it's a large PDE
  //
  if (PteRead.PdeVirtualAddress == PteRead.PteVirtualAddress) {
    ShowMessages("PDE is a large page, so it doesn't have a PTE\n");
  } else {
    ShowMessages("PTE at %016llx\tcontains %016llx\n",
                 PteRead.PteVirtualAddress, PteRead.PteValue);
  }
}

/* ==============================================================================================
 */

/**
 * @brief Interpret general event fields
 * @details If this function returns true then it means that there
 * was no error in parsing the general event details
 *
 * @param SplittedCommand All the commands
 */
BOOLEAN InterpretGeneralEventAndActionsFields(
    vector<string> *SplittedCommand, DEBUGGER_EVENT_TYPE_ENUM EventType,
    PDEBUGGER_GENERAL_EVENT_DETAIL *EventDetailsToFill,
    PUINT32 EventBufferLength, PDEBUGGER_GENERAL_ACTION *ActionDetailsToFill,
    PUINT32 ActionBufferLength) {

  PDEBUGGER_GENERAL_EVENT_DETAIL TempEvent;
  UINT64 ConditionBufferAddress;
  UINT32 ConditionBufferLength = 0;
  UINT64 CodeBufferAddress;
  UINT32 CodeBufferLength = 0;
  UINT32 LengthOfEventBuffer = 0;
  string CommandString;
  BOOLEAN HasConditionBuffer = FALSE;
  BOOLEAN HasCodeBuffer = FALSE;
  BOOLEAN IsNextCommandPid = FALSE;
  BOOLEAN IsNextCommandCoreId = FALSE;
  UINT32 CoreId;
  UINT32 ProcessId;
  vector<int> IndexesToRemove;
  int NewIndexToRemove = 0;
  int Index = 0;

  //
  // Create a command string to show in the history
  //
  for (auto Section : *SplittedCommand) {

    CommandString.append(Section);
    CommandString.append(" ");
  }
  //
  // Compute the size of buffer + 1 null for the end of buffer
  //

  UINT64 BufferOfCommandStringLength = CommandString.size() + 1;

  //
  // Allocate Buffer and zero for command to the buffer
  //
  PVOID BufferOfCommandString = malloc(BufferOfCommandStringLength);

  RtlZeroMemory(BufferOfCommandString, BufferOfCommandStringLength);

  //
  // Copy the string to the buffer
  //
  memcpy(BufferOfCommandString, CommandString.c_str(), CommandString.size());

  //
  // Check if there is a condition buffer in the command
  //
  if (!InterpretConditionsAndCodes(SplittedCommand, TRUE,
                                   &ConditionBufferAddress,
                                   &ConditionBufferLength)) {

    //
    // Indicate condition is not available
    //
    HasConditionBuffer = FALSE;

    //
    // ShowMessages("\nNo condition!\n");
    //

  } else {
    //
    // Indicate condition is available
    //
    HasConditionBuffer = TRUE;

    ShowMessages(
        "\n========================= Condition =========================\n");

    ShowMessages(
        "\nUINT64  DebuggerCheckForCondition(PGUEST_REGS Regs_RCX, PVOID "
        "Context_RDX)\n{\n");

    //
    // Disassemble the buffer
    //
    HyperDbgDisassembler((unsigned char *)ConditionBufferAddress, 0x0,
                         ConditionBufferLength);

    ShowMessages("}\n\n");

    ShowMessages(
        "=============================================================\n");
  }

  //
  // Check if there is a code buffer in the command
  //
  if (!InterpretConditionsAndCodes(SplittedCommand, FALSE, &CodeBufferAddress,
                                   &CodeBufferLength)) {

    //
    // Indicate code is not available
    //
    HasCodeBuffer = FALSE;
    //
    // ShowMessages("\nNo custom code!\n");
    //
  } else {

    //
    // Indicate code is available
    //
    HasCodeBuffer = TRUE;

    ShowMessages(
        "\n=========================    Code    =========================\n");
    ShowMessages("\nPVOID DebuggerRunCustomCodeFunc(PVOID "
                 "PreAllocatedBufferAddress_RCX, "
                 "PGUEST_REGS Regs_RDX, PVOID Context_R8)\n{\n");

    //
    // Disassemble the buffer
    //
    HyperDbgDisassembler((unsigned char *)CodeBufferAddress, 0x0,
                         CodeBufferLength);

    ShowMessages("}\n\n");

    ShowMessages(
        "=============================================================\n");
  }

  //
  // Create action and event based on previously parsed buffers
  // (DEBUGGER_GENERAL_ACTION)
  //
  // Allocate the buffer (with ConditionBufferLength and CodeBufferLength)
  //

  /*

  Layout of Buffer
   ________________________________
  |                                |
  |  DEBUGGER_GENERAL_EVENT_DETAIL |
  |                                |
  |________________________________|
  |                                |
  |       Condition Buffer         |
  |                                |
  |________________________________|
  |                                |
  |     DEBUGGER_GENERAL_ACTION    |
  |                                |
  |________________________________|
  |                                |
  |     Condition Custom Code      |
  |                                |
  |________________________________|


  */

  LengthOfEventBuffer =
      sizeof(DEBUGGER_GENERAL_EVENT_DETAIL) + ConditionBufferLength;

  TempEvent = (PDEBUGGER_GENERAL_EVENT_DETAIL)malloc(LengthOfEventBuffer);
  RtlZeroMemory(TempEvent, LengthOfEventBuffer);

  //
  // Check if buffer is availabe
  //
  if (TempEvent == NULL) {
    //
    // The heap is not available
    //
    return FALSE;
  }

  //
  // Event is enabled by default when created
  //
  TempEvent->IsEnabled = TRUE;

  //
  // Get a new tag for it
  //
  TempEvent->Tag = GetNewDebuggerEventTag();

  //
  // Set the core Id and Process Id to all cores and all
  // processes, next time we check whether the user needs
  // a special core or a special process then we change it
  //
  TempEvent->CoreId = DEBUGGER_EVENT_APPLY_TO_ALL_CORES;
  TempEvent->ProcessId = DEBUGGER_EVENT_APPLY_TO_ALL_PROCESSES;

  //
  // Set the event type
  //
  TempEvent->EventType = EventType;

  //
  // Get the current time
  //
  TempEvent->CreationTime = time(0);

  //
  // Set buffer string command
  //
  TempEvent->CommandStringBuffer = BufferOfCommandString;

  //
  // Fill the buffer of condition for event
  //
  if (HasConditionBuffer) {

    memcpy((PVOID)((UINT64)TempEvent + sizeof(DEBUGGER_GENERAL_EVENT_DETAIL)),
           (PVOID)ConditionBufferAddress, ConditionBufferLength);

    //
    // Set the size of the buffer for event condition
    //
    TempEvent->ConditionBufferSize = ConditionBufferLength;
  }

  //
  // Allocate the Action
  //
  UINT32 LengthOfActionBuffer =
      sizeof(DEBUGGER_GENERAL_ACTION) + CodeBufferLength;
  PDEBUGGER_GENERAL_ACTION TempAction =
      (PDEBUGGER_GENERAL_ACTION)malloc(LengthOfActionBuffer);

  RtlZeroMemory(TempAction, LengthOfActionBuffer);
  //
  // Fill the buffer of custom code for action
  //
  if (HasCodeBuffer) {

    memcpy((PVOID)((UINT64)TempAction + sizeof(DEBUGGER_GENERAL_ACTION)),
           (PVOID)CodeBufferAddress, CodeBufferLength);
    //
    // Set the action Tag
    //
    TempAction->EventTag = TempEvent->Tag;

    //
    // Set the action type
    //
    TempAction->ActionType = RUN_CUSTOM_CODE;

    //
    // Set the action buffer size
    //
    TempAction->CustomCodeBufferSize = CodeBufferLength;

    //
    // Increase the count of actions
    //
    TempEvent->CountOfActions = TempEvent->CountOfActions + 1;
  }

  //
  // Interpret rest of the command
  //

  for (auto Section : *SplittedCommand) {
    Index++;

    if (IsNextCommandPid) {

      if (!ConvertStringToUInt32(Section, &ProcessId)) {
        return FALSE;
      } else {
        //
        // Set the specific process id
        //
        TempEvent->ProcessId = ProcessId;
      }
      IsNextCommandPid = FALSE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }
    if (IsNextCommandCoreId) {
      if (!ConvertStringToUInt32(Section, &CoreId)) {
        return FALSE;
      } else {
        //
        // Set the specific core id
        //
        TempEvent->CoreId = CoreId;
      }
      IsNextCommandCoreId = FALSE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }

    if (!Section.compare("pid")) {
      IsNextCommandPid = TRUE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }
    if (!Section.compare("core")) {
      IsNextCommandCoreId = TRUE;

      //
      // Add index to remove it from the command
      //
      IndexesToRemove.push_back(Index);

      continue;
    }
  }

  //
  // Fill the address and length of event before release
  //
  *EventDetailsToFill = TempEvent;
  *EventBufferLength = LengthOfEventBuffer;

  //
  // Fill the address and length of action before release
  //
  *ActionDetailsToFill = TempAction;
  *ActionBufferLength = LengthOfActionBuffer;

  //
  // Remove the command that we interpreted above from the command
  //
  for (auto IndexToRemove : IndexesToRemove) {
    NewIndexToRemove++;
    SplittedCommand->erase(SplittedCommand->begin() +
                           (IndexToRemove - NewIndexToRemove));
  }

  //
  // Check if list is initialized or not
  //
  if (!g_EventTraceInitialized) {
    InitializeListHead(&g_EventTrace);
    g_EventTraceInitialized = TRUE;
  }

  //
  // Add the event to the trace list
  //
  InsertHeadList(&g_EventTrace, &(TempEvent->CommandsEventList));

  //
  // Everything is ok, let's return TRUE
  //
  return TRUE;
}

/* ==============================================================================================
 */

/**
 * @brief Register the event to the kernel
 */

BOOLEAN
SendEventToKernel(PDEBUGGER_GENERAL_EVENT_DETAIL Event,
                  UINT32 EventBufferLength) {
  BOOL Status;
  ULONG ReturnedLength;
  DEBUGGER_EVENT_AND_ACTION_REG_BUFFER ReturnedBuffer = {0};

  //
  // Test
  //
  /*
  ShowMessages("Tag : %llx\n", Event->Tag);
  ShowMessages("Command String : %s\n", Event->CommandStringBuffer);
  ShowMessages("CoreId : 0x%x\n", Event->CoreId);
  ShowMessages("Pid : 0x%x\n", Event->ProcessId);
  ShowMessages("Optional Param 1 : %llx\n", Event->OptionalParam1);
  ShowMessages("Optional Param 2 : %llx\n", Event->OptionalParam2);
  ShowMessages("Optional Param 3 : %llx\n", Event->OptionalParam3);
  ShowMessages("Optional Param 4 : %llx\n", Event->OptionalParam4);
  ShowMessages("Count of Actions : %d\n", Event->CountOfActions);
  ShowMessages("Event Type : %d\n", Event->EventType);
  return TRUE;
  */

  if (!DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return FALSE;
  }

  //
  // Send IOCTL
  //

  Status =
      DeviceIoControl(DeviceHandle,                  // Handle to device
                      IOCTL_DEBUGGER_REGISTER_EVENT, // IO Control code
                      Event,                         // Input Buffer to driver.
                      EventBufferLength,             // Input buffer length
                      &ReturnedBuffer, // Output Buffer from driver.
                      sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                                    // of
                                                                    // output
                                                                    // buffer
                                                                    // in
                                                                    // bytes.
                      &ReturnedLength, // Bytes placed in buffer.
                      NULL             // synchronous call
      );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return FALSE;
  }

  return TRUE;
}

/* ==============================================================================================
 */

/**
 * @brief Register the action to the event
 */

BOOLEAN
RegisterActionToEvent(PDEBUGGER_GENERAL_ACTION Action,
                      UINT32 ActionsBufferLength) {
  BOOL Status;
  ULONG ReturnedLength;
  DEBUGGER_EVENT_AND_ACTION_REG_BUFFER ReturnedBuffer = {0};

  //
  // Test
  //
  /*
  ShowMessages("Tag : %llx\n", Action->EventTag);
  ShowMessages("Action Type : %d\n", Action->ActionType);
  ShowMessages("Custom Code Buffer Size : 0x%x\n",
               Action->CustomCodeBufferSize);
  return TRUE;
  */

  if (!DeviceHandle) {
    ShowMessages("Handle not found, probably the driver is not loaded.\n");
    return FALSE;
  }

  //
  // Send IOCTL
  //

  Status =
      DeviceIoControl(DeviceHandle,                       // Handle to device
                      IOCTL_DEBUGGER_ADD_ACTION_TO_EVENT, // IO Control code
                      Action,              // Input Buffer to driver.
                      ActionsBufferLength, // Input buffer length
                      &ReturnedBuffer,     // Output Buffer from driver.
                      sizeof(DEBUGGER_EVENT_AND_ACTION_REG_BUFFER), // Length
                                                                    // of
                                                                    // output
                                                                    // buffer
                                                                    // in
                                                                    // bytes.
                      &ReturnedLength, // Bytes placed in buffer.
                      NULL             // synchronous call
      );

  if (!Status) {
    ShowMessages("Ioctl failed with code 0x%x\n", GetLastError());
    return FALSE;
  }

  return TRUE;
}
/* ==============================================================================================
 */

void CommandMonitorHelp() {
  ShowMessages("!monitor : Monitor address rang for read and writes.\n\n");
  ShowMessages("syntax : \t!monitor [attrib (r, w, rw)] [From Virtual Address "
               "(hex value)] [To Virtual Address (hex value)] core [core index "
               "(hex value)] pid [process id (hex value)]\n");

  ShowMessages("\t\te.g : !monitor rw fffff801deadb000 fffff801deadbfff\n");
  ShowMessages(
      "\t\te.g : !monitor r fffff801deadb000 fffff801deadbfff pid 400\n");
  ShowMessages("\t\te.g : !monitor w fffff801deadb000 fffff801deadbfff core 2 "
               "pid 400\n");
}

VOID CommandMonitor(vector<string> SplittedCommand) {

  PDEBUGGER_GENERAL_EVENT_DETAIL Event;
  PDEBUGGER_GENERAL_ACTION Action;
  UINT32 EventLength;
  UINT32 ActionLength;
  UINT64 TargetAddress;
  UINT64 OptionalParam1 = 0; // Set the 'from' target address
  UINT64 OptionalParam2 = 0; // Set the 'to' target address
  BOOLEAN SetFrom = FALSE;
  BOOLEAN SetTo = FALSE;

  if (SplittedCommand.size() < 4) {
    ShowMessages("incorrect use of '!monitor'\n");
    CommandMonitorHelp();
    return;
  }

  //
  // Interpret and fill the general event and action fields
  //
  //
  // We use HIDDEN_HOOK_READ_AND_WRITE here but it might be changed to
  // HIDDEN_HOOK_READ or HIDDEN_HOOK_WRITE it is because we are not sure what
  // kind event the user need
  //
  if (!InterpretGeneralEventAndActionsFields(
          &SplittedCommand, HIDDEN_HOOK_READ_AND_WRITE, &Event, &EventLength,
          &Action, &ActionLength)) {
    CommandMonitorHelp();
    return;
  }

  //
  // Interpret command specific details (if any)
  //

  for (auto Section : SplittedCommand) {
    if (!Section.compare("!monitor")) {
      continue;
    } else if (!Section.compare("r")) {
      Event->EventType = HIDDEN_HOOK_READ;
    } else if (!Section.compare("w")) {
      Event->EventType = HIDDEN_HOOK_WRITE;
    } else if (!Section.compare("rw") || !Section.compare("wr")) {
      Event->EventType = HIDDEN_HOOK_READ_AND_WRITE;

    } else {
      //
      // It's probably address
      //
      if (!SetFrom) {
        if (!ConvertStringToUInt64(Section, &OptionalParam1)) {
          //
          // Unkonwn parameter
          //
          ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
          CommandMonitorHelp();
          return;
        }
        SetFrom = TRUE;
      } else if (!SetTo) {
        if (!ConvertStringToUInt64(Section, &OptionalParam2)) {
          //
          // Unkonwn parameter
          //
          ShowMessages("Unknown parameter '%s'\n\n", Section.c_str());
          CommandMonitorHelp();
          return;
        }
        SetTo = TRUE;
      } else {
        //
        // Unkonwn parameter
        //
        ShowMessages("Unknown parameter '%s'\n", Section.c_str());
        CommandMonitorHelp();
        return;
      }
    }
  }
  if (OptionalParam1 > OptionalParam2) {
    //
    // 'from' is greater than 'to'
    //
    ShowMessages("Please choose the 'from' value first, then choose the 'to' "
                 "value.\n");
    return;
  }

  //
  // Set the optional parameters
  //
  Event->OptionalParam1 = OptionalParam1;
  Event->OptionalParam2 = OptionalParam2;

  //
  // Send the ioctl to the kernel for event registeration
  //
  SendEventToKernel(Event, EventLength);

  ShowMessages("\n-----------------------------------------------------\n");

  //
  // Add the event to the kernel
  //
  RegisterActionToEvent(Action, ActionLength);
}
/* ==============================================================================================
 */

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
  } else if (!FirstCommand.compare("wrmsr")) {
    CommandWrmsr(SplittedCommand);
  } else if (!FirstCommand.compare("rdmsr")) {
    CommandRdmsr(SplittedCommand);
  } else if (!FirstCommand.compare(".formats")) {
    CommandFormats(SplittedCommand);
  } else if (!FirstCommand.compare("!pte")) {
    CommandPte(SplittedCommand);
  } else if (!FirstCommand.compare("!monitor")) {
    CommandMonitor(SplittedCommand);
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
