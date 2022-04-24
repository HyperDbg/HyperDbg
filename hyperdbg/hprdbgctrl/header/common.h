/**
 * @file common.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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

//////////////////////////////////////////////////
//			    	 Functions                  //
//////////////////////////////////////////////////

VOID
PrintBits(size_t const size, void const * const ptr);

BOOL
Replace(std::string & str, const std::string & from, const std::string & to);

VOID
ReplaceAll(string & str, const string & from, const string & to);

const vector<string>
Split(const string & s, const char & c);

BOOLEAN
IsNumber(const string & str);

vector<string>
SplitIp(const string & str, char delim);

BOOLEAN
IsHexNotation(const string & s);

vector<char>
HexToBytes(const string & hex);

BOOLEAN
ConvertStringToUInt64(string TextToConvert, PUINT64 Result);

BOOLEAN
ConvertStringToUInt32(string TextToConvert, PUINT32 Result);

BOOLEAN
HasEnding(string const & fullString, string const & ending);

BOOLEAN
ValidateIP(const string & ip);

BOOLEAN
VmxSupportDetection();

BOOL
SetPrivilege(HANDLE  hToken,          // access token handle
             LPCTSTR lpszPrivilege,   // name of privilege to enable/disable
             BOOL    bEnablePrivilege // to enable or disable privilege
);

void
Trim(std::string & s);

std::string
RemoveSpaces(std::string str);

BOOLEAN
IsFileExistA(const char * FileName);

BOOLEAN
IsFileExistW(const wchar_t * FileName);

VOID
GetConfigFilePath(PWCHAR ConfigPath);

VOID
StringToWString(std::wstring & ws, const std::string & s);

VOID
SplitPathAndArgs(std::vector<std::string> & Qargs, const std::string & Command);

size_t
FindCaseInsensitive(std::string Input, std::string ToSearch, size_t Pos);

size_t
FindCaseInsensitiveW(std::wstring Input, std::wstring ToSearch, size_t Pos);

char *
ConvertStringVectorToCharPointerArray(const std::string & s);

std::vector<std::string>
ListDirectory(const std::string & Directory, const std::string & Extension);

BOOLEAN
IsEmptyString(char * Text);

//////////////////////////////////////////////////
//            	    Structures                  //
//////////////////////////////////////////////////

///**
// * @brief this structure is copied from Process Hacker source code (ntldr.h)
// *
// */
// typedef struct _RTL_PROCESS_MODULE_INFORMATION
//{
//    HANDLE Section;
//    PVOID  MappedBase;
//    PVOID  ImageBase;
//    ULONG  ImageSize;
//    ULONG  Flags;
//    UINT16 LoadOrderIndex;
//    UINT16 InitOrderIndex;
//    UINT16 LoadCount;
//    UINT16 OffsetToFileName;
//    UCHAR  FullPathName[256];
//} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;
//
///**
// * @brief this structure is copied from Process Hacker source code (ntldr.h)
// *
// */
// typedef struct _RTL_PROCESS_MODULES
//{
//    ULONG                          NumberOfModules;
//    RTL_PROCESS_MODULE_INFORMATION Modules[1];
//} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;
