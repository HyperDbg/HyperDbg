/**
 * @file DebugRegisters.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Header for debug registers implementation
 * @details
 * 
 * @version 0.1
 * @date 2021-11-23
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief Debug Register 7 Structure
 * 
 */
typedef union _DEBUG_REGISTER_7
{
    UINT64 Flags;

    struct
    {
        UINT64 LocalBreakpoint0 : 1;
        UINT64 GlobalBreakpoint0 : 1;
        UINT64 LocalBreakpoint1 : 1;
        UINT64 GlobalBreakpoint1 : 1;
        UINT64 LocalBreakpoint2 : 1;
        UINT64 GlobalBreakpoint2 : 1;
        UINT64 LocalBreakpoint3 : 1;
        UINT64 GlobalBreakpoint3 : 1;
        UINT64 LocalExactBreakpoint : 1;
        UINT64 GlobalExactBreakpoint : 1;
        UINT64 Reserved1 : 1; // always 1
        UINT64 RestrictedTransactionalMemory : 1;
        UINT64 Reserved2 : 1; // always 0
        UINT64 GeneralDetect : 1;
        UINT64 Reserved3 : 2; // always 0
        UINT64 ReadWrite0 : 2;
        UINT64 Length0 : 2;
        UINT64 ReadWrite1 : 2;
        UINT64 Length1 : 2;
        UINT64 ReadWrite2 : 2;
        UINT64 Length2 : 2;
        UINT64 ReadWrite3 : 2;
        UINT64 Length3 : 2;
    };
} DEBUG_REGISTER_7, *PDEBUG_REGISTER_7;

/**
 * @brief Debug Register 6 Structure
 * 
 */
typedef union DEBUG_REGISTER_6
{
    UINT64 Flags;

    struct
    {
        UINT64 BreakpointCondition : 4;
        UINT64 Reserved1 : 8; // always 1
        UINT64 Reserved2 : 1; // always 0
        UINT64 DebugRegisterAccessDetected : 1;
        UINT64 SingleInstruction : 1;
        UINT64 TaskSwitch : 1;
        UINT64 RestrictedTransactionalMemory : 1;
        UINT64 Reserved3 : 15; // always 1
    };
} DEBUG_REGISTER_6, *PDEBUG_REGISTER_6;

//////////////////////////////////////////////////
//				   	Enums 		     			//
//////////////////////////////////////////////////

typedef enum _DEBUG_REGISTER_TYPE
{
    BREAK_ON_INSTRUCTION_FETCH,
    BREAK_ON_WRITE_ONLY,
    BREAK_ON_IO_READ_OR_WRITE_NOT_SUPPORTED,
    BREAK_ON_READ_AND_WRITE_BUT_NOT_FETCH
} DEBUG_REGISTER_TYPE;

//////////////////////////////////////////////////
//				   	Functions	     			//
//////////////////////////////////////////////////

BOOLEAN
DebugRegistersSet(UINT32 DebugRegNum, DEBUG_REGISTER_TYPE ActionType, BOOLEAN ApplyToVmcs, UINT64 TargetAddress);
