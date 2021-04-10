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

//////////////////////////////////////////////////
//			    	 Functions                  //
//////////////////////////////////////////////////

VOID
PrintBits(size_t const size, void const * const ptr);

VOID
ReplaceAll(string & str, const string & from, const string & to);

const vector<string>
Split(const string & s, const char & c);

BOOLEAN
IsNumber(const string & str);

vector<string>
SplitIp(const string & str, char delim);

BOOLEAN
IsHexNotation(string s);

vector<char>
HexToBytes(const string & hex);

BOOLEAN
ConvertStringToUInt64(string TextToConvert, PUINT64 Result);

BOOLEAN
ConvertStringToUInt32(string TextToConvert, PUINT32 Result);

BOOLEAN
HasEnding(string const & fullString, string const & ending);

BOOLEAN
ValidateIP(string ip);

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

//////////////////////////////////////////////////
//            	    Structures                  //
//////////////////////////////////////////////////

/**
 * @brief this structure is copied from Process Hacker source code (ntldr.h)
 *
 */
typedef struct _RTL_PROCESS_MODULE_INFORMATION
{
    HANDLE Section;
    PVOID  MappedBase;
    PVOID  ImageBase;
    ULONG  ImageSize;
    ULONG  Flags;
    USHORT LoadOrderIndex;
    USHORT InitOrderIndex;
    USHORT LoadCount;
    USHORT OffsetToFileName;
    UCHAR  FullPathName[256];
} RTL_PROCESS_MODULE_INFORMATION, *PRTL_PROCESS_MODULE_INFORMATION;

/**
 * @brief this structure is copied from Process Hacker source code (ntldr.h)
 *
 */
typedef struct _RTL_PROCESS_MODULES
{
    ULONG                          NumberOfModules;
    RTL_PROCESS_MODULE_INFORMATION Modules[1];
} RTL_PROCESS_MODULES, *PRTL_PROCESS_MODULES;
