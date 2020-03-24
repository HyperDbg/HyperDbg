#include "Invept.h"
#include "InlineAsm.h"

/* Invoke the Invept instruction */
unsigned char Invept(UINT32 Type, INVEPT_DESC* Descriptor)
{
	if (!Descriptor)
	{
		INVEPT_DESC ZeroDescriptor = { 0 };
		Descriptor = &ZeroDescriptor;
	}

	return AsmInvept(Type, Descriptor);
}

/* Invalidates a single context in ept cache table */
unsigned char InveptSingleContext(UINT64 EptPointer)
{
	INVEPT_DESC Descriptor = { 0 };
	Descriptor.EptPointer = EptPointer;
	Descriptor.Reserved = 0;
	return Invept(INVEPT_SINGLE_CONTEXT, &Descriptor);
}

/* Invalidates all contexts in ept cache table */
unsigned char InveptAllContexts()
{
	return Invept(INVEPT_ALL_CONTEXTS, NULL);
}
