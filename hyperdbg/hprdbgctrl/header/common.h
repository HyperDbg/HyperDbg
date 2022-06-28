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
//			    	 Definitions                //
//////////////////////////////////////////////////

#define AssertReturn return;

#define AssertReturnFalse return FALSE;

#define AssertReturnOne return 1;

#define ASSERT_MESSAGE_DRIVER_NOT_LOADED "handle of the driver not found, probably the driver is not loaded.Did you use 'load' command ?\n"

#define AssertReturnStmt(expr, stmt, rc) \
    do                                   \
    {                                    \
        if (expr)                        \
        {                                \
            /* likely */                 \
        }                                \
        else                             \
        {                                \
            stmt;                        \
            rc;                          \
        }                                \
    } while (0)

#define AssertShowMessageReturnStmt(expr, message, rc) \
    do                                                 \
    {                                                  \
        if (expr)                                      \
        {                                              \
            /* likely */                               \
        }                                              \
        else                                           \
        {                                              \
            ShowMessages(message);                     \
            rc;                                        \
        }                                              \
    } while (0)

/**
 * @brief Size of each page (4096 bytes)
 *
 */
#define PAGE_SIZE 0x1000

/**
 * @brief Aligning a page
 *
 */
#define PAGE_ALIGN(Va) ((PVOID)((ULONG_PTR)(Va) & ~(PAGE_SIZE - 1)))

/**
 * @brief Cpuid to get virtual address width
 *
 */
#define CPUID_ADDR_WIDTH 0x80000008

//////////////////////////////////////////////////
//			    	 Spinlocks                  //
//////////////////////////////////////////////////

void
SpinlockLock(volatile LONG * Lock);

void
SpinlockLockWithCustomWait(volatile LONG * Lock, unsigned MaximumWait);

void
SpinlockUnlock(volatile LONG * Lock);

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

VOID
GetCpuid(UINT32 Func, UINT32 SubFunc, int * CpuInfo);

BOOLEAN
CheckCpuSupportRtm();

UINT32
Getx86VirtualAddressWidth();

BOOLEAN
CheckMemoryAccessSafety(UINT64 TargetAddress, UINT32 Size);
