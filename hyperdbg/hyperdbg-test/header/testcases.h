/**
 * @file testcases.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief header for test cases
 * @details
 * @version 0.11
 * @date 2024-08-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					 Test cases                 //
//////////////////////////////////////////////////

BOOLEAN
TestCommandParser();

BOOLEAN
TestPeParser();

BOOLEAN
TestCodeViewRsdsParser();

BOOLEAN
TestSemanticScripts();

BOOLEAN
TestScriptEngineFloatingPoint();

BOOLEAN
TestScriptEngineVariableTypes();

BOOLEAN
TestSnapshotFuzzingEngine();

int
FuzzTargetUserModeParser(const UINT8 * Buffer, size_t Size);

BOOLEAN
FuzzTargetKernelIoctlHandler(UINT32 IoctlCode, const UINT8 * InputBuffer, size_t InputSize, UINT32 * OutStatus);
