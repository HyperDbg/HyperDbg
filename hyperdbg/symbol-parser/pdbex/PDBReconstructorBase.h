#pragma once

#include "PDB.h"
#include "UdtFieldDefinitionBase.h"

class PDBReconstructorBase
{
	public:
		//
		// Called when reached the 'enum' type.
		// If the return value is true, the enum will be expanded.
		//
		virtual
		bool
		OnEnumType(
			const SYMBOL* Symbol
			)
		{
			return false;
		}

		//
		// Called when entering into the 'enum' type
		// which will be expanded.
		//
		virtual
		void
		OnEnumTypeBegin(
			const SYMBOL* Symbol
			)
		{

		}

		//
		// Called when leaving from the 'enum' type.
		//
		virtual
		void
		OnEnumTypeEnd(
			const SYMBOL* Symbol
			)
		{

		}

		//
		// Called for each field of the current 'enum' type.
		//
		virtual
		void
		OnEnumField(
			const SYMBOL_ENUM_FIELD* EnumField
			)
		{

		}

		//
		// Called when reached the UDT (struct/class/union)
		// If the return value is true, the UDT will be expanded.
		//
		virtual
		bool
		OnUdt(
			const SYMBOL* Symbol
			)
		{
			return false;
		}

		//
		// Called when entering into the UDT (struct/class/union)
		// which will be expanded.
		//
		virtual
		void
		OnUdtBegin(
			const SYMBOL* Symbol
			)
		{

		}

		//
		// Called when leaving from the current UDT.
		//
		virtual
		void
		OnUdtEnd(
			const SYMBOL* Symbol
			)
		{

		}

		//
		// Called when entering into the field of the current UDT.
		//
		virtual
		void
		OnUdtFieldBegin(
			const SYMBOL_UDT_FIELD* UdtField
			)
		{

		}

		//
		// Called when leaving from the field of the current UDT.
		//
		virtual
		void
		OnUdtFieldEnd(
			const SYMBOL_UDT_FIELD* UdtField
			)
		{

		}

		//
		// Called for each field in the current UDT.
		//
		virtual
		void
		OnUdtField(
			const SYMBOL_UDT_FIELD* UdtField,
			UdtFieldDefinitionBase* MemberDefinition
			)
		{

		}

		//
		// Called when entering into the nested anonymous UDT (struct/class/union)
		// which will be expanded.
		//
		virtual
		void
		OnAnonymousUdtBegin(
			UdtKind Kind,
			const SYMBOL_UDT_FIELD* FirstUdtField
			)
		{

		}

		//
		// Called when leaving from the current nested anonymous UDT.
		//
		virtual
		void
		OnAnonymousUdtEnd(
			UdtKind Kind,
			const SYMBOL_UDT_FIELD* FirstUdtField,
			const SYMBOL_UDT_FIELD* LastUdtField,
			DWORD Size
			)
		{

		}

		//
		// Called when entering the bitfield.
		//
		virtual
		void
		OnUdtFieldBitFieldBegin(
			const SYMBOL_UDT_FIELD* FirstUdtFieldBitField,
			const SYMBOL_UDT_FIELD* LastUdtFieldBitField
			)
		{

		}

		//
		// Called when leaving the bitfield.
		//
		virtual
		void
		OnUdtFieldBitFieldEnd(
			const SYMBOL_UDT_FIELD* FirstUdtFieldBitField,
			const SYMBOL_UDT_FIELD* LastUdtFieldBitField
			)
		{

		}

		//
		// Called when a padding member should be created.
		//
		virtual
		void
		OnPaddingMember(
			const SYMBOL_UDT_FIELD* UdtField,
			BasicType PaddingBasicType,
			DWORD PaddingBasicTypeSize,
			DWORD PaddingSize
			)
		{

		}

		//
		// Called when a padding bitfield field should be created.
		//
		virtual
		void
		OnPaddingBitFieldField(
			const SYMBOL_UDT_FIELD* UdtField,
			const SYMBOL_UDT_FIELD* PreviousUdtField
			)
		{

		}
};
