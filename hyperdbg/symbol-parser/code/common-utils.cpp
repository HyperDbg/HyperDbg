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
        return FALSE; // something is wrong with your path!

    if (Ftyp & FILE_ATTRIBUTE_DIRECTORY)
        return TRUE; // this is a directory!

    return FALSE; // this is not a directory!
}

/**
 * @brief create a directory recursively
 *
 * @param Path path of file
 *
 * @return BOOLEAN
 */
BOOLEAN
CreateDirectoryRecursive(const std::string & Path)
{
    SIZE_T Pos = 0;
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
 * @brief Split path and arguments and handle strings between quotes
 *
 * @param Qargs the vector to store the result of split
 * @param Command the command string to split
 * @return VOID
 */
VOID
SplitPathAndArgs(std::vector<std::string> & Qargs, const std::string & Command)
{
    int     Len  = (int)Command.length();
    BOOLEAN Qot  = FALSE;
    BOOLEAN Sqot = FALSE;
    int  ArgLen;

    for (int i = 0; i < Len; i++)
    {
        INT Start = i;
        if (Command[i] == '\"')
        {
            Qot = TRUE;
        }
        else if (Command[i] == '\'')
            Sqot = TRUE;

        if (Qot)
        {
            i++;
            Start++;
            while (i < Len && Command[i] != '\"')
                i++;
            if (i < Len)
                Qot = FALSE;
            ArgLen = i - Start;
            i++;
        }
        else if (Sqot)
        {
            i++;
            while (i < Len && Command[i] != '\'')
                i++;
            if (i < Len)
                Sqot = FALSE;
            ArgLen = i - Start;
            i++;
        }
        else
        {
            while (i < Len && Command[i] != ' ')
                i++;
            ArgLen = i - Start;
        }

        string Temp = Command.substr(Start, ArgLen);
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
 * @brief general split command
 *
 * @param s target string
 * @param c splitter (delimiter)
 * @return const vector<string>
 */
const std::vector<std::string>
Split(const std::string & s, const char & c)
{
    string         Buff {""};
    vector<string> V;

    for (auto n : s)
    {
        if (n != c)
            Buff += n;
        else if (n == c && !Buff.empty())
        {
            V.push_back(Buff);
            Buff.clear();
        }
    }
    if (!Buff.empty())
        V.push_back(Buff);

    return V;
}
