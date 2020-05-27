/**
 * @file common.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief header for HyperDbg's general functions for reading and converting and
 * etc
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#pragma once

void PrintBits(size_t const size, void const *const ptr);
void ReplaceAll(std::string &str, const std::string &from,
                const std::string &to);
const vector<string> Split(const string &s, const char &c);
bool IsNumber(const string &str);
vector<string> SplitIp(const string &str, char delim);
bool IsHexNotation(std::string s);
vector<char> HexToBytes(const std::string &hex);
BOOLEAN ConvertStringToUInt64(string TextToConvert, PUINT64 Result);
BOOLEAN ConvertStringToUInt32(string TextToConvert, PUINT32 Result);
BOOLEAN HasEnding(std::string const &fullString, std::string const &ending);
bool ValidateIP(string ip);

//
// These structures are copied from Process Hacker source code (ntldr.h)
//
typedef struct _RTL_PROCESS_MODULE_INFORMATION {
  HANDLE Section;
  PVOID MappedBase;
  PVOID ImageBase;
  ULONG ImageSize;
  ULONG Flags;
  USHORT LoadOrderIndex;
  USHORT InitOrderIndex;
  USHORT LoadCount;
  USHORT OffsetToFileName;
  UCHAR FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

typedef struct _RTL_PROCESS_MODULES {
  ULONG NumberOfModules;
  RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;
