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
extern "C"
{
	__declspec (dllimport) int __cdecl  HyperdbgLoad();
	__declspec (dllimport) int __cdecl  HyperdbgUnload();
	__declspec (dllimport) int __cdecl  HyperdbgInstallDriver();
	__declspec (dllimport) int __cdecl  HyperdbgUninstallDriver();
	__declspec (dllimport) int __cdecl  HyperdbgInterpreter(const char* Command);
	__declspec (dllimport) void __stdcall HyperdbgSetTextMessageCallback(Callback handler);

}


/**
 * @brief CLI main function
 * 
 * @return int 
 */
int main()
{
	//
	// Put to ease the test, it will be removed
	//
	if (HyperdbgInstallDriver()) {
		return 1;
	}

	if (HyperdbgLoad()) {
		return 1;
	}

	// ---------------------------------------------------------

	
	bool ExitFromDebugger = false;

	printf("HyperDbg Debugger [core version: v%s]\n",Version);
	printf("Please visit https://docs.hyperdbg.com for more information...\n");
	printf("HyperDbg is released under the GNU Public License v3 (GPLv3).\n\n");

	while (!ExitFromDebugger) 
	{
		printf("HyperDbg >");

		string command;
		getline(cin, command);
		int CommandExecutionResult = HyperdbgInterpreter(command.c_str());
		printf("\n");

		//
		//if the debugger encounters an exit state then the return will be 1
		//
		if (CommandExecutionResult == 1)
		{
			//
			// Exit from the debugger
			//
			ExitFromDebugger = true;
		}


	}
	return 0;
}

