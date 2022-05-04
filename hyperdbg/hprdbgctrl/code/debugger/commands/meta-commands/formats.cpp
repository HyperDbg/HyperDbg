/**
 * @file formats.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief .formats command
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief help of help command :)
 *
 * @return VOID
 */
VOID
CommandFormatsHelp()
{
    ShowMessages(".formats : shows a value or register in different formats.\n\n");

    ShowMessages("syntax : \t.formats [Expression (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : .formats nt!ExAllocatePoolWithTag\n");
    ShowMessages("\t\te.g : .formats nt!Kd_DEFAULT_Mask\n");
    ShowMessages("\t\te.g : .formats nt!Kd_DEFAULT_Mask+5\n");
    ShowMessages("\t\te.g : .formats 55\n");
    ShowMessages("\t\te.g : .formats @rax\n");
    ShowMessages("\t\te.g : .formats @rbx+@rcx\n");
    ShowMessages("\t\te.g : .formats $pid\n");
}

/**
 * @brief show results of .formats command
 *
 * @param U64Value
 * @return VOID
 */
VOID
CommandFormatsShowResults(UINT64 U64Value)
{
    time_t       t;
    struct tm *  tmp;
    char         MY_TIME[50];
    unsigned int Character;

    time(&t);

    //
    // localtime() uses the time pointed by t ,
    // to fill a tm structure with the values that
    // represent the corresponding local time.
    //

    tmp = localtime(&t);

    //
    // using strftime to display time
    //
    strftime(MY_TIME, sizeof(MY_TIME), "%x - %I:%M%p", tmp);

    ShowMessages("evaluate expression:\n");
    ShowMessages("Hex :        %s\n", SeparateTo64BitValue(U64Value).c_str());
    ShowMessages("Decimal :    %d\n", U64Value);
    ShowMessages("Octal :      %o\n", U64Value);

    ShowMessages("Binary :     ");
    PrintBits(sizeof(UINT64), &U64Value);

    ShowMessages("\nChar :       ");

    //
    // iterate through 8, 8 bits (8*6)
    //
    unsigned char * TempCharacter = (unsigned char *)&U64Value;
    for (size_t j = 0; j < sizeof(UINT64); j++)
    {
        Character = (unsigned int)TempCharacter[j];

        if (isprint(Character))
        {
            ShowMessages("%c", Character);
        }
        else
        {
            ShowMessages(".");
        }
    }
    ShowMessages("\nTime :       %s\n", MY_TIME);
    ShowMessages("Float :      %4.2f %+.0e %E\n", U64Value, U64Value, U64Value);
    ShowMessages("Double :     %.*e\n", DECIMAL_DIG, U64Value);
}

/**
 * @brief handler of .formats command
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandFormats(vector<string> SplittedCommand, string Command)
{
    UINT64  ConstantValue = 0;
    BOOLEAN HasError      = TRUE;

    if (SplittedCommand.size() == 1)
    {
        ShowMessages("incorrect use of '.formats'\n\n");
        CommandFormatsHelp();
        return;
    }

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove .formats from it
    //
    Command.erase(0, 8);

    //
    // Trim it again
    //
    Trim(Command);

    //
    // Evaluate a single expression
    //
    ConstantValue = ScriptEngineEvalSingleExpression(Command, &HasError);

    if (HasError)
    {
        ShowMessages("err, couldn't resolve error at '%s'\n", Command.c_str());
    }
    else
    {
        //
        // Show formats results for a constant
        //
        CommandFormatsShowResults(ConstantValue);
    }
}
