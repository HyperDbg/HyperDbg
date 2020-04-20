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
#include <algorithm>
#include <iterator>
#include <string>
#include <vector>

using namespace std;

// Exports
extern "C" {
	__declspec(dllexport) int __cdecl HyperdbgInterpreter(const char* Command);
}

bool g_IsConnectedToDebugger = false;


const vector<string> Split(const string& s, const char& c) {
	string buff{ "" };
	vector<string> v;

	for (auto n : s) {
		if (n != c)
			buff += n;
		else if (n == c && buff != "") {
			v.push_back(buff);
			buff = "";
		}
	}
	if (buff != "")
		v.push_back(buff);

	return v;
}
// check if given string is a numeric string or not
bool IsNumber(const string& str)
{
	// std::find_first_not_of searches the string for the first character 
	// that does not match any of the characters specified in its arguments
	return !str.empty() &&
		(str.find_first_not_of("[0123456789]") == std::string::npos);
}

// Function to split string str using given delimiter
vector<string> SplitIp(const string& str, char delim)
{
	auto i = 0;
	vector<string> list;

	auto pos = str.find(delim);

	while (pos != string::npos)
	{
		list.push_back(str.substr(i, pos - i));
		i = ++pos;
		pos = str.find(delim, pos);
	}

	list.push_back(str.substr(i, str.length()));

	return list;
}

// Function to validate an IP address
bool ValidateIP(string ip)
{
	// split the string into tokens
	vector<string> list = SplitIp(ip, '.');

	// if token size is not equal to four
	if (list.size() != 4)
		return false;

	// validate each token
	for (string str : list)
	{
		// verify that string is number or not and the numbers 
		// are in the valid range
		if (!IsNumber(str) || stoi(str) > 255 || stoi(str) < 0)
			return false;
	}

	return true;
}



void CommandClearScreen() { system("cls"); }

void CommandHiddenHook(vector<string> SplittedCommand) {

	//
	// Note : You can use "bh" and "!hiddenhook" in a same way
	// * means optional
	// !hiddenhook
	//				 [Type:(all/process)]
	//				 [Address:(hex) - Address to hook]
	//				*[Pid:(hex number) - only if you choose 'process' as
	//the Type] 				 [Action:(break/code/log)]
	//				*[Condition:({ asm in hex })]
	//				*[Code:({asm in hex}) - only if you choose 'code' as
	//the Action]
	//				*[Log:(gp regs, pseudo-regs, static address,
	//dereference regs +- value) - only if you choose 'log' as the Action]
	//
	//
	//		e.g :
	//				-	bh all break fffff801deadbeef
	//						Description : Breaks to the debugger in all
	//accesses to the selected address
	//
	//				-	!hiddenhook process 8e34
	// fffff801deadbeef 						Description : Breaks to the debugger in
	// accesses from the selected process by pid to the selected address
	//

	for (auto Section : SplittedCommand) {
		printf("%s", Section.c_str());
		printf("\n");
	}
}

void CommandConnectHelp() {
	printf("connect : connects to a remote or local machine to start debugging.\n\n");
	printf("syntax : \tconnect [ip] [port]\n");
	printf("\t\te.g : connect 192.168.1.5 50000\n");
	printf("\t\te.g : connect local\n");
}
void CommandConnect(vector<string> SplittedCommand) {

	if (SplittedCommand.size() == 1)
	{
		//
		// Means that user entered just a connect so we have to 
		// ask to connect to what ?
		// 
		printf("incorrect use of 'connect'\n\n");
		CommandConnectHelp();
		return;
	}
	else if (SplittedCommand.at(1) == "help")
	{
		CommandConnectHelp();
		return;
	}
	else if (SplittedCommand.at(1) == "local" && SplittedCommand.size() == 2) {
		//
		// connect to local debugger
		//
		printf("local debug current system\n");
		g_IsConnectedToDebugger = true;

		return;
	}
	else if (SplittedCommand.size() == 3) {

		string ip = SplittedCommand.at(1);
		string port = SplittedCommand.at(2);

		//
		// means that probably wants to connect to a remote 
		// system, let's first check the if the parameters are
		// valid
		//
		if (!ValidateIP(ip))
		{
			printf("incorrect ip address\n");
			return;
		}
		if (!IsNumber(port) || stoi(port) > 65535 || stoi(port) < 0)
		{
			printf("incorrect port\n");
			return;
		}

		//
		// connect to remote debugger
		//
		printf("local debug remote system\n");
		g_IsConnectedToDebugger = true;


		return;
	}
	else {
		printf("incorrect use of 'connect'\n\n");
		CommandConnectHelp();
		return;
	}

}

void CommandDisconnect(vector<string> SplittedCommand) {
	for (auto Section : SplittedCommand) {
		printf("%s", Section.c_str());
		printf("\n");
	}
}


void CommandLoadHelp() {
	printf("load : installs the driver and load the kernel modules.\n\n");
	printf("syntax : \tload\n");
}
void CommandLoad(vector<string> SplittedCommand) {

	if (SplittedCommand.size() != 1)
	{
		printf("incorrect use of 'load'\n\n");
		CommandLoadHelp();
		return;
	}
	if (!g_IsConnectedToDebugger)
	{
		printf("You're not connected to any instance of HyperDbg, did you use 'connect'? \n");
		return;
	}
	printf("try to install driver...\n");
	if (HyperdbgInstallDriver())
	{
		printf("Failed to install driver\n");
		return;
	}

	printf("try to install load kernel modules...\n");
	if (HyperdbgLoad())
	{
		printf("Failed to load driver\n");
		return;
	}
}

void CommandUnload(vector<string> SplittedCommand) {
	for (auto Section : SplittedCommand) {
		printf("%s", Section.c_str());
		printf("\n");
	}
}

void CommandCpuHelp() {
	printf("cpu : collects a report from cpu features.\n\n");
	printf("syntax : \tcpu\n");
}
void CommandCpu(vector<string> SplittedCommand) {

	if (SplittedCommand.size() != 1)
	{
		printf("incorrect use of 'cpu'\n\n");
		CommandCpuHelp();
		return;
	}
	ReadCpuDetails();
}

/**
 * @brief Interpret commands
 *
 * @param Command The text of command
 * @return int returns return zero if it was successful or non-zero if there was
 * error
 */
int _cdecl HyperdbgInterpreter(const char* Command) {

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
	if (SplittedCommand.empty()) {
		printf("\n");
		return 0;
	}

	string FirstCommand = SplittedCommand.front();

	if (!FirstCommand.compare("clear") || !FirstCommand.compare("cls") ||
		!FirstCommand.compare(".cls")) {
		CommandClearScreen();
	}
	else if (!FirstCommand.compare("connect")) {
		CommandConnect(SplittedCommand);
	}
	else if (!FirstCommand.compare("disconnect")) {
		CommandDisconnect(SplittedCommand);
	}
	else if (!FirstCommand.compare("load")) {
		CommandLoad(SplittedCommand);
	}
	else if (!FirstCommand.compare("unload")) {
		CommandUnload(SplittedCommand);
	}
	else if (!FirstCommand.compare("cpu")) {
		CommandCpu(SplittedCommand);
	}
	else if (!FirstCommand.compare("!hiddenhook") ||
		!FirstCommand.compare("bh")) {
		CommandHiddenHook(SplittedCommand);
	}
	else {
		printf("Couldn't resolve error at '%s'", FirstCommand.c_str());
		printf("\n");
	}

	return 0;
}
