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

    ShowMessages("Evaluate expression:\n");
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

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // Send over serial
        //

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
        // error
        //
        ShowMessages("err, you're not connected to any debuggee\n");
    }
}
