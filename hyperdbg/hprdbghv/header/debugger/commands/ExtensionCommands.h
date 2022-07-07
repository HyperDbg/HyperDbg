/**
 * @file ExtensionCommands.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers of Debugger Commands (Extensions)
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				     Functions		      		//
//////////////////////////////////////////////////

BOOLEAN
ExtensionCommandPte(PDEBUGGER_READ_PAGE_TABLE_ENTRIES_DETAILS PteDetails, BOOLEAN IsOperatingInVmxRoot);

VOID
ExtensionCommandVa2paAndPa2va(PDEBUGGER_VA2PA_AND_PA2VA_COMMANDS AddressDetails, BOOLEAN OperateOnVmxRoot);

VOID
ExtensionCommandChangeAllMsrBitmapReadAllCores(UINT64 BitmapMask);

VOID
ExtensionCommandResetChangeAllMsrBitmapReadAllCores();

VOID
ExtensionCommandChangeAllMsrBitmapWriteAllCores(UINT64 BitmapMask);

VOID
ExtensionCommandResetAllMsrBitmapWriteAllCores();

VOID
ExtensionCommandEnableRdtscExitingAllCores();

VOID
ExtensionCommandDisableRdtscExitingAllCores();

VOID
ExtensionCommandDisableRdtscExitingForClearingEventsAllCores();

VOID
ExtensionCommandDisableMov2DebugRegsExitingForClearingEventsAllCores();

VOID
ExtensionCommandEnableRdpmcExitingAllCores();

VOID
ExtensionCommandDisableRdpmcExitingAllCores();

VOID
ExtensionCommandSetExceptionBitmapAllCores(UINT64 ExceptionIndex);

VOID
ExtensionCommandUnsetExceptionBitmapAllCores(UINT64 ExceptionIndex);

VOID
ExtensionCommandResetExceptionBitmapAllCores();

VOID
ExtensionCommandEnableMovDebugRegistersExitingAllCores();

VOID
ExtensionCommandDisableMovDebugRegistersExitingAllCores();

VOID
ExtensionCommandSetExternalInterruptExitingAllCores();

VOID
ExtensionCommandUnsetExternalInterruptExitingOnlyOnClearingInterruptEventsAllCores();

VOID
ExtensionCommandIoBitmapChangeAllCores(UINT64 Port);

VOID
ExtensionCommandIoBitmapResetAllCores();

VOID
ExtensionCommandEnableMovControlRegisterExitingAllCores(PDEBUGGER_EVENT Event);

VOID
ExtensionCommandDisableMovToControlRegisterExitingAllCores(PDEBUGGER_EVENT Event);

VOID
ExtensionCommandDisableMov2ControlRegsExitingForClearingEventsAllCores(PDEBUGGER_EVENT Event);