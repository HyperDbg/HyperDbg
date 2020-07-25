/**
 * @file breakcontrol.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief break control is the handler for CTRL+C and CTRL+BREAK Signals
 * @details
 * @version 0.1
 * @date 2020-07-24
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

extern BOOLEAN g_BreakPrintingOutput;

BOOL WINAPI BreakController(DWORD CtrlType) {
  switch (CtrlType) {
    // Handle the CTRL-C signal.
  case CTRL_C_EVENT:
    //
    // Sleep because the other thread that shows must be stopped
    //
    Sleep(500);
    ShowMessages("\npause\npausing debugger...\n");
    g_BreakPrintingOutput = TRUE;
    return TRUE;

    // CTRL-CLOSE: confirm that the user wants to exit.
  case CTRL_CLOSE_EVENT:
    return TRUE;

    // Pass other signals to the next handler.
  case CTRL_BREAK_EVENT:
    //
    // Sleep because the other thread that shows must be stopped
    //
    Sleep(500);
    ShowMessages("\npause\npausing debugger...\n");
    g_BreakPrintingOutput = TRUE;
    return TRUE;

  case CTRL_LOGOFF_EVENT:
    return FALSE;

  case CTRL_SHUTDOWN_EVENT:
    return FALSE;

  default:
    //
    // Return TRUE if handled this message, further handler functions won't be
    // called.
    // Return FALSE to pass this message to further handlers until default
    // handler calls ExitProcess().
    //

    return FALSE;
    return FALSE;
  }
}
