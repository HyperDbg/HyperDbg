/**
 * @file routines.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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

VOID
TestCaseDispatcher(UINT32 TestCase, HANDLE PipeHandle);

//////////////////////////////////////////////////
//		     Assembly Functions                 //
//////////////////////////////////////////////////

extern "C" {
extern void inline XorSledFunction();
}

//////////////////////////////////////////////////
//				Test Case Routines              //
//////////////////////////////////////////////////

VOID
AttachDetachTest(HANDLE PipeHandle);
