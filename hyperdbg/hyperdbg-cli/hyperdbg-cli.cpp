/**
 * @file hyperdbg-cli.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief HyperDbg Cli Interface
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "Configuration.h"
#include "Definition.h"
#include <Windows.h>
#include <conio.h>
#include <iostream>

#pragma comment(lib, "HPRDBGCTRL.lib")

/* Header file of HPRDBGCTRL */
/* Imports */
extern "C" {
__declspec(dllimport) int __cdecl HyperdbgLoad();
__declspec(dllimport) int __cdecl HyperdbgUnload();
__declspec(dllimport) int __cdecl HyperdbgInstallDriver();
__declspec(dllimport) int __cdecl HyperdbgUninstallDriver();
__declspec(dllimport) void __stdcall HyperdbgSetTextMessageCallback(
    Callback handler);
}

void PrintAppearance() {

  printf("\n"

         "    _   _                             _                  _____       "
         "               ____                 _       _     \n"
         "   | | | |_   _ _ __   ___ _ ____   _(_)___  ___  _ __  |  ___| __ "
         "___  _ __ ___   / ___|  ___ _ __ __ _| |_ ___| |__  \n"
         "   | |_| | | | | '_ \\ / _ \\ '__\\ \\ / / / __|/ _ \\| '__| | |_ | "
         "'__/ _ \\| '_ ` _ \\  \\___ \\ / __| '__/ _` | __/ __| '_ \\ \n"
         "   |  _  | |_| | |_) |  __/ |   \\ V /| \\__ \\ (_) | |    |  _|| | "
         "| (_) | | | | | |  ___) | (__| | | (_| | || (__| | | |\n"
         "   |_| |_|\\__, | .__/ \\___|_|    \\_/ |_|___/\\___/|_|    |_|  |_| "
         " \\___/|_| |_| |_| |____/ \\___|_|  \\__,_|\\__\\___|_| |_|\n"
         "          |___/|_|                                                   "
         "                                                  \n"
         "\n\n");
}

int main() {
  //
  // Print HyperDbg Message
  //
  PrintAppearance();

  //
  // Installing Driver
  //
  if (HyperdbgInstallDriver()) {
    printf("Failed to install driver\n");
    printf("Press any key to exit ...");
    _getch();
    return 1;
  }

  if (HyperdbgLoad()) {
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
  if (HyperdbgUninstallDriver()) {
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
