/**
 * @file HyperDbgHyperTrace.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from hypertrace (tracing) module
 * @version 0.18
 * @date 2026-02-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifdef HYPERDBG_HYPERTRACE
#    define IMPORT_EXPORT_HYPERTRACE __declspec(dllexport)
#else
#    define IMPORT_EXPORT_HYPERTRACE __declspec(dllimport)
#endif

//////////////////////////////////////////////////
//            HyperTrace Functions 	    		//
//////////////////////////////////////////////////

//
// Initialize the hypertrace module with the provided callbacks
//
IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceInitCallback(HYPERTRACE_CALLBACKS * HypertraceCallbacks, BOOLEAN RunningOnHypervisorEnvironment);

//
// Uninitialize the HyperTrace module
//
IMPORT_EXPORT_HYPERTRACE VOID
HyperTraceUnInit();

//////////////////////////////////////////////////
//                LBR Functions 	    		//
//////////////////////////////////////////////////

IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceLbrSave(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest);

IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceLbrPrint(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest);

IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceLbrCheck();

IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceLbrFlush(HYPERTRACE_LBR_OPERATION_PACKETS * HyperTraceOperationRequest);

IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceLbrQueryStateOfLbrSaveAndLoadVmExitAndEntryControls(UINT32 CoreId);

//
// Perform operations related to HyperTrace LBR dumping
//
IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceLbrPerformDump(HYPERTRACE_LBR_DUMP_PACKETS * LbrDumpRequest);

//
// Perform operations related to HyperTrace LBR based on the request type and parameters
//
IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceLbrPerformOperation(HYPERTRACE_LBR_OPERATION_PACKETS * LbrOperationRequest);

//////////////////////////////////////////////////
//                 PT Functions 	    		//
//////////////////////////////////////////////////

BOOLEAN
HyperTracePtDisable(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest);

//
// Perform operations related to HyperTrace PT based on the request type and parameters
//
IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTracePtPerformOperation(HYPERTRACE_PT_OPERATION_PACKETS * PtOperationRequest);
