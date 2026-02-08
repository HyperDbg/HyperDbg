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
//            hypertrace functions 	    		//
//////////////////////////////////////////////////

//
// Check if LBR is supported and initialize LBR state list and lock
//
IMPORT_EXPORT_HYPERTRACE BOOLEAN
LbrCheck();
