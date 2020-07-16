/**
 * @file logopen.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .logopen command
 * @details
 * @version 0.1
 * @date 2020-07-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

using namespace std;

extern BOOLEAN g_LogOpened;
extern ofstream g_LogOpenFile;

VOID CommandLogopenHelp() {
  ShowMessages(".logopen : save commands and results in a file.\n\n");
  ShowMessages("syntax : \.logopen [FilePath]\n");
}

VOID CommandLogopen(vector<string> SplittedCommand, string Command) {

  if (SplittedCommand.size() == 1) {
    ShowMessages("please specify a file.\n");
    CommandLogopenHelp();
    return;
  }

  if (g_LogOpened) {
    ShowMessages("log was opened previously, you have the close it first "
                 "(using .logclose).\n");
    return;
  }

  //
  // Trim the command
  //
  Trim(Command);

  //
  // Remove .logopen from it
  //
  Command.erase(0, 8);

  //
  // Trim it again
  //
  Trim(Command);

  //
  // Try to open it as file
  //
  g_LogOpenFile.open(Command.c_str());

  //
  // Check if it's okay
  //
  if (g_LogOpenFile.is_open()) {
    //
    // Start intercepting logs
    //
    g_LogOpened = TRUE;

    //
    // Enable save to the file (Message + time)
    //
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    ShowMessages("Save commands and results into file : %s (%d-%02d-%02d "
                 "%02d:%02d:%02d)\n",
                 Command.c_str(), tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                 tm.tm_hour, tm.tm_min, tm.tm_sec);

  } else {
    ShowMessages("enable to open file : %s\n", Command.c_str());
    return;
  }
}

VOID LogopenSaveToFile(const char *Text) { g_LogOpenFile << Text; }
