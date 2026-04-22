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
//            Example Functions 	    		//
//////////////////////////////////////////////////

//
// Used for testing purposes
//
IMPORT_EXPORT_HYPERTRACE VOID
HyperTraceExamplePerformLbrTrace(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall);

//////////////////////////////////////////////////
//            HyperTrace Functions 	    		//
//////////////////////////////////////////////////

//
// Initialize the hypertrace module with the provided callbacks
//
IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceInitCallback(HYPERTRACE_CALLBACKS * HypertraceCallbacks);

//
// Uninitialize the HyperTrace module
//
IMPORT_EXPORT_HYPERTRACE VOID
HyperTraceUninit();

//
// Perform operations related to HyperTrace based on the request type and parameters
//
IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTracePerformOperation(HYPERTRACE_OPERATION_PACKETS * LbrOperationRequest,
                           BOOLEAN                        ApplyFromVmxRootMode);

//////////////////////////////////////////////////
//                LBR Functions 	    		//
//////////////////////////////////////////////////

IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceStartLbr(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall);

IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceStopLbr(BOOLEAN ApplyFromVmxRootMode, BOOLEAN ApplyByVmcall);

IMPORT_EXPORT_HYPERTRACE BOOLEAN
HyperTraceQueryStateOfLbrSaveAndLoadVmExitAndEntryControls(UINT32 CoreId);
