/**
 * @file Invept.h
 * @author Sina Karvandi (sina@hyperdbg.org)
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

//////////////////////////////////////////////////
//                 Functions	    			//
//////////////////////////////////////////////////

UCHAR
EptInvept(_In_ UINT32 Type, _In_ INVEPT_DESCRIPTOR * Descriptor);

UCHAR
EptInveptAllContexts();

UCHAR
EptInveptSingleContext(_In_ UINT64 EptPointer);
