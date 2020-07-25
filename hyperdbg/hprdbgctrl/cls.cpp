/**
 * @file cls.c
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief .cls, cls, clear commands implementations
 * @details
 * @version 0.1
 * @date 2020-05-27
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

VOID CommandClearScreenHelp() {
  ShowMessages(".cls : clears the screen.\n\n");
  ShowMessages("syntax : \t.cls\n");
}
VOID CommandClearScreen() { system("cls"); }
