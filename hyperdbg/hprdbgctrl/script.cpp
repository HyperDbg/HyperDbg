/**
 * @file script.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief script command
 * @details
 * @version 0.1
 * @date 2020-07-03
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

VOID CommandScriptHelp() {
  ShowMessages(".script : run a HyperDbg script.\n\n");
  ShowMessages("syntax : \.script [FilePath]\n");
}

VOID CommandScript(vector<string> SplittedCommand, string Command) {

  std::string Line;
  BOOLEAN IsOpened = FALSE;
  string NewPath;
  int CommandExecutionResult = 0;

  if (SplittedCommand.size() == 1) {
    ShowMessages("please specify a file.\n");
    CommandScriptHelp();
    return;
  }

  //
  // Trim the command
  //
  Trim(Command);

  //
  // Remove .script from it
  //
  Command.erase(0, 7);

  //
  // Trim it again
  //
  Trim(Command);

  //
  // Parse the script file
  //
  ifstream File(Command);
  if (File.is_open()) {
    IsOpened = TRUE;
    while (std::getline(File, Line)) {
      ShowMessages("HyperDbg >%s\n", Line.c_str());
      CommandExecutionResult = HyperdbgInterpreter(Line.c_str());

      //
      // if the debugger encounters an exit state then the return will be 1
      //
      if (CommandExecutionResult == 1) {
        //
        // Exit from the debugger
        //
        exit(0);
      }
    }
    File.close();
  }

  if (!IsOpened) {
    ShowMessages("invalid file specified for .script command.\n");
  }
}
