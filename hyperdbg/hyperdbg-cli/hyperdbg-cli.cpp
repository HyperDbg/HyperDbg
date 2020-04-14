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
	/*
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
	*/

	//
	// Installing Driver
	//
	if (HyperdbgInstallDriver())
	{
		printf("Failed to install driver\n");
		printf("Press any key to exit ...");
		_getch();
		return 1;
	}

	if (HyperdbgLoad())
	{
		printf("Failed to load driver\n");
		printf("Press any key to exit ...");
		_getch();
		return 1;
	}

	printf("Press any key to exit vmx ...");
	_getch();

	HyperdbgUnload();

	//
	// Installing Driver
	//
	if (HyperdbgUninstallDriver())
	{
		printf("Failed to uninstall driver\n");
		printf("Press any key to exit ...");
		_getch();
		return 1;
	}

	printf("Press any key to exit...");
	_getch();

	exit(0);

	return 0;
}

