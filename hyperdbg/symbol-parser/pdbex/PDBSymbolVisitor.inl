#include "PDBSymbolVisitor.h"
#include "PDB.h"
#include "PDBSymbolVisitorBase.h"
#include "PDBReconstructorBase.h"

#include <memory>
#include <stack>

template <
	typename MEMBER_DEFINITION_TYPE
>
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::PDBSymbolVisitor(
	PDBReconstructorBase* ReconstructVisitor,
	void* MemberDefinitionSettings
	)
{
	m_ReconstructVisitor = ReconstructVisitor;
	m_MemberDefinitionSettings = MemberDefinitionSettings;
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::Run(
	const SYMBOL* Symbol
	)
{
	Visit(Symbol);
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::Visit(
	const SYMBOL* Symbol
	)
{
	PDBSymbolVisitorBase::Visit(Symbol);
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitBaseType(
	const SYMBOL* Symbol
	)
{
	//
	// BaseType:
	// short/int/long/...
	//

	m_MemberContextStack.top()->VisitBaseType(Symbol);
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitEnumType(
	const SYMBOL* Symbol
	)
{
	//
	// EnumType:
	// enum XYZ
	// {
	//   XYZ_1,
	//   XYZ_2,
	// };
	//

	//
	// enum XYZ ...
	//

	if (m_ReconstructVisitor->OnEnumType(Symbol))
	{
		//
		// ...
		// {
		//   XYZ_1,
		//   XYZ_2,
		// }
		//

		m_ReconstructVisitor->OnEnumTypeBegin(Symbol);
		PDBSymbolVisitorBase::VisitEnumType(Symbol);
		m_ReconstructVisitor->OnEnumTypeEnd(Symbol);
	}
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitTypedefType(
	const SYMBOL* Symbol
	)
{

}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitPointerType(
	const SYMBOL* Symbol
	)
{
	//
	// PointerType:
	// short*/int*/long*/...
	//

	m_MemberContextStack.top()->VisitPointerTypeBegin(Symbol);
	PDBSymbolVisitorBase::VisitPointerType(Symbol);
	m_MemberContextStack.top()->VisitPointerTypeEnd(Symbol);
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitArrayType(
	const SYMBOL* Symbol
	)
{
	//
	// ArrayType:
	// int XYZ[8];
	//

	m_MemberContextStack.top()->VisitArrayTypeBegin(Symbol);
	PDBSymbolVisitorBase::VisitArrayType(Symbol);
	m_MemberContextStack.top()->VisitArrayTypeEnd(Symbol);

}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitFunctionType(
	const SYMBOL* Symbol
	)
{
	//
	// #TODO:
	// Currently, show void* instead of functions.
	//

	m_MemberContextStack.top()->VisitFunctionTypeBegin(Symbol);
	//PDBSymbolVisitorBase::VisitFunctionType(Symbol);
	m_MemberContextStack.top()->VisitFunctionTypeEnd(Symbol);
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitFunctionArgType(
	const SYMBOL* Symbol
	)
{
	m_MemberContextStack.top()->VisitFunctionArgTypeBegin(Symbol);
	PDBSymbolVisitorBase::VisitFunctionArgType(Symbol);
	m_MemberContextStack.top()->VisitFunctionArgTypeEnd(Symbol);
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitUdt(
	const SYMBOL* Symbol
	)
{
	//
	// Udt:
	// struct XYZ
	// {
	//   int XYZ_1;
	//   char XYZ_2;
	// };
	//

	//
	// struct XYZ ...
	//

	if (m_ReconstructVisitor->OnUdt(Symbol))
	{
		//
		// ...
		// {
		//   int XYZ_1;
		//   char XYZ_2;
		// }
		//

		if (Symbol->Size > 0)
		{
			//
			// Save the current stacks of anonymous UDTs.
			// This prevents interferencing of members
			// of nested UDTs.
			//
			// Stacks are restored after visiting of the current UDT.
			//
			AnonymousUdtStack AnonymousUDTStackBackup;
			AnonymousUdtStack AnonymousUnionStackBackup;
			AnonymousUdtStack AnonymousStructStackBackup;
			m_AnonymousUdtStack.swap(AnonymousUDTStackBackup);
			m_AnonymousUnionStack.swap(AnonymousUnionStackBackup);
			m_AnonymousStructStack.swap(AnonymousStructStackBackup);

			{
				m_MemberContextStack.push(MemberDefinitionFactory());

				m_ReconstructVisitor->OnUdtBegin(Symbol);
				PDBSymbolVisitorBase::VisitUdt(Symbol);
				m_ReconstructVisitor->OnUdtEnd(Symbol);

				m_MemberContextStack.pop();
			}

			m_AnonymousStructStack.swap(AnonymousStructStackBackup);
			m_AnonymousUnionStack.swap(AnonymousUnionStackBackup);
			m_AnonymousUdtStack.swap(AnonymousUDTStackBackup);
		}
	}
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitOtherType(
	const SYMBOL* Symbol
	)
{

}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitEnumField(
	const SYMBOL_ENUM_FIELD* EnumField
	)
{
	m_ReconstructVisitor->OnEnumField(EnumField);
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitUdtField(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	BOOL IsBitFieldMember = UdtField->Bits != 0;
	BOOL IsFirstBitFieldMember = IsBitFieldMember && !m_PreviousBitFieldField;

	//
	// Push new member context.
	//

	m_MemberContextStack.push(MemberDefinitionFactory());
	m_MemberContextStack.top()->SetMemberName(UdtField->Name);

	if (!IsBitFieldMember || IsFirstBitFieldMember)
	{
		//
		// Handling of inlined user defined types.
		//
		// These checks are performed when the current member
		// is not a bitfield member (except the first one).
		//
		// Note that calling these inside of the bitfield
		// would not make sense.
		//

		CheckForDataFieldPadding(UdtField);
		CheckForAnonymousUnion(UdtField);
		CheckForAnonymousStruct(UdtField);
	}

	//
	// Is this the first bitfield member?
	//

	if (IsFirstBitFieldMember)
	{
		BOOL IsFirstBitFieldMemberPadding = UdtField->BitPosition != 0;

		assert(m_CurrentBitField.HasValue() == false);

		//
		// If first bitfield field is padding, set "FirstUdtFieldBitField" as nullptr.
		// This forces creation of the "wrapping" struct even if this bitfield
		// has only one NAMED member.
		//

		m_CurrentBitField.FirstUdtFieldBitField = IsFirstBitFieldMemberPadding ? nullptr : UdtField;
		m_CurrentBitField.LastUdtFieldBitField = GetNextUdtFieldWithRespectToBitFields(UdtField) - 1;

		m_ReconstructVisitor->OnUdtFieldBitFieldBegin(
			m_CurrentBitField.FirstUdtFieldBitField,
			m_CurrentBitField.LastUdtFieldBitField
			);
	}

	if (IsBitFieldMember)
	{
		//
		// Handling of unnamed bitfield fields.
		//

		CheckForBitFieldFieldPadding(UdtField);
	}

	//
	// Dump the field.
	//

	m_ReconstructVisitor->OnUdtFieldBegin(UdtField);
	Visit(UdtField->Type);
	m_ReconstructVisitor->OnUdtField(UdtField, m_MemberContextStack.top().get());
	m_ReconstructVisitor->OnUdtFieldEnd(UdtField);

	m_MemberContextStack.pop();

	//
	// Remember this UdtField as a last bitfield field.
	//

	if (IsBitFieldMember)
	{
		m_PreviousBitFieldField = UdtField;
	}
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitUdtFieldEnd(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	CheckForEndOfAnonymousUdt(UdtField);
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::VisitUdtFieldBitFieldEnd(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	assert(m_CurrentBitField.HasValue() == true);
	assert(m_CurrentBitField.LastUdtFieldBitField == UdtField);

	m_ReconstructVisitor->OnUdtFieldBitFieldEnd(
		m_CurrentBitField.FirstUdtFieldBitField,
		m_CurrentBitField.LastUdtFieldBitField
		);

	m_CurrentBitField.Clear();

	VisitUdtFieldEnd(UdtField);

	m_PreviousBitFieldField = nullptr;
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::CheckForDataFieldPadding(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	//
	// Members are sometimes not properly aligned.
	// Example (original definition):
	//   struct XYZ
	//   {
	//     char XYZ_1;
	//     int  XYZ_2;  // This member actually begins at offset 4 (if packing was not applied),
	//                  // resulting in 3 spare bytes before this field.
	//   };
	//
	// This routine creates a "padding" member to fill the empty space, so the final reconstructed
	// structure would look like following:
	//   struct XYZ
	//   {
	//     char XYZ_1;
	//     char Padding_0[3]; // Padding member.
	//     int  XYZ_2;
	//   };
	//

	//
	// Take previous member, sum the size of the field and its offset
	// and compare it to the current member offset.
	// If the sum is less than the current member offset, there is a spare space
	// which will be filled by padding member.
	//

	UdtFieldContext UdtFieldCtx(UdtField);
	DWORD PreviousUdtFieldOffset = 0;
	DWORD SizeOfPreviousUdtField = 0;

	if (UdtFieldCtx.IsFirst() == false)
	{
		PreviousUdtFieldOffset = m_PreviousUdtField->Offset;
		SizeOfPreviousUdtField = m_SizeOfPreviousUdtField;
	}

	if (PreviousUdtFieldOffset + SizeOfPreviousUdtField < UdtField->Offset)
	{
		DWORD Difference = UdtField->Offset - (PreviousUdtFieldOffset + SizeOfPreviousUdtField);

		//
		// We can use !(Difference & 3) if we want to be clever.
		//

		BOOL DifferenceIsDivisibleBy4 = !(Difference % 4);

		m_ReconstructVisitor->OnPaddingMember(
			UdtField,
			DifferenceIsDivisibleBy4 ?     btLong     :   btChar  ,
			DifferenceIsDivisibleBy4 ?       4        :     1     ,
			DifferenceIsDivisibleBy4 ? Difference / 4 : Difference
			);
	}
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::CheckForBitFieldFieldPadding(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	BOOL WasPreviousBitFieldMember = m_PreviousBitFieldField
	  ? m_PreviousBitFieldField->Bits != 0
	  : FALSE;

	if (
	  //
	  // Checks if the first bitfield field is unnamed:
	  //   struct XYZ
	  //   {
	  //     unsigned     : 16;  // Unnamed bitfield field!
	  //     unsigned var : 16;
	  //   };
	  //

	  (UdtField->BitPosition != 0 && !WasPreviousBitFieldMember) ||

	  //
	  // Checks if some middle bitfield field is unnamed:
	  //   struct XYZ
	  //   {
	  //     unsigned var1 : 12;
	  //     unsigned      : 10;  // Unnamed bitfield field!
	  //     unsigned var2 : 12;
	  //   };
	  //

	  (WasPreviousBitFieldMember &&
	   UdtField->BitPosition != m_PreviousBitFieldField->BitPosition + m_PreviousBitFieldField->Bits)
	  )
	{
		//
		// Create padding bitfield field.
		//

		m_ReconstructVisitor->OnPaddingBitFieldField(UdtField, m_PreviousBitFieldField);
	}
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::CheckForAnonymousUnion(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	//
	// When some UDT contains anonymous unions, they are not projected
	// into the PDB file - they are part of the UDT (ie. struct).
	// Anonymous unions can be detected through checking of starting offsets
	// of members in the structure - if there exist more than 1 member (DataField)
	// which start at the same offset, they are placed inside of the union.
	//

	UdtFieldContext UdtFieldCtx(UdtField);

	if (UdtFieldCtx.IsLast())
	{
		//
		// If current member is the last member of the current UDT,
		// there won't be any anonymous unions.
		//

		return;
	}

	if (!m_AnonymousUdtStack.empty() &&
	     m_AnonymousUdtStack.top()->Kind == UdtUnion)
	{
		//
		// Don't start an anonymous union while we're still inside of one.
		//

		return;
	}

	//
	// Iterate members starting from the current one.
	// If any following member which starts at the same offset
	// as the current member does exist, then they must be wrapped
	// inside of the union.
	//

	do
	{
		if (UdtFieldCtx.NextUdtField->Offset == UdtField->Offset)
		{

			//
			// Do not try to wrap in the union
			// those members, which are out of bounds
			// of the anonymous struct we're currently in.
			//
			// In other words, this prevents creating meaningless unions
			// which have only one member - because it detected
			// that there exist member, which has the same offset -
			// - but the member is already in another struct.
			//

			if (m_AnonymousStructStack.empty() ||
			  (!m_AnonymousStructStack.empty() && UdtFieldCtx.NextUdtField <= m_AnonymousStructStack.top()->LastUdtField))
			{
				PushAnonymousUdt(std::make_shared<AnonymousUdt>(UdtUnion, UdtField, nullptr, UdtField->Type->Size));
				m_ReconstructVisitor->OnAnonymousUdtBegin(UdtUnion, UdtField);
				break;
			}
		}
	} while (UdtFieldCtx.GetNext());
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::CheckForAnonymousStruct(
	const SYMBOL_UDT_FIELD* UdtField
	)
{

	//
	// When some UDT contains anonymous structs, they are not projected
	// into the PDB file - they are part of the structure (Udt, respectively).
	// This dumper creates anonymous structs where it's obvious
	// that an anonmous structure is present in the union.
	// Consider following snippet:
	//
	// 0: kd> dt ntdll!_KTHREAD
	// ...
	//   +0x190 StackBase        : Ptr32 Void
	//   +0x194 SuspendApc       : _KAPC
	//   +0x194 SuspendApcFill0  : [1] UChar
	//   +0x195 ResourceIndex    : UChar
	//   +0x194 SuspendApcFill1  : [3] UChar
	//   +0x197 QuantumReset     : UChar
	//   +0x194 SuspendApcFill2  : [4] UChar
	//   +0x198 KernelTime       : Uint4B
	//   +0x194 SuspendApcFill3  : [36] UChar
	//   +0x1b8 WaitPrcb         : Ptr32 _KPRCB
	// ...
	//
	// Note that offset 0x194 is shared among many members, even though after those members
	// is placed another member which starts at another offset than 0x194.
	// This is effectively done by structs placed inside unions. The above snipped could be represented
	// as:
	//
	// struct _KTHREAD {
	// ...
	//   /* 0x0190 */ void* StackBase;
	//   union {
	//     /* 0x0194 */ struct _KAPC SuspendApc;
	//     struct {
	//       /* 0x0194 */ unsigned char SuspendApcFill0[1];
	//       /* 0x0195 */ unsigned char ResourceIndex;
	//     };
	//     struct {
	//       /* 0x0194 */ unsigned char SuspendApcFill1[3];
	//       /* 0x0197 */ unsigned char QuantumReset;
	//     };
	//     struct {
	//       /* 0x0194 */ unsigned char SuspendApcFill2[4];
	//       /* 0x0198 */ unsigned long KernelTime;
	//     };
	//     struct {
	//       /* 0x0194 */ unsigned char SuspendApcFill3[36];
	//       /* 0x01b8 */ KPRCB* WaitPrcb;
	//     };
	// ...
	// };
	//

	UdtFieldContext UdtFieldCtx(UdtField);

	if (UdtFieldCtx.IsLast())
	{
		//
		// If current member is the last member of the current UDT,
		// there won't be any anonymous structs.
		//

		return;
	}

	if (!m_AnonymousUdtStack.empty() &&
	     m_AnonymousUdtStack.top()->Kind != UdtUnion)
	{
		//
		// Don't start an anonymous struct while we're still inside of one.
		//

		return;
	}

	if (UdtFieldCtx.NextUdtField->Offset <= UdtField->Offset)
	{
		//
		// If the offset of the next member is less than or equals to the offset
		// of the actual member, we cannot create a struct here.
		//

		return;
	}

	do
	{

		//
		// If offsets of next member and current member equal
		// or the offset of the next member is less than the offset
		// of the end of the last anonymous UDT,
		// we will create an anonymous struct.
		//

		if (
		     UdtFieldCtx.NextUdtField->Offset == UdtField->Offset ||
		     (
		       !m_AnonymousUdtStack.empty() &&
		       UdtFieldCtx.NextUdtField->Offset < m_AnonymousUdtStack.top()->FirstUdtField->Offset + m_AnonymousUdtStack.top()->Size
		     )
		)
		{

			//
			// Guess the last member of this anonymous struct.
			// Note that this guess is not required to be correct.
			// It only serves as a break for creation of anonymous unions.
			//

			do
			{
				bool IsEndOfAnonymousStruct =
					UdtFieldCtx.IsLast() ||
					UdtFieldCtx.NextUdtField->Offset <= UdtField->Offset;

				if (IsEndOfAnonymousStruct)
				{
					break;
				}
			} while (UdtFieldCtx.GetNext());

			//
			// UdtFieldCtx.CurrentUdtField now holds the last member
			// of this anonymous struct.
			//

			PushAnonymousUdt(std::make_shared<AnonymousUdt>(UdtStruct, UdtField, UdtFieldCtx.CurrentUdtField));
			m_ReconstructVisitor->OnAnonymousUdtBegin(UdtStruct, UdtField);
			break;
		}
	} while (UdtFieldCtx.GetNext());
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::CheckForEndOfAnonymousUdt(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	//
	// This method is called after each UDT field
	// and after the last member of the bitfield,
	// so this is the best place to refresh
	// these two properties.
	//

	m_PreviousUdtField       = UdtField;
	m_SizeOfPreviousUdtField = UdtField->Type->Size;

	if (m_AnonymousUdtStack.empty())
	{
		//
		// No UDT to check.
		//

		return;
	}

	UdtFieldContext UdtFieldCtx(UdtField, FALSE);

	//
	// The current member could be nested more than once
	// and at this point more anonymous UDTs could be closed,
	// so the code is wrapped inside of the loop.
	//

	AnonymousUdt* LastAnonymousUdt;

	do
	{
		LastAnonymousUdt = m_AnonymousUdtStack.top().get();
		LastAnonymousUdt->MemberCount += 1;

		bool IsEndOfAnonymousUdt = false;

		if (LastAnonymousUdt->Kind == UdtUnion)
		{
			//
			// Update the size of the current nested union.
			// The size of the union is as big as its biggest member.
			//

			LastAnonymousUdt->Size = max(LastAnonymousUdt->Size, m_SizeOfPreviousUdtField);

			//
			// Determination if this is the end of the anonymous union.
			//
			//   - UdtFieldCtx.IsLast()
			//     - If the current member is last in the root structure.
			//
			//       This check covers all opened anonymous UDTs before
			//       top root structure ends.
			//
			//   - UdtFieldCtx.NextUdtField->Offset < UdtField->Offset
			//     - If the offset of the next member is less than to the offset of the current member.
			//
			//   - (UdtFieldCtx.NextUdtField->Offset == UdtField->Offset + LastAnonymousUdt->Size)
			//     - If the offset of the next member equals to the sum of
			//       * the offset of the current member and
			//       * the computed size of the current nested union.
			//
			//   - (UdtFieldCtx.NextUdtField->Offset == UdtField->Offset + 8 && Is64BitBasicType(UdtFieldCtx.NextUdtField->Type))
			//     - If the offset of the next member equals to the offset of current member + 8 and
			//       the next member is of type [u]int64_t.
			//       This is the cause of the alignment.
			//
			//   - (UdtFieldCtx.NextUdtField->Offset >  UdtField->Offset && UdtField->Bits != 0)
			//     - If the offset of the next member is bigger than the offset of the current member and
			//       current member is not a part of the bitfield.
			//
			//   - (UdtFieldCtx.NextUdtField->Offset >  UdtField->Offset && UdtField->Offset + UdtField->Type->Size != UdtFieldCtx.NextUdtField->Offset)
			//     - If the offset of the next member is bigger than the offset of the current member and
			//       the offset of the end of the current member is not equal to the offset of the next member.
			//

			IsEndOfAnonymousUdt =
			   UdtFieldCtx.IsLast() ||
			   UdtFieldCtx.NextUdtField->Offset <  UdtField->Offset ||
			  (UdtFieldCtx.NextUdtField->Offset == UdtField->Offset + LastAnonymousUdt->Size) ||
			  (UdtFieldCtx.NextUdtField->Offset == UdtField->Offset + 8 && Is64BitBasicType(UdtFieldCtx.NextUdtField->Type)) ||
			  (UdtFieldCtx.NextUdtField->Offset >  UdtField->Offset && UdtField->Bits != 0) ||
			  (UdtFieldCtx.NextUdtField->Offset >  UdtField->Offset && UdtField->Offset + UdtField->Type->Size != UdtFieldCtx.NextUdtField->Offset);
		}
		else
		{
			//
			// Update the size of the current nested structure/class.
			// The total size increases by the size of previous member.
			// Because the previous member could be non-trivial member (ie. union),
			// we will use the variable m_SizeOfPreviousUdtField.
			//
			LastAnonymousUdt->Size += m_SizeOfPreviousUdtField;

			//
			// Determination if this is the end of the anonymous struct.
			//
			//   - UdtFieldCtx.IsLast()
			//     - If the current member is last in the root structure.
			//
			//       This check covers all opened anonymous UDTs before
			//       top root structure ends.
			//
			//   - UdtFieldCtx.NextUdtField->Offset <= UdtField->Offset
			//     - If the offset of the next member is less than or equal to the offset of the current member.
			//

			IsEndOfAnonymousUdt =
				UdtFieldCtx.IsLast() ||
				UdtFieldCtx.NextUdtField->Offset <= UdtField->Offset;


			//
			// Special condition for closing anonymous structs
			// which are placed inside of the anonymous unions.
			//
			// This prevents structs to be longer than it's actually needed.
			//
			// If the offset of the first member after the parent union
			// would be equal to the actual offset of the next member,
			// we can close this struct.
			// Also, in this struct must be at least 2 members.
			//

			AnonymousUdt* LastAnonymousUnion =
				m_AnonymousUnionStack.empty()
				? nullptr
				: m_AnonymousUnionStack.top().get();

			IsEndOfAnonymousUdt = IsEndOfAnonymousUdt || (
			    LastAnonymousUnion != nullptr &&
			   (LastAnonymousUnion->FirstUdtField->Offset + LastAnonymousUnion->Size == UdtField->Offset + UdtField->Type->Size ||
			    LastAnonymousUnion->FirstUdtField->Offset + LastAnonymousUnion->Size == UdtFieldCtx.NextUdtField->Offset) &&
			    LastAnonymousUdt->MemberCount >= 2
			);
		}

		if (IsEndOfAnonymousUdt)
		{
			//
			// Close the anonymous UDT.
			//

			m_SizeOfPreviousUdtField = LastAnonymousUdt->Size;
			LastAnonymousUdt->LastUdtField = UdtField;

			m_ReconstructVisitor->OnAnonymousUdtEnd(
				LastAnonymousUdt->Kind,
				LastAnonymousUdt->FirstUdtField,
				LastAnonymousUdt->LastUdtField,
				LastAnonymousUdt->Size
				);

			PopAnonymousUdt();

			LastAnonymousUdt = nullptr;
		}

		if (!m_AnonymousUdtStack.empty())
		{
			if (m_AnonymousUdtStack.top()->Kind == UdtUnion)
			{
				//
				// If the AnonymousUdtStack is still not empty
				// and an anonymous union is at the top of it,
				// we must set the first member of the anonymous union
				// as the current member.
				//
				// The reason behind is that the first member of the union
				// is guaranteed to be at the starting offset of the union.
				// This not might be true for another members, as they
				// can be part of another anonymous struct.
				//
				// Example:
				//
				// union {
				//   int a;    /* 0x10 */
				//   int b;    /* 0x10 */
				//   struct {
				//     int c;  /* 0x10 */
				//     int d;  /* 0x14 */
				//             /*
				//              * This is where we are now. We end the struct here,
				//              * and the current offset is 0x14,
				//              * but the union starts at the offset 0x10, so we set
				//              * the current member to the first member of the unnamed union
				//              * which is "int a".
				//              */
				//   };
				// };

				UdtField = m_AnonymousUdtStack.top()->FirstUdtField;
				m_PreviousUdtField = UdtField;
			}
			else
			{
				//
				// If at the top of the AnonymousUdtStack is the struct or class,
				// set the current member back to the actual current member
				// which has been provided.
				//

				UdtField = UdtFieldCtx.CurrentUdtField;
				m_PreviousUdtField = UdtField;
			}
		}
	} while (LastAnonymousUdt == nullptr && !m_AnonymousUdtStack.empty());
}

template <
	typename MEMBER_DEFINITION_TYPE
>
std::shared_ptr<UdtFieldDefinitionBase>
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::MemberDefinitionFactory()
{
	auto MemberDefinition = std::make_shared<MEMBER_DEFINITION_TYPE>();
	MemberDefinition->SetSettings(m_MemberDefinitionSettings);

	return MemberDefinition;
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::PushAnonymousUdt(
	std::shared_ptr<AnonymousUdt> Item
	)
{
	m_AnonymousUdtStack.push(Item);

	if (Item->Kind == UdtUnion)
	{
		m_AnonymousUnionStack.push(Item);
	}
	else
	{
		m_AnonymousStructStack.push(Item);
	}
}

template <
	typename MEMBER_DEFINITION_TYPE
>
void
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::PopAnonymousUdt()
{
	if (m_AnonymousUdtStack.top()->Kind == UdtUnion)
	{
		m_AnonymousUnionStack.pop();
	}
	else
	{
		m_AnonymousStructStack.pop();
	}

	m_AnonymousUdtStack.pop();
}

template <
	typename MEMBER_DEFINITION_TYPE
>
const SYMBOL_UDT_FIELD*
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::GetNextUdtFieldWithRespectToBitFields(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	const SYMBOL_UDT* ParentUdt = &UdtField->Parent->u.Udt;
	DWORD UdtFieldCount = ParentUdt->FieldCount;

	const SYMBOL_UDT_FIELD* NextUdtField = UdtField + 1;
	const SYMBOL_UDT_FIELD* EndOfUdtField = &ParentUdt->Fields[UdtFieldCount];

	if (NextUdtField >= EndOfUdtField)
	{
		return EndOfUdtField;
	}

	do
	{
		if (NextUdtField->BitPosition == 0)
		{
			//
			// BitPosition == 0 announces a new member.
			//

			break;
		}
	} while (++NextUdtField < EndOfUdtField);

	return NextUdtField;
}

template <
	typename MEMBER_DEFINITION_TYPE
>
bool
PDBSymbolVisitor<MEMBER_DEFINITION_TYPE>::Is64BitBasicType(
	const SYMBOL* Symbol
	)
{
	return (Symbol->Tag == SymTagBaseType && Symbol->Size == 8);
}
