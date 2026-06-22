/**
 * @file HyperDbgHyperPerf.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from hyperperf (pmu) module
 * @version 0.21
 * @date 2026-06-22
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifdef HYPERDBG_HYPERPERF
#    define IMPORT_EXPORT_HYPERPERF __declspec(dllexport)
#else
#    define IMPORT_EXPORT_HYPERPERF __declspec(dllimport)
#endif

//////////////////////////////////////////////////
//            HyperPerf Functions 	    		//
//////////////////////////////////////////////////

//
// Initialize the hyperperf module with the provided callbacks
//
IMPORT_EXPORT_HYPERPERF BOOLEAN
HyperPerfInitCallback(HYPERPERF_CALLBACKS * HyperPerfCallbacks, BOOLEAN RunningOnHypervisorEnvironment);

//
// Uninitialize the HyperPerf module
//
IMPORT_EXPORT_HYPERPERF VOID
HyperPerfUninit();
