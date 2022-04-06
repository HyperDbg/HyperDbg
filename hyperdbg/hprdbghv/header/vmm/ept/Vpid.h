/**
 * @file Vpid.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief VPID Headers
 * @details
 * @version 0.1
 * @date 2020-04-11
 * 
 * @copyright This project is released under the GNU Public License v3.
 * 
 */
#pragma once

//////////////////////////////////////////////////
//					Enums						//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//					Definitions					//
//////////////////////////////////////////////////

/**
 * @brief VPID Tag
 * 
 */
#define VPID_TAG 0x1

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

VOID
Invvpid_fn(_In_ INVVPID_TYPE Type, _Inout_ INVVPID_DESCRIPTOR * Descriptor);

VOID
InvvpidIndividualAddress_fn(_In_ UINT16 Vpid, _In_ UINT64 LinearAddress);

VOID
InvvpidSingleContext_fn(_In_ UINT16 Vpid);

VOID
InvvpidAllContexts_fn();

VOID
InvvpidSingleContextRetainingGlobals_fn(_In_ UINT16 Vpid);
