/**
 * @file dt.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief dt command
 * @details
 * @version 0.1
 * @date 2021-12-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "..\hprdbgctrl\pch.h"

/**
 * @brief help of dt command
 *
 * @return VOID
 */
VOID
CommandDtHelp()
{
    ShowMessages(".formats : Show a value or register in different formats.\n\n");
    ShowMessages("syntax : \t.formats [Expression (string)]\n");

    ShowMessages("\t\te.g : .formats nt!ExAllocatePoolWithTag\n");
    ShowMessages("\t\te.g : .formats nt!Kd_DEFAULT_Mask\n");
    ShowMessages("\t\te.g : .formats nt!Kd_DEFAULT_Mask+5\n");
    ShowMessages("\t\te.g : .formats 55\n");
    ShowMessages("\t\te.g : .formats @rax\n");
    ShowMessages("\t\te.g : .formats @rbx+@rcx\n");
    ShowMessages("\t\te.g : .formats $pid\n");
}

/**
 * @brief dt command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandDt(vector<string> SplittedCommand, string Command)
{
    string              TempTypeNameHolder;
    UINT64              TargetAddress                      = NULL;
    PVOID               BufferAddressRetrievedFromDebuggee = NULL;
    std::vector<char *> TempCharArrayVector;

    if (SplittedCommand.size() == 1)
    {
        ShowMessages("incorrect use of 'dt'\n\n");
        CommandDtHelp();
        return;
    }

    //
    // Configure default parameters
    //
    const int    SizeOfDefaultParams                         = 4;
    const char * SimpleStructShowParams[SizeOfDefaultParams] = {0};

    SimpleStructShowParams[0] = "-j-";
    SimpleStructShowParams[1] = "-k-";
    SimpleStructShowParams[2] = "-e";
    SimpleStructShowParams[3] = "n";

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove dt from it
    //
    Command.erase(0, 2);

    //
    // Trim it again
    //
    Trim(Command);

    //
    // Check for the first and second arguments
    //
    vector<string> TempSplittedCommand {Split(Command, ' ')};

    //
    // If the size is zero, then it's only a type name
    //
    if (TempSplittedCommand.size() == 1)
    {
        //
        // Call the dt parser wrapper, it's only a structure (type) name
        // Call it with default configuration
        //
        ScriptEngineShowDataBasedOnSymbolTypesWrapper(TempSplittedCommand.at(0).c_str(),
                                                      NULL,
                                                      NULL,
                                                      (char *)SimpleStructShowParams,
                                                      SizeOfDefaultParams);
    }
    else
    {
        //
        // When we're here, it means we have size() >= 2, so we have to check
        // the first and the second method to see which one is the type and
        // which one is the symbol
        //

        //
        // Check if the first parameter is an address or valid expression
        //
        if (!SymbolConvertNameOrExprToAddress(TempSplittedCommand.at(0).c_str(),
                                              &TargetAddress))
        {
            //
            // No it's not, we'll get the first argument as the structure (type) name
            // And we have to check whether the second argument is a buffer address or not
            //
            if (!SymbolConvertNameOrExprToAddress(TempSplittedCommand.at(1).c_str(),
                                                  &TargetAddress))
            {
                //
                // The second argument is also not buffer address
                // probably the user entered a structure (type) name along with some params
                //
            }
            else
            {
                //
                // The second argument is a buffer address
                // The user entered a structure (type) name along with buffer address
                //
            }
        }
        else
        {
            //
            // The first argument is a buffer address, so we get the first argument as
            // a buffer address and the second argument as the the structure (type) name
            //
            if (TempSplittedCommand.size() == 2)
            {
                //
                // There is not parameters, only a buffer address and then a symbol name
                // Call it with default configuration
                //
                ScriptEngineShowDataBasedOnSymbolTypesWrapper(TempSplittedCommand.at(1).c_str(),
                                                              TargetAddress,
                                                              BufferAddressRetrievedFromDebuggee,
                                                              (char *)SimpleStructShowParams,
                                                              SizeOfDefaultParams);
            }
            else
            {
                //
                // Other than the first argument which is a buffer address, and the second
                // argument which is structure (type) name, there are other parameters, so
                // we WON'T call it with default parameters
                //
                TempTypeNameHolder = TempSplittedCommand.at(1);

                //
                // Remove the first, and the second arguments
                //
                TempSplittedCommand.erase(TempSplittedCommand.begin());
                TempSplittedCommand.erase(TempSplittedCommand.begin());

                //
                // Convert it to char* array (argv)
                //
                std::transform(TempSplittedCommand.begin(),
                               TempSplittedCommand.end(),
                               std::back_inserter(TempCharArrayVector),
                               ConvertStringVectorToCharPointerArray);

                //
                // Call the wrapper of pdbex
                //
                ScriptEngineShowDataBasedOnSymbolTypesWrapper(TempTypeNameHolder.c_str(),
                                                              TargetAddress,
                                                              BufferAddressRetrievedFromDebuggee,
                                                              TempCharArrayVector[0],
                                                              TempCharArrayVector.size());
            }
        }
    }

    //
    // Remove the temporary vector array if anything is allocated
    //
    for (size_t i = 0; i < TempCharArrayVector.size(); i++)
    {
        delete[] TempCharArrayVector[i];
    }
}
