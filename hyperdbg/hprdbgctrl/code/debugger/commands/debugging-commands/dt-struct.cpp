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
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN                  g_IsSerialConnectedToRemoteDebuggee;
extern ACTIVE_DEBUGGING_PROCESS g_ActiveProcessDebuggingState;

/**
 * @brief help of dt command
 *
 * @return VOID
 */
VOID
CommandDtHelp()
{
    ShowMessages("dt !dt : displays information about a local variable, global "
                 "variable or data type.\n\n");

    ShowMessages("If you want to read physical memory then add '!' at the "
                 "start of the command\n\n");

    ShowMessages("syntax : \tdt [Module!SymbolName (string)] [AddressExpression (string)] "
                 "[pid ProcessId (hex)] [padding Padding (yesno)] [offset Offset (yesno)] "
                 "[bitfield Bitfield (yesno)] [native Native (yesno)] [decl Declaration (yesno)] "
                 "[def Definitions (yesno)] [func Functions (yesno)] [pragma Pragma (yesno)] "
                 "[prefix Prefix (string)] [suffix Suffix (string)] [inline Expantion (string)] "
                 "[output FileName (string)]\n\n");
    ShowMessages("syntax : \t!dt [Module!SymbolName (string)] [AddressExpression (string)] "
                 "[padding Padding (yesno)] [offset Offset (yesno)] [bitfield Bitfield (yesno)] "
                 "[native Native (yesno)] [decl Declaration (yesno)] [def Definitions (yesno)] "
                 "[func Functions (yesno)] [pragma Pragma (yesno)] [prefix Prefix (string)] "
                 "[suffix Suffix (string)] [inline Expantion (string)] [output FileName (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : dt nt!_EPROCESS\n");
    ShowMessages("\t\te.g : dt nt!_EPROCESS fffff8077356f010\n");
    ShowMessages("\t\te.g : dt nt!_EPROCESS $proc\n");
    ShowMessages("\t\te.g : dt nt!_KPROCESS @rax+@rbx+c0\n");
    ShowMessages("\t\te.g : !dt nt!_EPROCESS 1f0300\n");
    ShowMessages("\t\te.g : dt nt!_MY_STRUCT 7ff00040 pid 1420\n");
    ShowMessages("\t\te.g : dt nt!_EPROCESS $proc inline all\n");
    ShowMessages("\t\te.g : dt nt!_EPROCESS fffff8077356f010 inline no\n");
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

    ShowMessages("syntax : \struct [Module!SymbolName (string)] [offset Offset (yesno)] [bitfield Bitfield (yesno)] "
                 "[native Native (yesno)] [decl Declaration (yesno)] [def Definitions (yesno)] "
                 "[func Functions (yesno)] [pragma Pragma (yesno)] [prefix Prefix (string)] "
                 "[suffix Suffix (string)] [inline Expantion (string)] [output FileName (string)]\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : struct nt!_EPROCESS\n");
    ShowMessages("\t\te.g : struct nt!*\n");
    ShowMessages("\t\te.g : struct nt!* output ntheader.h\n");
    ShowMessages("\t\te.g : struct nt!* func yes output ntheader.h\n");
    ShowMessages("\t\te.g : struct nt!* func yes output ntheader.h\n");
}

/**
 * @brief Convert HyperDbg arguments for dt and struct commands to pdbex arguments 
 * @details This is because we don't wanna modify the internal structure of pdbex
 * so let's pdbex parse its standard commands
 * 
 * @param ExtraArgs
 * @param PdbexArgs
 * @param ProcessId
 * 
 * @return BOOLEAN
 */
