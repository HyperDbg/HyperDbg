/**
 * @file Invept.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header for INVEPT functions
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//                   Structures		   			//
//////////////////////////////////////////////////

/**
 * @brief INVEPT Instruction Descriptor
 * 
 */
typedef struct _INVEPT_DESC
{
    UINT64 EptPointer;
    UINT64 Reserved;
} INVEPT_DESC, *PINVEPT_DESC;

//////////////////////////////////////////////////
//                 Functions	    			//
//////////////////////////////////////////////////

UCHAR
Invept(UINT32 Type, INVEPT_DESC * Descriptor);

UCHAR
InveptAllContexts();

UCHAR
InveptSingleContext(UINT64 EptPonter);
