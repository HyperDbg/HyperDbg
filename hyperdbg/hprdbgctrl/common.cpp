/**
 * @file common.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief HyperDbg general functions for reading and converting and etc
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

/**
 * @brief add ` between 64 bit values and convert them to string
 *
 * @param Value
 * @return string
 */
string
SeparateTo64BitValue(UINT64 Value)
{
    ostringstream OstringStream;
    string        Temp;

    OstringStream << setw(16) << setfill('0') << hex << Value;
    Temp = OstringStream.str();

    Temp.insert(8, 1, '`');
    return Temp;
}

/**
 * @brief print bits and bytes for d* commands
 *
 * @param size
 * @param ptr
 * @return VOID
 */
VOID
PrintBits(size_t const size, void const * const ptr)
{
    unsigned char * b = (unsigned char *)ptr;
    unsigned char   byte;
    int             i, j;

    for (i = size - 1; i >= 0; i--)
    {
        for (j = 7; j >= 0; j--)
        {
            byte = (b[i] >> j) & 1;
            ShowMessages("%u", byte);
        }
        ShowMessages(" ", byte);
    }
}

/**
 * @brief general replace all function
 *
 * @param str
 * @param from
 * @param to
 * @return VOID
 */
VOID
ReplaceAll(string & str, const string & from, const string & to)
{
    size_t SartPos = 0;

    if (from.empty())
        return;

    while ((SartPos = str.find(from, SartPos)) != std::string::npos)
    {
        str.replace(SartPos, from.length(), to);
        //
        // In case 'to' contains 'from', like replacing
        // 'x' with 'yx'
        //
        SartPos += to.length();
    }
}

/**
 * @brief general split command
 *
 * @param s target string
 * @param c splitter (delimiter)
 * @return const vector<string>
 */
const vector<string>
Split(const string & s, const char & c)
{
    string         buff {""};
    vector<string> v;

    for (auto n : s)
    {
        if (n != c)
            buff += n;
        else if (n == c && buff != "")
        {
            v.push_back(buff);
            buff = "";
        }
    }
    if (buff != "")
        v.push_back(buff);

    return v;
}

/**
 * @brief check if given string is a numeric string or not
 *
 * @param str
 * @return BOOLEAN
 */
BOOLEAN
IsNumber(const string & str)
{
    //
    // std::find_first_not_of searches the string for the first character
    // that does not match any of the characters specified in its arguments
    //
    return !str.empty() &&
           (str.find_first_not_of("[0123456789]") == std::string::npos);
}

/**
 * @brief Function to split string str using given delimiter
 *
 * @param str
 * @param delim
 * @return vector<string>
 */
vector<string>
SplitIp(const string & str, char delim)
{
    int            i = 0;
    vector<string> list;
    size_t         pos;

    pos = str.find(delim);

    while (pos != string::npos)
    {
        list.push_back(str.substr(i, pos - i));
        i   = ++pos;
        pos = str.find(delim, pos);
    }

    list.push_back(str.substr(i, str.length()));

    return list;
}

/**
 * @brief check whether the string is hex or not
 *
 * @param s
 * @return BOOLEAN
 */
BOOLEAN
IsHexNotation(string s)
{
    BOOLEAN IsAnyThing = FALSE;

    for (char & c : s)
    {
        IsAnyThing = TRUE;

        if (!isxdigit(c))
        {
            return FALSE;
        }
    }
    if (IsAnyThing)
    {
        return TRUE;
    }
    return FALSE;
}

/**
 * @brief converts hex to bytes
 *
 * @param hex
 * @return vector<char>
 */
vector<char>
HexToBytes(const string & hex)
{
    vector<char> Bytes;

    for (unsigned int i = 0; i < hex.length(); i += 2)
    {
        std::string byteString = hex.substr(i, 2);
        char        byte       = (char)strtol(byteString.c_str(), NULL, 16);
        Bytes.push_back(byte);
    }

    return Bytes;
}

/**
 * @brief check and convert string to a 64 bit unsigned it and also
 *  check for special notations like 0x etc.
 * @param TextToConvert the target string
 * @param Result result will be save to the pointer
 * @return BOOLEAN shows whether the conversion was successful or not
 */
BOOLEAN
ConvertStringToUInt64(string TextToConvert, PUINT64 Result)
{
    if (TextToConvert.rfind("0x", 0) == 0 || TextToConvert.rfind("0X", 0) == 0 ||
        TextToConvert.rfind("\\x", 0) == 0 ||
        TextToConvert.rfind("\\X", 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 2);
    }
    else if (TextToConvert.rfind("x", 0) == 0 ||
             TextToConvert.rfind("X", 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 1);
    }
    TextToConvert.erase(remove(TextToConvert.begin(), TextToConvert.end(), '`'),
                        TextToConvert.end());

    if (!IsHexNotation(TextToConvert))
    {
        return FALSE;
    }
    const char * Text         = TextToConvert.c_str();
    errno                     = 0;
    unsigned long long result = strtoull(Text, NULL, 16);

    *Result = result;

    if (errno == EINVAL)
    {
        return FALSE;
    }
    else if (errno == ERANGE)
    {
        return TRUE;
    }

    return TRUE;
}

