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

string SeparateTo64BitValue(UINT64 Value) {

  ostringstream OstringStream;
  string Temp;

  OstringStream << setw(16) << setfill('0') << hex << Value;
  Temp = OstringStream.str();

  Temp.insert(8, 1, '`');
  return Temp;
}

VOID PrintBits(size_t const size, void const *const ptr) {
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

VOID ReplaceAll(string &str, const string &from, const string &to) {

  size_t SartPos = 0;

  if (from.empty())
    return;

  while ((SartPos = str.find(from, SartPos)) != std::string::npos) {
    str.replace(SartPos, from.length(), to);
    //
    // In case 'to' contains 'from', like replacing
    // 'x' with 'yx'
    //
    SartPos += to.length();
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

//
// check if given string is a numeric string or not
//
BOOLEAN IsNumber(const string &str) {

  //
  // std::find_first_not_of searches the string for the first character
  // that does not match any of the characters specified in its arguments
  //
  return !str.empty() &&
         (str.find_first_not_of("[0123456789]") == std::string::npos);
}

//
// Function to split string str using given delimiter
//
vector<string> SplitIp(const string &str, char delim) {

  int i = 0;
  vector<string> list;
  size_t pos;

  pos = str.find(delim);

  while (pos != string::npos) {
    list.push_back(str.substr(i, pos - i));
    i = ++pos;
    pos = str.find(delim, pos);
  }

  list.push_back(str.substr(i, str.length()));

  return list;
}

BOOLEAN IsHexNotation(string s) {

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

vector<char> HexToBytes(const string &hex) {

  vector<char> Bytes;

  for (unsigned int i = 0; i < hex.length(); i += 2) {
    std::string byteString = hex.substr(i, 2);
    char byte = (char)strtol(byteString.c_str(), NULL, 16);
    Bytes.push_back(byte);
  }

  return Bytes;
}

BOOLEAN ConvertStringToUInt64(string TextToConvert, PUINT64 Result) {

  if (TextToConvert.rfind("0x", 0) == 0 || TextToConvert.rfind("0X", 0) == 0 ||
      TextToConvert.rfind("\\x", 0) == 0 ||
      TextToConvert.rfind("\\X", 0) == 0) {
    TextToConvert = TextToConvert.erase(0, 2);
  } else if (TextToConvert.rfind("x", 0) == 0 ||
             TextToConvert.rfind("X", 0) == 0) {
    TextToConvert = TextToConvert.erase(0, 1);
  }
  TextToConvert.erase(remove(TextToConvert.begin(), TextToConvert.end(), '`'),
                      TextToConvert.end());

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

  if (TextToConvert.rfind("0x", 0) == 0 || TextToConvert.rfind("0X", 0) == 0 ||
      TextToConvert.rfind("\\x", 0) == 0 ||
      TextToConvert.rfind("\\X", 0) == 0) {
    TextToConvert = TextToConvert.erase(0, 2);
  } else if (TextToConvert.rfind("x", 0) == 0 ||
             TextToConvert.rfind("X", 0) == 0) {
    TextToConvert = TextToConvert.erase(0, 1);
  }

  TextToConvert.erase(remove(TextToConvert.begin(), TextToConvert.end(), '`'),
                      TextToConvert.end());

  UINT32 TempResult;
  if (!IsHexNotation(TextToConvert)) {
    return FALSE;
  }
  TempResult = stoi(TextToConvert, nullptr, 16);

  //
  // Apply the results
  //
  *Result = TempResult;
}

BOOLEAN HasEnding(string const &fullString, string const &ending) {

  if (fullString.length() >= ending.length()) {
    return (0 == fullString.compare(fullString.length() - ending.length(),
                                    ending.length(), ending));
  } else {
    return FALSE;
  }
}

//
// Function to validate an IP address
//
BOOLEAN ValidateIP(string ip) {

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
  for (string str : list) {
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
 * @brief Detect VMX support
 *
 * @return true if vmx is supported
 * @return false if vmx is not supported
 */
BOOLEAN VmxSupportDetection() {
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
BOOLEAN SetPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege,
                     BOOL bEnablePrivilege) {
  LUID luid;
  BOOLEAN bRet = FALSE;

  if (LookupPrivilegeValue(NULL, lpszPrivilege, &luid)) {
    TOKEN_PRIVILEGES tp;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = (bEnablePrivilege) ? SE_PRIVILEGE_ENABLED : 0;
    //
    //  Enable the privilege or disable all privileges.
    //
    if (AdjustTokenPrivileges(hToken, FALSE, &tp, NULL, (PTOKEN_PRIVILEGES)NULL,
                              (PDWORD)NULL)) {
      //
      //  Check to see if you have proper access.
      //  You may get "ERROR_NOT_ALL_ASSIGNED".
      //
      bRet = (GetLastError() == ERROR_SUCCESS);
    }
  }
  return bRet;
}

// trim from start (in place)
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
        }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

// trim from both ends (in place)
void Trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}