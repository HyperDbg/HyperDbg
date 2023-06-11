/**
 * @file Disassembler.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for disassembler in kernel
 * @details
 *
 * @version 0.3
 * @date 2023-06-06
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

BOOLEAN
DisassemblerShowInstructionsInVmxNonRootMode(PVOID Address, UINT32 Length, BOOLEAN Is32Bit);

BOOLEAN
DisassemblerShowOneInstructionInVmxNonRootMode(PVOID Address, UINT64 ActualRip, BOOLEAN Is32Bit);

UINT32
DisassemblerShowOneInstructionInVmxRootMode(PVOID Address, BOOLEAN Is32Bit);
