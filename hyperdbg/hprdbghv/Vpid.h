#pragma once
#include <ntddk.h>

//////////////////////////////////////////////////
//					Enums						//
//////////////////////////////////////////////////

typedef enum _INVVPID_ENUM
{
	INVVPID_INDIVIDUAL_ADDRESS = 0x00000000,
	INVVPID_SINGLE_CONTEXT = 0x00000001,
	INVVPID_ALL_CONTEXT = 0x00000002,
	INVVPID_SINGLE_CONTEXT_RETAINING_GLOBALS = 0x00000003
}INVVPID_ENUM, * PINVVPID_ENUM;


//////////////////////////////////////////////////
//					Structures					//
//////////////////////////////////////////////////

typedef struct _INVVPID_DESCRIPTOR
{
	UINT64 VPID : 16;
	UINT64 RESERVED : 48;
	UINT64 LINEAR_ADDRESS;

} INVVPID_DESCRIPTOR, * PINVVPID_DESCRIPTOR;



//////////////////////////////////////////////////
//					Definitions					//
//////////////////////////////////////////////////
#define VPID_TAG			0x1

//////////////////////////////////////////////////
//					Functions					//
//////////////////////////////////////////////////

void Invvpid(INVVPID_ENUM Type, INVVPID_DESCRIPTOR* Descriptor);
void InvvpidIndividualAddress(UINT16 Vpid, UINT64 LinearAddress);
void InvvpidSingleContext(UINT16 Vpid);
void InvvpidAllContexts();
void InvvpidSingleContextRetainingGlobals(UINT16 Vpid);
