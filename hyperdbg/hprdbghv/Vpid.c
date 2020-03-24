#include "Vpid.h"
#include "InlineAsm.h"


void Invvpid(INVVPID_ENUM Type, INVVPID_DESCRIPTOR* Descriptor)
{
	if (!Descriptor)
	{
		static INVVPID_DESCRIPTOR ZeroDescriptor = { 0 };
		Descriptor = &ZeroDescriptor;
	}

	return AsmInvvpid(Type, Descriptor);
}

void InvvpidIndividualAddress(UINT16 Vpid, UINT64 LinearAddress)
{
	INVVPID_DESCRIPTOR Descriptor = { Vpid, 0, LinearAddress };
	return Invvpid(INVVPID_INDIVIDUAL_ADDRESS, &Descriptor);
}

void InvvpidSingleContext(UINT16 Vpid)
{
	INVVPID_DESCRIPTOR Descriptor = { Vpid, 0, 0 };
	return Invvpid(INVVPID_SINGLE_CONTEXT, &Descriptor);
}

void InvvpidAllContexts()
{
	return Invvpid(INVVPID_ALL_CONTEXT, NULL);
}

void InvvpidSingleContextRetainingGlobals(UINT16 Vpid)
{
	INVVPID_DESCRIPTOR Descriptor = { Vpid, 0, 0 };
	return Invvpid(INVVPID_SINGLE_CONTEXT_RETAINING_GLOBALS, &Descriptor);
}