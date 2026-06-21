/**
 * @file pe.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .pe command
 * @details
 * @version 0.1
 * @date 2021-12-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of the .pe command
 *
 * @return VOID
 */
VOID
CommandPeHelp()
{
    ShowMessages(".pe : parses portable executable (PE) files, displays header metadata, and dumps sections.\n\n");

    ShowMessages("syntax : \t.pe [header] [FilePath (string)]\n");
    ShowMessages("syntax : \t.pe [section] [SectionName (string)] [FilePath (string)]\n");
    ShowMessages("\n.pe section dumps are capped at 1 MiB per matching section and 4 MiB total.\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .pe header c:\\reverse\\myfile.exe\n");
    ShowMessages("\t\te.g : .pe section .text \"c:\\reverse files\\myfile.exe\"\n");
    ShowMessages("\t\te.g : .pe section .rdata \"c:\\reverse files\\myfile.exe\"\n");
}

/**
 * @brief .pe command handler
 *
 * @param CommandTokens
 * @param Command
 * @return VOID
 */
VOID
CommandPe(vector<CommandToken> CommandTokens, string Command)
{
    BOOLEAN Is32Bit = FALSE;
    wstring Filepath;
    string  TempFilePath;
    BOOLEAN ShowDumpOfSection = FALSE;

    if (CommandTokens.size() <= 2)
    {
        ShowMessages("err, incorrect use of the '%s' command\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandPeHelp();
        return;
    }

    //
    // Check for first option
    //
    if (CompareLowerCaseStrings(CommandTokens.at(1), "section"))
    {
        if (CommandTokens.size() == 3)
        {
            ShowMessages("err, incorrect use of the '%s' command\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
            CommandPeHelp();
            return;
        }

        if (CommandTokens.size() != 4)
        {
            ShowMessages("err, incorrect use of the '%s' command\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
            CommandPeHelp();
            return;
        }
        ShowDumpOfSection = TRUE;
        TempFilePath      = GetCaseSensitiveStringFromCommandToken(CommandTokens.at(3));
    }
    else if (CompareLowerCaseStrings(CommandTokens.at(1), "header"))
    {
        if (CommandTokens.size() != 3)
        {
            ShowMessages("err, incorrect use of the '%s' command\n\n",
                         GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
            CommandPeHelp();
            return;
        }

        ShowDumpOfSection = FALSE;
        TempFilePath      = GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2));
    }
    else
    {
        //
        // Couldn't resolve or unknown parameter
        //
        ShowMessages("err, couldn't resolve error at '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(1)).c_str());
        CommandPeHelp();
        return;
    }

    //
    // Convert path to wstring
    //
    StringToWString(Filepath, TempFilePath);

    //
    // TEMPORARY LINUX SHIM:
    //   std::wstring stores native wchar_t, which is 4 bytes on Linux, but the
    //   HyperDbg WCHAR type is 2 bytes (UINT16) and the PE-parser path APIs take
    //   a const WCHAR *. The cast below exists ONLY so the project compiles on
    //   Linux. On Linux it produces a bogus 2-byte reinterpretation of the 4-byte
    //   buffer; that is acceptable for now only because Linux file I/O is still
    //   stubbed (the path is never actually opened). On Windows WCHAR == wchar_t,
    //   so it is a plain, correct pointer with no reinterpretation.
    //
    //   TODO (Linux): remove this cast once real Linux file I/O lands. The proper
    //   fix is to convert the 4-byte wchar_t path into a 2-byte WCHAR/UTF-16
    //   buffer (e.g. a std::wstring -> WCHAR helper) and pass that, so the PE
    //   parser receives a valid path. The same conversion is needed by
    //   PlatformMapFileReadOnly / PlatformOpenFileForWriting.
    //
#ifdef __linux__
    const WCHAR * FilepathW = (const WCHAR *)Filepath.c_str();
#else
    const WCHAR * FilepathW = Filepath.c_str();
#endif

    //
    // Detect whether PE is 32-bit or 64-bit
    //
    if (!PeIsPE32BitOr64Bit(FilepathW, &Is32Bit))
    {
        //
        // File was invalid, the error message is shown in the above function
        //
        return;
    }

    //
    // Parse PE file
    //
    if (!ShowDumpOfSection)
    {
        PeShowSectionInformationAndDump(FilepathW, NULL, Is32Bit);
    }
    else
    {
        PeShowSectionInformationAndDump(FilepathW, GetCaseSensitiveStringFromCommandToken(CommandTokens.at(2)).c_str(), Is32Bit);
    }
}
