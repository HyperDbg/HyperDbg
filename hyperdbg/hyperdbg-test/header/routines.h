/**
 * @file routines.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief header for testing routines
 * @details
 * @version 0.1
 * @date 2020-09-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					 Functions                  //
//////////////////////////////////////////////////
BOOLEAN
TestCase(std::vector<std::string> & TestCase);

//////////////////////////////////////////////////
//		     Assembly Functions                 //
//////////////////////////////////////////////////

extern "C" {
extern VOID inline AsmTest();
}

//////////////////////////////////////////////////
//				Test Case Routines              //
//////////////////////////////////////////////////

VOID
TestCreateLookupTable(HANDLE PipeHandle, PVOID KernelInformation, UINT32 KernelInformationSize);

//////////////////////////////////////////////////
//				General Functions               //
//////////////////////////////////////////////////

std::string
Uint64ToString(UINT64 Value);

BOOLEAN
StringReplace(std::string & Str, const std::string & From, const std::string & To);

std::string
ConvertToString(CHAR * Str);