/**
 * @brief check and convert string to a 32 bit unsigned it and also
 *  check for special notations like 0x etc.
 * @param TextToConvert the target string
 * @param Result result will be save to the pointer
 * @return BOOLEAN shows whether the conversion was successful or not
 */
BOOLEAN
ConvertStringToUInt32(string TextToConvert, PUINT32 Result)
{
    if (TextToConvert.rfind("0x", 0) == 0 || TextToConvert.rfind("0X", 0) == 0 ||
        TextToConvert.rfind("\\x", 0) == 0 ||
        TextToConvert.rfind("\\X", 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 2);
    }
    else if (TextToConvert.rfind("x", 0) == 0 ||
             TextToConvert.rfind("X", 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 1);
    }

    TextToConvert.erase(remove(TextToConvert.begin(), TextToConvert.end(), '`'),
                        TextToConvert.end());

    UINT32 TempResult;
    if (!IsHexNotation(TextToConvert))
    {
        return FALSE;
    }
    TempResult = stoi(TextToConvert, nullptr, 16);

    //
    // Apply the results
    //
    *Result = TempResult;

    return TRUE;
}

/**
 * @brief checks whether the string ends with a special string or not
 *
 * @param fullString
 * @param ending
 * @return BOOLEAN if true then it shows that string ends with another string
 * and if false then it shows that this string is not ended with the target
 * string
 */
BOOLEAN
HasEnding(string const & fullString, string const & ending)
{
    if (fullString.length() >= ending.length())
    {
        return (0 == fullString.compare(fullString.length() - ending.length(),
                                        ending.length(),
                                        ending));
    }
    else
    {
        return FALSE;
    }
}

/**
 * @brief Function to validate an IP address
 *
 * @param ip
 * @return BOOLEAN
 */
BOOLEAN
ValidateIP(string ip)
{
    //
    // split the string into tokens
    //
    vector<string> list = SplitIp(ip, '.');

    //
    // if token size is not equal to four
    //
    if (list.size() != 4)
        return FALSE;

    //
    // validate each token
    //
    for (string str : list)
    {
        //
        // verify that string is number or not and the numbers
        // are in the valid range
        //
        if (!IsNumber(str) || stoi(str) > 255 || stoi(str) < 0)
            return FALSE;
    }

    return TRUE;
}

/**
 * @brief Detect whether the VMX is supported or not
 *
 * @return true if vmx is supported
 * @return false if vmx is not supported
 */
BOOLEAN
VmxSupportDetection()
{
    //
    // Call asm function
    //
    return AsmVmxSupportDetection();
}

/**
 * @brief SetPrivilege enables/disables process token privilege
 *
 * @param hToken
 * @param lpszPrivilege
 * @param bEnablePrivilege
 * @return BOOL
 */
BOOL
SetPrivilege(HANDLE  hToken,          // access token handle
             LPCTSTR lpszPrivilege,   // name of privilege to enable/disable
             BOOL    bEnablePrivilege // to enable or disable privilege
)
{
    TOKEN_PRIVILEGES tp;
    LUID             luid;

    if (!LookupPrivilegeValue(NULL,          // lookup privilege on local system
                              lpszPrivilege, // privilege to lookup
                              &luid))        // receives LUID of privilege
    {
        ShowMessages("LookupPrivilegeValue error: %u\n", GetLastError());
        return FALSE;
    }

    tp.PrivilegeCount     = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    //
    // Enable the privilege or disable all privileges.
    //
    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
    {
        ShowMessages("AdjustTokenPrivileges error: %u\n", GetLastError());
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        ShowMessages("The token does not have the specified privilege. \n");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief trim from start of string (in place)
 *
 * @param s
 */
static inline void
ltrim(std::string & s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) { return !std::isspace(ch); }));
}

/**
 * @brief trim from the end of string (in place)
 *
 * @param s
 */
static inline void
rtrim(std::string & s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) { return !std::isspace(ch); })
                .base(),
            s.end());
}

/**
 * @brief trim from both ends and start of a string (in place)
 *
 * @param s
 */
void
Trim(std::string & s)
{
    ltrim(s);
    rtrim(s);
}

/**
 * @brief Remove all the spaces in a string
 *
 * @param str
 */
std::string
RemoveSpaces(std::string str)
{
    str.erase(remove(str.begin(), str.end(), ' '), str.end());
    return str;
}
