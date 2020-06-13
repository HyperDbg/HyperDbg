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

BOOLEAN
ExtensionCommandPte(PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PteDetails);

VOID
ExtensionCommandChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask);
VOID
ExtensionCommandChangeAllMsrBitmapWriteAllCores(UINT64 BitmapMask);
VOID
ExtensionCommandEnableRdtscExitingAllCores();
VOID
ExtensionCommandEnableRdpmcExitingAllCores();
VOID
ExtensionCommandSetExceptionBitmapAllCores(UINT64 ExceptionIndex);
VOID
ExtensionCommandEnableMovDebugRegistersExiyingAllCores();
VOID
ExtensionCommandSetExternalInterruptExitingAllCores();
VOID
ExtensionCommandIoBitmapChangeAllCores(UINT64 Port);
