/**
 * @file dt-struct.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief dt and struct command
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
    ShowMessages("dt : displays information about a local variable, global "
                 "variable or data type.\n\n");
    ShowMessages("syntax : \tdt [Module!SymbolName (string)] [Expression (string)]\n");

    ShowMessages("\t\te.g : dt nt!_EPROCESS\n");
    ShowMessages("\t\te.g : dt nt!_EPROCESS fffff8077356f010\n");
    ShowMessages("\t\te.g : dt nt!_EPROCESS @rbx+@rcx\n");
}

/**
 * @brief help of struct command
 *
 * @return VOID
 */
VOID
CommandStructHelp()
{
    ShowMessages("struct : displays a data type, enum, or structure derived from PDB symbols.\n\n");
    ShowMessages("syntax : \struct [Module!SymbolName (string)]\n");

    ShowMessages("\t\te.g : struct nt!_EPROCESS\n");
}

/**
 * @brief Show data based on the symbol structure and data types
 *
 * @param TypeName
 * @param Address
 * @param IsStruct
 * @param BufferAddress
 * @param AdditionalParameters
 * 
 * @return BOOLEAN
 */
BOOLEAN
CommandDtShowDataBasedOnSymbolTypes(
    const char * TypeName,
    UINT64       Address,
    BOOLEAN      IsStruct,
    PVOID        BufferAddress,
    const char * AdditionalParameters)
{
    UINT64  StructureSize       = 0;
    BOOLEAN ResultOfFindingSize = FALSE;

    if (Address != NULL)
    {
        //
        // *** We need to read the memory here ***
        //

        //
        // Get the field size
        //
        ResultOfFindingSize = ScriptEngineGetDataTypeSizeWrapper((char *)TypeName, &StructureSize);

        //
        // Check if size is found
        //
        if (!ResultOfFindingSize || StructureSize == 0)
        {
            //
            // Field not found or size is invalid
            //
            ShowMessages("err, couldn't resolve error at '%s'\n", TypeName);
            return FALSE;
        }

        //
        // *** Read the memory ***
        //
    }
    else
    {
        //
        // It's a simple structure without an address
        // Call the pdbex wrapper
        //
        return ScriptEngineShowDataBasedOnSymbolTypesWrapper(TypeName, Address, IsStruct, BufferAddress, AdditionalParameters);
    }
}

/**
 * @brief dt and struct command handler
 *
 * @param SplittedCommand
 * @param Command
 * @return VOID
 */
VOID
CommandDtAndStruct(vector<string> SplittedCommand, string Command)
{
    std::string TempTypeNameHolder;
    std::string TempExtraParamHolder;
    BOOLEAN     IsStruct                           = FALSE;
    UINT64      TargetAddress                      = NULL;
    PVOID       BufferAddressRetrievedFromDebuggee = NULL;

    //
    // Check if command is 'dt' or 'struct'
    //
    if (!SplittedCommand.at(0).compare("struct") ||
        !SplittedCommand.at(0).compare("structure"))
    {
        IsStruct = TRUE;
    }
    else
    {
        IsStruct = FALSE;
    }

    if (SplittedCommand.size() == 1)
    {
        ShowMessages("incorrect use of '%s'\n\n", SplittedCommand.at(0).c_str());

        if (IsStruct)
        {
            CommandStructHelp();
        }
        else
        {
            CommandDtHelp();
        }

        return;
    }

    //
    // Trim the command
    //
    Trim(Command);

    //
    // Remove dt, struct, or structure from it
    //
    Command.erase(0, SplittedCommand.at(0).size());

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
        CommandDtShowDataBasedOnSymbolTypes(TempSplittedCommand.at(0).c_str(),
                                            NULL,
                                            IsStruct,
                                            NULL,
                                            PDBEX_DEFAULT_CONFIGURATION);
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
                TempTypeNameHolder = TempSplittedCommand.at(0);

                //
                // Remove the first argument
                //
                TempSplittedCommand.erase(TempSplittedCommand.begin());

                //
                // Concat extra parameters
                //
                for (auto item : TempSplittedCommand)
                {
                    TempExtraParamHolder = TempExtraParamHolder + " " + item;
                }

                //
                // removes first space character
                //
                TempExtraParamHolder.erase(0, 1);

                //
                // Call the wrapper of pdbex
                //
                CommandDtShowDataBasedOnSymbolTypes(TempTypeNameHolder.c_str(),
                                                    TargetAddress,
                                                    IsStruct,
                                                    BufferAddressRetrievedFromDebuggee,
                                                    TempExtraParamHolder.c_str());
            }
            else
            {
                //
                // The second argument is a buffer address
                // The user entered a structure (type) name along with buffer address
                //
                if (TempSplittedCommand.size() == 2)
                {
                    //
                    // There is not parameters, only a symbol name and then a buffer address
                    // Call it with default configuration
                    //
                    CommandDtShowDataBasedOnSymbolTypes(TempSplittedCommand.at(0).c_str(),
                                                        TargetAddress,
                                                        IsStruct,
                                                        BufferAddressRetrievedFromDebuggee,
                                                        PDBEX_DEFAULT_CONFIGURATION);
                }
                else
                {
                    //
                    // Other than the first argument which is a structure (type) name, and
                    // the second argument which is buffer address, there are other parameters, so
                    // we WON'T call it with default parameters
                    //
                    TempTypeNameHolder = TempSplittedCommand.at(0);

                    //
                    // Remove the first, and the second arguments
                    //
                    TempSplittedCommand.erase(TempSplittedCommand.begin());
                    TempSplittedCommand.erase(TempSplittedCommand.begin());

                    //
                    // Concat extra parameters
                    //
                    for (auto item : TempSplittedCommand)
                    {
                        TempExtraParamHolder = TempExtraParamHolder + " " + item;
                    }

                    //
                    // removes first space character
                    //
                    TempExtraParamHolder.erase(0, 1);

                    //
                    // Call the wrapper of pdbex
                    //
                    CommandDtShowDataBasedOnSymbolTypes(TempTypeNameHolder.c_str(),
                                                        TargetAddress,
                                                        IsStruct,
                                                        BufferAddressRetrievedFromDebuggee,
                                                        TempExtraParamHolder.c_str());
                }
            }
        }
        else
        {
            //
            // The first argument is a buffer address, so we get the first argument as
            // a buffer address and the second argument as the structure (type) name
            //
            if (TempSplittedCommand.size() == 2)
            {
                //
                // There is not parameters, only a buffer address and then a symbol name
                // Call it with default configuration
                //
                CommandDtShowDataBasedOnSymbolTypes(TempSplittedCommand.at(1).c_str(),
                                                    TargetAddress,
                                                    IsStruct,
                                                    BufferAddressRetrievedFromDebuggee,
                                                    PDBEX_DEFAULT_CONFIGURATION);
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
                // Concat extra parameters
                //
                for (auto item : TempSplittedCommand)
                {
                    TempExtraParamHolder = TempExtraParamHolder + " " + item;
                }

                //
                // removes first space character
                //
                TempExtraParamHolder.erase(0, 1);

                //
                // Call the wrapper of pdbex
                //
                CommandDtShowDataBasedOnSymbolTypes(TempTypeNameHolder.c_str(),
                                                    TargetAddress,
                                                    IsStruct,
                                                    BufferAddressRetrievedFromDebuggee,
                                                    TempExtraParamHolder.c_str());
            }
        }
    }
}