BOOLEAN
CommandDtAndStructConvertHyperDbgArgsToPdbex(vector<string> ExtraArgs,
                                             std::string &  PdbexArgs,
                                             UINT32 *       ProcessId)
{
    UINT32  TargetProcessId     = NULL;
    BOOLEAN NextItemIsYesNo     = FALSE;
    BOOLEAN NextItemIsString    = FALSE;
    BOOLEAN NextItemIsInline    = FALSE;
    BOOLEAN NextItemIsFileName  = FALSE;
    BOOLEAN NextItemIsProcessId = FALSE;

    //
    // Clear the args
    //
    PdbexArgs = "";

    //
    // Traverse through the extra arguments
    //
    for (auto Item : ExtraArgs)
    {
        //
        // Check for output file name
        //
        if (NextItemIsFileName)
        {
            PdbexArgs += Item + " ";

            NextItemIsFileName = FALSE;
            continue;
        }

        //
        // Check for process id
        //
        if (NextItemIsProcessId)
        {
            if (!ConvertStringToUInt32(Item, &TargetProcessId))
            {
                ShowMessages("err, you should enter a valid process id\n\n");
                return FALSE;
            }

            NextItemIsProcessId = FALSE;
            continue;
        }

        //
        // Check if we expect yes/no answers
        //
        if (NextItemIsYesNo)
        {
            if (!Item.compare("yes"))
            {
                PdbexArgs += " ";
            }
            else if (!Item.compare("no"))
            {
                PdbexArgs += "- ";
            }
            else
            {
                //
                // Yes/no expected but didn't see it
                //
                ShowMessages("err, please insert 'yes' or 'no' as the argument\n\n");
                return FALSE;
            }

            NextItemIsYesNo = FALSE;
            continue;
        }

        //
        // Check if we expect inline param answers
        //
        if (NextItemIsInline)
        {
            if (!Item.compare("none"))
            {
                PdbexArgs += "n ";
            }
            else if (!Item.compare("all"))
            {
                PdbexArgs += "a ";
            }
            else if (!Item.compare("unnamed") || !Item.compare("unamed"))
            {
                PdbexArgs += "i ";
            }
            else
            {
                //
                // none/inline/all expected but didn't see it
                //
                ShowMessages("err, please insert 'none', 'inline', or 'all' as the argument\n\n");
                return FALSE;
            }

            NextItemIsInline = FALSE;
            continue;
        }

        //
        // Check if we expect string answers
        //
        if (NextItemIsString)
        {
            PdbexArgs += Item + " ";

            NextItemIsString = FALSE;
            continue;
        }

        //
        // Check for args
        //
        if (!Item.compare("pid"))
        {
            NextItemIsProcessId = TRUE;
        }
        else if (!Item.compare("output"))
        {
            NextItemIsFileName = TRUE;
            PdbexArgs += "-o ";
        }
        else if (!Item.compare("inline"))
        {
            NextItemIsInline = TRUE;
            PdbexArgs += "-e ";
        }
        else if (!Item.compare("prefix"))
        {
            NextItemIsString = TRUE;
            PdbexArgs += "-r ";
        }
        else if (!Item.compare("suffix"))
        {
            NextItemIsString = TRUE;
            PdbexArgs += "-g ";
        }
        else if (!Item.compare("padding"))
        {
            NextItemIsYesNo = TRUE;
            PdbexArgs += "-p";
        }
        else if (!Item.compare("offset") || !Item.compare("offsets"))
        {
            NextItemIsYesNo = TRUE;
            PdbexArgs += "-x";
        }
        else if (!Item.compare("bitfield") || !Item.compare("bitfields"))
        {
            NextItemIsYesNo = TRUE;
            PdbexArgs += "-b";
        }
        else if (!Item.compare("native"))
        {
            NextItemIsYesNo = TRUE;
            PdbexArgs += "-i";
        }
        else if (!Item.compare("decl"))
        {
            NextItemIsYesNo = TRUE;
            PdbexArgs += "-n";
        }
        else if (!Item.compare("def"))
        {
            NextItemIsYesNo = TRUE;
            PdbexArgs += "-l";
        }
        else if (!Item.compare("func"))
        {
            NextItemIsYesNo = TRUE;
            PdbexArgs += "-f";
        }
        else if (!Item.compare("pragma"))
        {
            NextItemIsYesNo = TRUE;
            PdbexArgs += "-z";
        }
        else
        {
            //
            // Unknown args
            //
            ShowMessages("err, unknown argument at '%s'\n\n", Item.c_str());
            return FALSE;
        }
    }

    //
    // Check if user enetered yes/no or string when expected or not
    //
    if (NextItemIsYesNo || NextItemIsString || NextItemIsInline || NextItemIsFileName || NextItemIsProcessId)
    {
        ShowMessages("err, incomplete argument\n\n");
        return FALSE;
    }

    //
    // Set the process id
    //
    *ProcessId = TargetProcessId;

    return TRUE;
}

