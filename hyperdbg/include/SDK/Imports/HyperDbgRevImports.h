/**
 * @file HyperDbgRevImports.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from reversing machine interface
 * @version 0.2
 * @date 2023-02-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Header file of hpr
// Imports
//
#ifdef __cplusplus
extern "C" {
#endif

//
// Reversing Machine Module
//
__declspec(dllimport) int ReversingMachineStart();
__declspec(dllimport) int ReversingMachineStop();

#ifdef __cplusplus
}
#endif
