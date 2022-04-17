/**
 * @file Msr.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Model-Specific Registers Numbers and definitions
 * @details This file does not contain all the MSRs, instead it contains general MSRs
 * that used in this projec, some other MSRs e.g., EPT related MSRs are in their specific
 * headers
 * 
 * Definition of different Model-Specific Register (MSRs):
 * Thanks to all rust developers, this file is copied from:
 *	        https://github.com/gz/rust-x86/blob/master/src/msr.rs
 * 
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				    Structures					//
//////////////////////////////////////////////////

/**
 * @brief General MSR Structure
 * 
 */
typedef union _MSR
{
    struct
    {
        ULONG Low;
        ULONG High;
    } Fields;

    UINT64 Flags;

} MSR, *PMSR;