/**
 * @brief Show data based on the symbol structure and data types
 *
 * @param TypeName
 * @param Address
 * @param IsStruct
 * @param BufferAddress
 * @param TargetPid
 * @param IsPhysicalAddress
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
    UINT32       TargetPid,
    BOOLEAN      IsPhysicalAddress,
    const char * AdditionalParameters)
{
    UINT64                      StructureSize       = 0;
    BOOLEAN                     ResultOfFindingSize = FALSE;
    DEBUGGER_DT_COMMAND_OPTIONS DtOptions           = {0};

    //
    // Check for pid
    //

    if (g_IsSerialConnectedToRemoteDebuggee && TargetPid != 0)
    {
        //
        // Check to prevent using process id in dt command
        //
        ShowMessages("err, you cannot specify 'pid' in the debugger mode\n");
        return FALSE;
    }
    else if (TargetPid == NULL)
    {
        //
        // By default if the user-debugger is active, we use these commands
        // on the memory layout of the debuggee process
        //
        if (g_ActiveProcessDebuggingState.IsActive)
        {
            TargetPid = g_ActiveProcessDebuggingState.ProcessId;
        }
        else
        {
            //
            // Use the current process for the pid
            //
            TargetPid = GetCurrentProcessId();
        }
    }

    //
    // Set the options
    //
    DtOptions.TypeName             = TypeName;
    DtOptions.Address              = Address;
    DtOptions.IsStruct             = IsStruct;
    DtOptions.BufferAddress        = NULL; // we didn't read it yet
    DtOptions.TargetPid            = TargetPid;
    DtOptions.AdditionalParameters = AdditionalParameters;

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
        // Set the type (structure) size
        //
        DtOptions.SizeOfTypeName = StructureSize;

        //
        // Read the memory
        //
        HyperDbgReadMemoryAndDisassemble(DEBUGGER_SHOW_COMMAND_DT,
                                         Address,
                                         IsPhysicalAddress ? DEBUGGER_READ_PHYSICAL_ADDRESS : DEBUGGER_READ_VIRTUAL_ADDRESS,
                                         READ_FROM_KERNEL,
                                         TargetPid,
                                         StructureSize,
                                         &DtOptions);
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
    std::string PdbexArgs                          = "";
    BOOLEAN     IsStruct                           = FALSE;
    UINT64      TargetAddress                      = NULL;
    PVOID       BufferAddressRetrievedFromDebuggee = NULL;
    UINT32      TargetPid                          = NULL;
    BOOLEAN     IsPhysicalAddress                  = FALSE;

    //
    // Check if command is 'struct' or not
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

    //
    // Check if command is '!dt' for physical address or not
    //
    if (!SplittedCommand.at(0).compare("!dt"))
    {
        IsPhysicalAddress = TRUE;
    }
    else
    {
        IsPhysicalAddress = FALSE;
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
                                            TargetPid,
                                            IsPhysicalAddress,
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
        if (IsStruct || !SymbolConvertNameOrExprToAddress(TempSplittedCommand.at(0).c_str(),
                                                          &TargetAddress))
        {
            //
            // No it's not, we'll get the first argument as the structure (type) name
            // And we have to check whether the second argument is a buffer address or not
            //
            if (IsStruct || !SymbolConvertNameOrExprToAddress(TempSplittedCommand.at(1).c_str(),
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
                // Convert to pdbex args
                //
                if (!CommandDtAndStructConvertHyperDbgArgsToPdbex(TempSplittedCommand, PdbexArgs, &TargetPid))
                {
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
                // Call the wrapper of pdbex
                //
                CommandDtShowDataBasedOnSymbolTypes(TempTypeNameHolder.c_str(),
                                                    TargetAddress,
                                                    IsStruct,
                                                    BufferAddressRetrievedFromDebuggee,
                                                    TargetPid,
                                                    IsPhysicalAddress,
                                                    PdbexArgs.c_str());
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
                                                        TargetPid,
                                                        IsPhysicalAddress,
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
                    // Convert to pdbex args
                    //
                    if (!CommandDtAndStructConvertHyperDbgArgsToPdbex(TempSplittedCommand, PdbexArgs, &TargetPid))
                    {
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
                    // Call the wrapper of pdbex
                    //
                    CommandDtShowDataBasedOnSymbolTypes(TempTypeNameHolder.c_str(),
                                                        TargetAddress,
                                                        IsStruct,
                                                        BufferAddressRetrievedFromDebuggee,
                                                        TargetPid,
                                                        IsPhysicalAddress,
                                                        PdbexArgs.c_str());
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
                                                    TargetPid,
                                                    IsPhysicalAddress,
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
                // Convert to pdbex args
                //
                if (!CommandDtAndStructConvertHyperDbgArgsToPdbex(TempSplittedCommand, PdbexArgs, &TargetPid))
                {
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
                // Call the wrapper of pdbex
                //
                CommandDtShowDataBasedOnSymbolTypes(TempTypeNameHolder.c_str(),
                                                    TargetAddress,
                                                    IsStruct,
                                                    BufferAddressRetrievedFromDebuggee,
                                                    TargetPid,
                                                    IsPhysicalAddress,
                                                    PdbexArgs.c_str());
            }
        }
    }
}
