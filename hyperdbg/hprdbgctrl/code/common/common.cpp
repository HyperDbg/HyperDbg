/**
 * @file common.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief HyperDbg general functions for reading and converting and etc
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_RtmSupport;
extern UINT32  g_VirtualAddressWidth;

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
BOOL
Replace(std::string & str, const std::string & from, const std::string & to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return FALSE;
    str.replace(start_pos, from.length(), to);
    return TRUE;
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
        else if (n == c && !buff.empty())
        {
            v.push_back(buff);
            buff.clear();
        }
    }
    if (!buff.empty())
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
 * @brief check whether the string is hex or not
 *
 * @param s
 * @return BOOLEAN
 */
BOOLEAN
IsHexNotation(const string & s)
{
    BOOLEAN IsAnyThing = FALSE;

    for (auto & CptrChar : s)
    {
        IsAnyThing = TRUE;

        if (!isxdigit(CptrChar))
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
 * @brief check whether the string is decimal or not
 *
 * @param s
 * @return BOOLEAN
 */
BOOLEAN
IsDecimalNotation(const string & s)
{
    BOOLEAN IsAnyThing = FALSE;

    for (auto & CptrChar : s)
    {
        IsAnyThing = TRUE;

        if (!isdigit(CptrChar))
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
 * @brief check and convert string to a 64 bit unsigned integer and also
 *  check for special notations like 0x, 0n, etc.
 *
 * @param TextToConvert the target string
 * @param Result result will be save to the pointer
 *
 * @return BOOLEAN shows whether the conversion was successful or not
 */
BOOLEAN
ConvertStringToUInt64(string TextToConvert, PUINT64 Result)
{
    BOOLEAN IsDecimal = FALSE; // By default everything is hex

    if (TextToConvert.rfind("0x", 0) == 0 || TextToConvert.rfind("0X", 0) == 0 ||
        TextToConvert.rfind("\\x", 0) == 0 ||
        TextToConvert.rfind("\\X", 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 2);
    }
    else if (TextToConvert.rfind('x', 0) == 0 ||
             TextToConvert.rfind('X', 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 1);
    }
    else if (TextToConvert.rfind("0n", 0) == 0 || TextToConvert.rfind("0N", 0) == 0 ||
             TextToConvert.rfind("\\n", 0) == 0 ||
             TextToConvert.rfind("\\N", 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 2);
        IsDecimal     = TRUE;
    }
    else if (TextToConvert.rfind('n', 0) == 0 ||
             TextToConvert.rfind('N', 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 1);
        IsDecimal     = TRUE;
    }

    //
    // Remove '`' (if any)
    //
    TextToConvert.erase(remove(TextToConvert.begin(), TextToConvert.end(), '`'),
                        TextToConvert.end());

    if (IsDecimal)
    {
        if (!IsDecimalNotation(TextToConvert))
        {
            //
            // Not decimal
            //
            return FALSE;
        }
        else
        {
            errno                                 = 0;
            char *                       unparsed = NULL;
            const char *                 s        = TextToConvert.c_str();
            const unsigned long long int n        = strtoull(s, &unparsed, 10);

            if (errno || (!n && s == unparsed))
            {
                // fflush(stdout);
                // perror(s);
                return FALSE;
            }

            *Result = n;
            return TRUE;
        }
    }
    else
    {
        //
        // It's not decimal
        //
        if (!IsHexNotation(TextToConvert))
        {
            //
            // Not decimal and not hex!
            //
            return FALSE;
        }
        else
        {
            //
            // It's a hex number
            //
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
    }
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
    BOOLEAN IsDecimal = FALSE; // By default everything is hex

    if (TextToConvert.rfind("0x", 0) == 0 || TextToConvert.rfind("0X", 0) == 0 ||
        TextToConvert.rfind("\\x", 0) == 0 ||
        TextToConvert.rfind("\\X", 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 2);
    }
    else if (TextToConvert.rfind('x', 0) == 0 ||
             TextToConvert.rfind('X', 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 1);
    }
    else if (TextToConvert.rfind("0n", 0) == 0 || TextToConvert.rfind("0N", 0) == 0 ||
             TextToConvert.rfind("\\n", 0) == 0 ||
             TextToConvert.rfind("\\N", 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 2);
        IsDecimal     = TRUE;
    }
    else if (TextToConvert.rfind('n', 0) == 0 ||
             TextToConvert.rfind('N', 0) == 0)
    {
        TextToConvert = TextToConvert.erase(0, 1);
        IsDecimal     = TRUE;
    }

    TextToConvert.erase(remove(TextToConvert.begin(), TextToConvert.end(), '`'),
                        TextToConvert.end());

    if (IsDecimal)
    {
        if (!IsDecimalNotation(TextToConvert))
        {
            return FALSE;
        }
        else
        {
            try
            {
                int i   = std::stoi(TextToConvert);
                *Result = i;
                return TRUE;
            }
            catch (std::invalid_argument const & e)
            {
                //
                // Bad input: std::invalid_argument thrown
                //
                return FALSE;
            }
            catch (std::out_of_range const & e)
            {
                //
                // Integer overflow: std::out_of_range thrown
                //
                return FALSE;
            }

            return FALSE;
        }
    }
    else
    {
        //
        // It's not decimal
        //
        if (!IsHexNotation(TextToConvert))
        {
            return FALSE;
        }
        else
        {
            //
            // It's hex numer
            //
            UINT32 TempResult;
            TempResult = stoi(TextToConvert, nullptr, 16);

            //
            // Apply the results
            //
            *Result = TempResult;

            return TRUE;
        }
    }
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
ValidateIP(const string & ip)
{
    //
    // split the string into tokens
    //
    vector<string> list = Split(ip, '.');

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
        ShowMessages("err, in LookupPrivilegeValue (%x)\n", GetLastError());
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
        ShowMessages("err, in AdjustTokenPrivileges (%x)\n", GetLastError());
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
    {
        ShowMessages("err, the token does not have the specified privilege (ACCESS DENIED!)\n");
        ShowMessages("make sure to run it with administrator privileges\n");
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

/**
 * @brief check if a file exist or not (ASCII)
 *
 * @param FileName path of file
 * @return BOOLEAN shows whether the file exist or not
 */
BOOLEAN
IsFileExistA(const char * FileName)
{
    struct stat buffer;
    return (stat(FileName, &buffer) == 0);
}

/**
 * @brief check if a file exist or not (wide-char)
 *
 * @param FileName path of file
 * @return BOOLEAN shows whether the file exist or not
 */
BOOLEAN
IsFileExistW(const wchar_t * FileName)
{
    struct _stat64i32 buffer;
    return (_wstat(FileName, &buffer) == 0);
}

/**
 * @brief Is empty character
 *
 * @param Text
 */
BOOLEAN
IsEmptyString(char * Text)
{
    size_t Len;

    if (Text == NULL || Text[0] == '\0')
    {
        return TRUE;
    }

    Len = strlen(Text);
    for (size_t i = 0; i < Len; i++)
    {
        if (Text[i] != ' ' && Text[i] != '\t' && Text[i] != ' \n')
        {
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief Get config path
 *
 * @param ConfigPath
 */
VOID
GetConfigFilePath(PWCHAR ConfigPath)
{
    WCHAR CurrentPath[MAX_PATH] = {0};

    //
    // Get path file of current exe
    //
    GetModuleFileNameW(NULL, CurrentPath, MAX_PATH);

    //
    // Remove exe file name
    //
    PathRemoveFileSpecW(CurrentPath);

    //
    // Combine current exe path with config file name
    //
    PathCombineW(ConfigPath, CurrentPath, CONFIG_FILE_NAME);
}

/**
 * @brief Create a list of special files in a directory
 *
 * @param Directory
 * @param Extension
 * @return std::vector<std::string>
 */
std::vector<std::string>
ListDirectory(const std::string & Directory, const std::string & Extension)
{
    WIN32_FIND_DATAA         FindData;
    HANDLE                   Find     = INVALID_HANDLE_VALUE;
    std::string              FullPath = Directory + "\\" + Extension;
    std::vector<std::string> DirList;

    Find = FindFirstFileA(FullPath.c_str(), &FindData);

    if (Find == INVALID_HANDLE_VALUE)
        throw std::runtime_error("invalid handle value! please check your path...");

    while (FindNextFileA(Find, &FindData) != 0)
    {
        DirList.push_back(Directory + "\\" + std::string(FindData.cFileName));
    }

    FindClose(Find);

    return DirList;
}

/**
 * @brief convert std::string to std::wstring
 *
 * @param ws
 * @param s
 * @return VOID
 */
VOID
StringToWString(std::wstring & ws, const std::string & s)
{
    std::wstring wsTmp(s.begin(), s.end());

    ws = wsTmp;
}

/**
 * @brief Split path and arguments and handle strings between quotes
 *
 * @param Qargs
 * @param Command
 * @return VOID
 */
VOID
SplitPathAndArgs(std::vector<std::string> & Qargs, const std::string & Command)
{
    int  Len = Command.length();
    bool Qot = false, Sqot = false;
    int  ArgLen;

    for (int i = 0; i < Len; i++)
    {
        int start = i;
        if (Command[i] == '\"')
        {
            Qot = true;
        }
        else if (Command[i] == '\'')
            Sqot = true;

        if (Qot)
        {
            i++;
            start++;
            while (i < Len && Command[i] != '\"')
                i++;
            if (i < Len)
                Qot = false;
            ArgLen = i - start;
            i++;
        }
        else if (Sqot)
        {
            i++;
            while (i < Len && Command[i] != '\'')
                i++;
            if (i < Len)
                Sqot = false;
            ArgLen = i - start;
            i++;
        }
        else
        {
            while (i < Len && Command[i] != ' ')
                i++;
            ArgLen = i - start;
        }

        string Temp = Command.substr(start, ArgLen);
        if (!Temp.empty() && Temp != " ")
        {
            Qargs.push_back(Temp);
        }
    }

    /*
    for (int i = 0; i < Qargs.size(); i++)
    {
        std::cout << Qargs[i] << std::endl;
    }

    std::cout << Qargs.size();

    if (Qot || Sqot)
        std::cout << "One of the quotes is open\n";
    */
}

/**
 * @brief Find case insensitive sub string in a given substring
 *
 * @param Input
 * @param ToSearch
 * @param Pos
 * @return size_t
 */
size_t
FindCaseInsensitive(std::string Input, std::string ToSearch, size_t Pos)
{
    // Convert complete given String to lower case
    std::transform(Input.begin(), Input.end(), Input.begin(), ::tolower);
    // Convert complete given Sub String to lower case
    std::transform(ToSearch.begin(), ToSearch.end(), ToSearch.begin(), ::tolower);
    // Find sub string in given string
    return Input.find(ToSearch, Pos);
}

/**
 * @brief Find case insensitive sub string in a given substring (unicode)
 *
 * @param Input
 * @param ToSearch
 * @param Pos
 * @return size_t
 */
size_t
FindCaseInsensitiveW(std::wstring Input, std::wstring ToSearch, size_t Pos)
{
    // Convert complete given String to lower case
    std::transform(Input.begin(), Input.end(), Input.begin(), ::tolower);
    // Convert complete given Sub String to lower case
    std::transform(ToSearch.begin(), ToSearch.end(), ToSearch.begin(), ::tolower);
    // Find sub string in given string
    return Input.find(ToSearch, Pos);
}

/**
 * @brief Convert vector<string> to char*
 * @details use it like :
 *  std::transform(vs.begin(), vs.end(), std::back_inserter(vc), ConvertStringVectorToCharPointerArray);
 *  from: https://stackoverflow.com/questions/7048888/stdvectorstdstring-to-char-array
 *
 * @param Input
 * @param ToSearch
 * @param Pos
 * @return size_t
 */
char *
ConvertStringVectorToCharPointerArray(const std::string & s)
{
    char * pc = new char[s.size() + 1];
    std::strcpy(pc, s.c_str());
    return pc;
}

/**
 * @brief Get cpuid results
 *
 * @param UINT32 Func
 * @param UINT32 SubFunc
 * @param int * CpuInfo
 * @return VOID
 */
VOID
GetCpuid(UINT32 Func, UINT32 SubFunc, int * CpuInfo)
{
    __cpuidex(CpuInfo, Func, SubFunc);
}

/**
 * @brief Get virtual address width for x86 processors
 *
 * @return UINT32
 */
UINT32
Getx86VirtualAddressWidth()
{
    int Regs[4];

    GetCpuid(CPUID_ADDR_WIDTH, 0, Regs);

    //
    // Extracting bit 15:8 from eax register
    //
    return ((Regs[0] >> 8) & 0x0ff);
}

/**
 * @brief Check whether the processor supports RTM or not
 *
 * @return BOOLEAN
 */
BOOLEAN
CheckCpuSupportRtm()
{
    int     Regs1[4];
    int     Regs2[4];
    BOOLEAN Result;

    //
    // TSX is controlled via MSR_IA32_TSX_CTRL.  However, support for this
    // MSR is enumerated by ARCH_CAP_TSX_MSR bit in MSR_IA32_ARCH_CAPABILITIES.
    //
    // TSX control (aka MSR_IA32_TSX_CTRL) is only available after a
    // microcode update on CPUs that have their MSR_IA32_ARCH_CAPABILITIES
    // bit MDS_NO=1. CPUs with MDS_NO=0 are not planned to get
    // MSR_IA32_TSX_CTRL support even after a microcode update. Thus,
    // tsx= cmdline requests will do nothing on CPUs without
    // MSR_IA32_TSX_CTRL support.
    //

    GetCpuid(0, 0, Regs1);
    GetCpuid(7, 0, Regs2);

    //
    // Check RTM and MPX extensions in order to filter out TSX on Haswell CPUs
    //
    Result = Regs1[0] >= 0x7 && (Regs2[1] & 0x4800) == 0x4800;

    return Result;
}

/**
 * @brief Checks if the address is canonical based on x86 processor's
 * virtual address width or not
 * @param VAddr virtual address to check
 * @param IsKernelAddress Filled to show whether the address is a
 * kernel address or user-address
 * @brief IsKernelAddress wouldn't check for page attributes, it
 * just checks the address convention in Windows
 *
 * @return BOOLEAN
 */
BOOLEAN
CheckCanonicalVirtualAddress(UINT64 VAddr, PBOOLEAN IsKernelAddress)
{
    UINT64 Addr = (UINT64)VAddr;
    UINT64 MaxVirtualAddrLowHalf, MinVirtualAddressHighHalf;

    //
    // Get processor's address width for VA
    //
    UINT32 AddrWidth = g_VirtualAddressWidth;

    //
    // get max address in lower-half canonical addr space
    // e.g. if width is 48, then 0x00007FFF_FFFFFFFF
    //
    MaxVirtualAddrLowHalf = ((UINT64)1ull << (AddrWidth - 1)) - 1;

    //
    // get min address in higher-half canonical addr space
    // e.g., if width is 48, then 0xFFFF8000_00000000
    //
    MinVirtualAddressHighHalf = ~MaxVirtualAddrLowHalf;

    //
    // Check to see if the address in a canonical address
    //
    if ((Addr > MaxVirtualAddrLowHalf) && (Addr < MinVirtualAddressHighHalf))
    {
        *IsKernelAddress = FALSE;
        return FALSE;
    }

    //
    // Set whether it's a kernel address or not
    //
    if (MinVirtualAddressHighHalf < Addr)
    {
        *IsKernelAddress = TRUE;
    }
    else
    {
        *IsKernelAddress = FALSE;
    }

    return TRUE;
}

/**
 * @brief This function checks whether the address is valid or not using
 * Intel TSX
 *
 * @param Address Address to check
 *
 * @param UINT32 ProcId
 * @return BOOLEAN Returns true if the address is valid; otherwise, false
 */
BOOLEAN
CheckIfAddressIsValidUsingTsx(UINT64 Address)
{
    UINT32  Status = 0;
    BOOLEAN Result = FALSE;
    CHAR    TempContent;

    if ((Status = _xbegin()) == _XBEGIN_STARTED)
    {
        //
        // Try to read the memory
        //
        TempContent = *(CHAR *)Address;
        _xend();

        //
        // No error, address is valid
        //
        Result = TRUE;
    }
    else
    {
        //
        // Address is not valid, it aborts the tsx rtm
        //
        Result = FALSE;
    }

    return Result;
}

/**
 * @brief Check the safety to access the memory
 * @param TargetAddress
 * @param Size
 *
 * @return BOOLEAN
 */
BOOLEAN
CheckMemoryAccessSafety(UINT64 TargetAddress, UINT32 Size)
{
    BOOLEAN IsKernelAddress;
    BOOLEAN Result = FALSE;

    //
    // First, we check if the address is canonical based
    // on Intel processor's virtual address width
    //
    if (!CheckCanonicalVirtualAddress(TargetAddress, &IsKernelAddress))
    {
        //
        // No need for further check, address is invalid
        //
        return FALSE;
    }

    //
    // We cannot check a kernel-mode address here in user-mode
    //
    if (IsKernelAddress)
    {
        return FALSE;
    }

    //
    // We'll check address with TSX if it supports TSX RTM
    //
    if (g_RtmSupport)
    {
        //
        // *** The guest supports Intel TSX ***
        //

        UINT64 AddressToCheck =
            (CHAR *)TargetAddress + Size - ((CHAR *)PAGE_ALIGN(TargetAddress));

        if (AddressToCheck > PAGE_SIZE)
        {
            //
            // Address should be accessed in more than one page
            //
            UINT64 ReadSize = AddressToCheck;

            while (Size != 0)
            {
                ReadSize = (UINT64)PAGE_ALIGN(TargetAddress + PAGE_SIZE) - TargetAddress;

                if (ReadSize == PAGE_SIZE && Size < PAGE_SIZE)
                {
                    ReadSize = Size;
                }

                if (!CheckIfAddressIsValidUsingTsx(TargetAddress))
                {
                    //
                    // Address is not valid
                    //
                    return FALSE;
                }

                /*
                ShowMessages("Addr From : %llx to Addr To : %llx | ReadSize : %llx\n",
                             TargetAddress,
                             TargetAddress + ReadSize,
                             ReadSize);
                */

                //
                // Apply the changes to the next addresses (if any)
                //
                Size          = Size - ReadSize;
                TargetAddress = TargetAddress + ReadSize;
            }
        }
        else
        {
            if (!CheckIfAddressIsValidUsingTsx(TargetAddress))
            {
                //
                // Address is not valid
                //
                return FALSE;
            }
        }
    }
    else
    {
        //
        // *** processor doesn't support RTM ***
        //

        //
        // There is no way to perfom this check! The below implementation doesn't satisfy
        // our needs for address checks, but we're not trying to ask kernel about it as
        // HyperDbg's script engine is not designed to be runned these functions in user-mode
        // so we left it unimplemented to avoid crashes in the main program
        //
        return FALSE;

        //
        // Check if memory is safe and present
        //

        UINT64 AddressToCheck =
            (CHAR *)TargetAddress + Size - ((CHAR *)PAGE_ALIGN(TargetAddress));

        if (AddressToCheck > PAGE_SIZE)
        {
            //
            // Address should be accessed in more than one page
            //
            UINT64 ReadSize = AddressToCheck;

            while (Size != 0)
            {
                ReadSize = (UINT64)PAGE_ALIGN(TargetAddress + PAGE_SIZE) - TargetAddress;

                if (ReadSize == PAGE_SIZE && Size < PAGE_SIZE)
                {
                    ReadSize = Size;
                }

                try
                {
                    UINT64 ReadingTest = *((UINT64 *)TargetAddress);
                }
                catch (...)
                {
                    //
                    // Address is not valid
                    //
                    return FALSE;
                }

                /*
                ShowMessages("Addr From : %llx to Addr To : %llx | ReadSize : %llx\n",
                             TargetAddress,
                             TargetAddress + ReadSize,
                             ReadSize);
                */

                //
                // Apply the changes to the next addresses (if any)
                //
                Size          = Size - ReadSize;
                TargetAddress = TargetAddress + ReadSize;
            }
        }
        else
        {
            try
            {
                UINT64 ReadingTest = *((UINT64 *)TargetAddress);
            }
            catch (...)
            {
                //
                // Address is not valid
                //
                return FALSE;
            }
        }
    }

    //
    // If we've reached here, the address was valid
    //
    return TRUE;
}
