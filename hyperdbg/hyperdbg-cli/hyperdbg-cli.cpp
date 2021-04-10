/**
 * @file hyperdbg-cli.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Main HyperDbg Cli source coede
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include <Windows.h>
#include <string>
#include <conio.h>
#include <iostream>
#include "Definition.h"
#include "Configuration.h"
#include "details.h"

#pragma comment(lib, "HPRDBGCTRL.lib")

using namespace std;

//
// Header file of HPRDBGCTRL
// Imports
//
extern "C" {
__declspec(dllimport) int HyperdbgLoadVmm();
__declspec(dllimport) int HyperdbgUnload();
__declspec(dllimport) int HyperdbgInstallVmmDriver();
__declspec(dllimport) int HyperdbgUninstallDriver();
__declspec(dllimport) int HyperdbgStopDriver();
__declspec(dllimport) int HyperdbgInterpreter(const char * Command);
__declspec(dllexport) void HyperdbgShowSignature();
__declspec(dllimport) void HyperdbgSetTextMessageCallback(Callback handler);
}

/**
 * @brief CLI main function
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int
main(int argc, char * argv[])
{
    bool ExitFromDebugger = false;

    if (argc != 1)
    {
        //
        // User-passed arguments to the debugger
        //
        if (!strcmp(argv[1], "--script"))
        {
            char ScriptBuffer[MAX_PATH + 10] = {0};

            //
            // Executing the script
            //
            sprintf_s(ScriptBuffer, sizeof(ScriptBuffer), ".script %s", argv[2]);
            HyperdbgInterpreter(ScriptBuffer);
            printf("\n");
        }
        else
        {
            printf("invalid command line options passed to HyperDbg !\n");
            return 1;
        }
    }

    //
    // Put to ease the test, it will be removed
    //
    /* 
	if (HyperdbgInstallVmmDriver()) {
		return 1;
	}

	if (HyperdbgLoadVmm()) {
		return 1;
	}
	_getch();
	_getch();
	return 0;
	*/

    //
    // ---------------------------------------------------------
    //

    printf("HyperDbg Debugger [core version: v%s]\n", Version);
    printf("Please visit https://docs.hyperdbg.com for more information...\n");
    printf("HyperDbg is released under the GNU Public License v3 (GPLv3).\n\n");

    while (!ExitFromDebugger)
    {
        HyperdbgShowSignature();

        string command;
        getline(cin, command);

        if (cin.fail() || cin.eof())
        {
            cin.clear(); // reset cin state
        }

        int CommandExecutionResult = HyperdbgInterpreter(command.c_str());

        //
        // if the debugger encounters an exit state then the return will be 1
        //
        if (CommandExecutionResult == 1)
        {
            //
            // Exit from the debugger
            //
            ExitFromDebugger = true;
        }
        if (CommandExecutionResult != 2)
        {
            printf("\n");
        }
    }

    return 0;
}
