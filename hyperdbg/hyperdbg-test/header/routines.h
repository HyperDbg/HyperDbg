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
extern void inline AsmTest();
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
Uint64ToString(UINT64 value);

BOOLEAN
StringReplace(std::string & str, const std::string & from, const std::string & to);

std::string
ConvertToString(char * Str);
