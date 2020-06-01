/**
 * @file ExtensionCommands.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Headers of Debugger Commands (Extensions)
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */

#include <ntddk.h>

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

VOID
ExtensionCommandEnableEferOnAllProcessors();

VOID
ExtensionCommandDisableEferOnAllProcessors();

VOID
ExtensionCommandDisableMsrBitmapAllCores();

BOOLEAN
ExtensionCommandPte(PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PteDetails);
