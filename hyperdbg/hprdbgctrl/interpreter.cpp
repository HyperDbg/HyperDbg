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

using namespace std;

// Exports
extern "C"
{
    __declspec (dllexport) int __cdecl  HyperdbgInterpreter(std::string Command);
}

 /**
  * @brief Interpret commands
  *
  * @param Command The text of command
  * @return int returns return zero if it was successful or non-zero if there was error
  */
int  HyperdbgInterpreter(std::string Command) {

    /*
    std::string delimiter = " ";

    size_t pos = 0;
    std::string token;
    while ((pos = Command.find(delimiter)) != std::string::npos) {
        token = Command.substr(0, pos);
        std::cout << token << std::endl;
        Command.erase(0, pos + delimiter.length());
    }
    std::cout << Command << std::endl;
    */

    return 0;
}