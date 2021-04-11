/**
 * @file formats.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .formats command
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
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of help command :)
 *
 * @return VOID
 */
VOID
CommandFormatsHelp()
{
    ShowMessages(".formats : Show a value or register in different formats.\n\n");
    ShowMessages("syntax : \t.formats [hex value | register | Expression]\n");
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
    time_t      t;
    struct tm * tmp;
    char        MY_TIME[50];
    char        Character;

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
    for (size_t j = 0; j < 8; j++)
    {
        Character = (char)(((char *)&U64Value)[j]);

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
    PVOID  CodeBuffer;
    UINT64 BufferAddress;
    UINT64 ConstantValue = 0;
    UINT32 BufferLength;
    UINT32 Pointer;

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
    // Remove print.formats from it
    //
    Command.erase(0, 8);

    //
    // Trim it again
    //
    Trim(Command);

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send over serial
        //
        //
        // Prepend and append 'print(' and ')'
        //
        Command.insert(0, "formats(");
        Command.append(");");

        //
        // TODO: end of string must have a whitspace. fix it.
        //
        Command.append(" ");
        // Expr = " x = 4 >> 1; ";

        //
        // Run script engine handler
        //
        CodeBuffer = ScriptEngineParseWrapper((char *)Command.c_str());

        if (CodeBuffer == NULL)
        {
            //
            // return to show that this item contains an script
            //
            return;
        }

        //
        // Print symbols (test)
        //
        // PrintSymbolBufferWrapper(CodeBuffer);

        //
        // Set the buffer and length
        //
        BufferAddress = ScriptEngineWrapperGetHead(CodeBuffer);
        BufferLength  = ScriptEngineWrapperGetSize(CodeBuffer);
        Pointer       = ScriptEngineWrapperGetPointer(CodeBuffer);

        //
        // Send it to the remote debuggee
        //
        KdSendScriptPacketToDebuggee(BufferAddress, BufferLength, Pointer, TRUE);

        //
        // Remove the buffer of script engine interpreted code
        //
        ScriptEngineWrapperRemoveSymbolBuffer(CodeBuffer);
    }
    else
    {
        //
        // The user is either in VMI-mode or not connected to debuggee,
        // in this state, we will support .formats for constant values
        // not register, or expression
        //
        if (SplittedCommand.size() != 2)
        {
            ShowMessages("err, only one constant value is allowed when you're not connected to a debuggee\n\n");
            CommandFormatsHelp();
            return;
        }

        if (!ConvertStringToUInt64(SplittedCommand.at(1), &ConstantValue))
        {
            //
            // Unkonwn parameter
            //
            ShowMessages("err, unknown parameter '%s'\nwhile you're not connected to any debuggee (Debugger Mode), "
                         "you can only use a constant value in the '.formats' and you're not allowed to use registers "
                         "or expressions\n\n",
                         SplittedCommand.at(1).c_str());
            return;
        }

        //
        // Show formats results for a constant
        //
        CommandFormatsShowResults(ConstantValue);
    }
}
