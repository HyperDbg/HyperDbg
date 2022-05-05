/**
 * @file common-utils.cpp
 * @author Alee Amini (alee@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief common utils
 * @details
 * @version 0.1
 * @date 2021-06-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief check if a file exist or not
 * 
 * @param FileName path of file
 * @return BOOLEAN shows whether the file exist or not
 */
BOOLEAN
IsFileExists(const std::string & FileName)
{
    struct stat buffer;
    return (stat(FileName.c_str(), &buffer) == 0);
}

/**
 * @brief check if a path exist or not
 * @param DirPath path of dir
 * @return BOOLEAN shows whether the file exist or not
 */
BOOLEAN
IsDirExists(const std::string & DirPath)
{
    DWORD Ftyp = GetFileAttributesA(DirPath.c_str());
    if (Ftyp == INVALID_FILE_ATTRIBUTES)
        return FALSE; //something is wrong with your path!

    if (Ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return TRUE; // this is a directory!

    return FALSE; // this is not a directory!
}

/**
 * @brief create a directory recursivly
 * 
 * @param Path path of file
 * 
 * @return BOOLEAN 
 */
BOOLEAN
CreateDirectoryRecursive(const std::string & Path)
{
    size_t Pos = 0;
    do
    {
        Pos = Path.find_first_of("\\/", Pos + 1);
        CreateDirectoryA(Path.substr(0, Pos).c_str(), NULL);

    } while (Pos != std::string::npos && Pos <= Path.size());

    if (IsDirExists(Path))
        return TRUE;

    return FALSE;
}

/**
 * @brief general split command
 *
 * @param s target string
 * @param c splitter (delimiter)
 * @return const vector<string>
 */
const std::vector<std::string>
Split(const std::string & s, const char & c)
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
