/**
 * @file interpreter.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief The hyperdbg command interpreter and driver connector
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

#include "pch.h"
#include <string>
#include <vector>
#include <iterator>
#include <algorithm>

using namespace std;


#define clrscr() printf("\e[1;1H\e[2J")

// Exports
extern "C"
{
	__declspec (dllexport) int __cdecl  HyperdbgInterpreter(const char* Command);
}


const vector<string> Split(const string& s, const char& c)
{
	string buff{ "" };
	vector<string> v;

	for (auto n : s)
	{
		if (n != c) buff += n; else
			if (n == c && buff != "") { v.push_back(buff); buff = ""; }
	}
	if (buff != "") v.push_back(buff);

	return v;
}

void CommandClearScreen() {
	clrscr();
}


/**
 * @brief Interpret commands
 *
 * @param Command The text of command
 * @return int returns return zero if it was successful or non-zero if there was error
 */
int _cdecl  HyperdbgInterpreter(const char* Command) {

	string CommandString(Command);

	//
	// Convert to lower case 
	//
	transform(CommandString.begin(), CommandString.end(), CommandString.begin(),
		[](unsigned char c) { return std::tolower(c); });

	vector<string> SplittedCommand{ Split(CommandString, ' ') };

	//
	// Check if user entered an empty imput
	//
	if (SplittedCommand.empty())
	{
		printf("\n");
		return 0;
	}

	string FirstCommand = SplittedCommand.front();


	if (!FirstCommand.compare("clear") || !FirstCommand.compare("cls") || !FirstCommand.compare(".cls"))
	{
		system("cls");
	}
	else 
	{
		printf("Couldn't resolve error at '%s'", FirstCommand.c_str());
		printf("\n\n");

	}



	return 0;
}