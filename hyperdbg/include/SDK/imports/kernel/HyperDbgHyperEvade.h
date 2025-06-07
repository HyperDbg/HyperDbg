/**
 * @file HyperDbgHyperEvade.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from hyperevade (transparency) module
 * @version 0.14
 * @date 2025-06-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

#ifdef HYPERDBG_HYPEREVADE
#    define IMPORT_EXPORT_HYPEREVADE __declspec(dllexport)
#else
#    define IMPORT_EXPORT_HYPEREVADE __declspec(dllimport)
#endif

//////////////////////////////////////////////////
//            hyperevade functions 	    		//
//////////////////////////////////////////////////

IMPORT_EXPORT_HYPEREVADE VOID
TransparentCheckAndModifyCpuid(INT32 CpuInfo[], PGUEST_REGS Regs);
