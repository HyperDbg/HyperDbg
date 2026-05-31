/**
 * @file tools.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief general functions used in test project
 * @details
 * @version 0.1
 * @date 2021-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Convert a UINT64 value to a hex string
 *
 * @param Value the value to convert
 * @return std::string the hex string representation
 */
std::string
Uint64ToString(UINT64 Value)
{
    ostringstream Os;
    Os << setw(16) << setfill('0') << hex << Value;
    return Os.str();
}

/**
 * @brief Replace the first occurrence of a substring in a string
 *
 * @param Str the string to modify
 * @param From the substring to search for
 * @param To the replacement substring
 * @return BOOLEAN TRUE if the replacement was made, FALSE otherwise
 */
BOOLEAN
StringReplace(std::string & Str, const std::string & From, const std::string & To)
{
    SIZE_T StartPos = Str.find(From);
    if (StartPos == string::npos)
        return FALSE;
    Str.replace(StartPos, From.length(), To);
    return TRUE;
}

/**
 * @brief Convert a C-string to a std::string
 *
 * @param Str the C-string to convert
 * @return std::string the resulting string object
 */
std::string
ConvertToString(CHAR * Str)
{
    string Result(Str);

    return Result;
}
