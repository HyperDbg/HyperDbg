/**
 * @file Loader.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Routines for perform initial VMM and RM
 * @details
 *
 * @version 0.2
 * @date 2023-01-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//		    	 Loader Functions				//
//////////////////////////////////////////////////

VOID
LoaderUninitializeLogTracer();

BOOLEAN
LoaderInitVmmAndReversingMachine();
