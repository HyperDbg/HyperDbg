#pragma once
#include "PDB.h"

class PDBSymbolVisitorBase
{
	public:
		virtual
		~PDBSymbolVisitorBase() = default;

		virtual
		void
		Visit(
			const SYMBOL* Symbol
			)
		{
			switch (Symbol->Tag)
			{
				case SymTagBaseType:
					VisitBaseType(Symbol);
					break;

				case SymTagEnum:
					VisitEnumType(Symbol);
					break;

				case SymTagTypedef:
					VisitTypedefType(Symbol);
					break;

				case SymTagPointerType:
					VisitPointerType(Symbol);
					break;

				case SymTagArrayType:
					VisitArrayType(Symbol);
					break;

				case SymTagFunctionType:
					VisitFunctionType(Symbol);
					break;

				case SymTagFunctionArgType:
					VisitFunctionArgType(Symbol);
					break;

				case SymTagUDT:
					VisitUdt(Symbol);
					break;

				default:
					VisitOtherType(Symbol);
					break;
			}
		}

	protected:
		virtual
		void
		VisitBaseType(
			const SYMBOL* Symbol
			)
		{

		}

		virtual
		void
		VisitEnumType(
			const SYMBOL* Symbol
			)
		{
			for (DWORD i = 0; i < Symbol->u.Enum.FieldCount; i++)
			{
				VisitEnumField(&Symbol->u.Enum.Fields[i]);
			}
		}

		virtual
		void
		VisitTypedefType(
			const SYMBOL* Symbol
			)
		{
			Visit(Symbol->u.Typedef.Type);
		}

		virtual
		void
		VisitPointerType(
			const SYMBOL* Symbol
			)
		{
			Visit(Symbol->u.Pointer.Type);
		}

		virtual
		void
		VisitArrayType(
			const SYMBOL* Symbol
			)
		{
			Visit(Symbol->u.Array.ElementType);
		}

		virtual
		void
		VisitFunctionType(
			const SYMBOL* Symbol
			)
		{
			for (DWORD i = 0; i < Symbol->u.Function.ArgumentCount; i++)
			{
				Visit(Symbol->u.Function.Arguments[i]);
			}
		}

		virtual
		void
		VisitFunctionArgType(
			const SYMBOL* Symbol
			)
		{
			Visit(Symbol->u.FunctionArg.Type);
		}

		virtual
		void
		VisitUdt(
			const SYMBOL* Symbol
			)
		{
			const SYMBOL_UDT_FIELD* UdtField;
			const SYMBOL_UDT_FIELD* EndOfUdtField;

			if (Symbol->u.Udt.FieldCount == 0)
			{
				//
				// Early return on empty UDTs.
				//

				return;
			}

			UdtField = Symbol->u.Udt.Fields;
			EndOfUdtField = &Symbol->u.Udt.Fields[Symbol->u.Udt.FieldCount];

			do
			{
				if (UdtField->Bits == 0)
				{
					//
					// Non-bitfield member.
					//
					VisitUdtFieldBegin(UdtField);
					VisitUdtField(UdtField);
					VisitUdtFieldEnd(UdtField);
				}
				else
				{
					//
					// UdtField now points to the first member of the bitfield.
					//
					VisitUdtFieldBitFieldBegin(UdtField);

					do
					{
						//
						// Visit all bitfield members
						//
						VisitUdtFieldBitField(UdtField);
					} while (++UdtField < EndOfUdtField &&
					           UdtField->BitPosition != 0);

					//
					// UdtField now points behind the last bitfield member.
					// So decrement the iterator and call VisitUdtFieldBitFieldEnd().
					//
					VisitUdtFieldBitFieldEnd(--UdtField);
				}
			} while (++UdtField < EndOfUdtField);
		}

		virtual
		void
		VisitOtherType(
			const SYMBOL* Symbol
			)
		{

		}

		virtual
		void
		VisitEnumField(
			const SYMBOL_ENUM_FIELD* EnumField
			)
		{

		}

		virtual
		void
		VisitUdtFieldBegin(
			const SYMBOL_UDT_FIELD* UdtField
			)
		{

		}

		virtual
		void
		VisitUdtFieldEnd(
			const SYMBOL_UDT_FIELD* UdtField
			)
		{

		}

		virtual
		void
		VisitUdtField(
			const SYMBOL_UDT_FIELD* UdtField
			)
		{

		}

		virtual
		void
		VisitUdtFieldBitFieldBegin(
			const SYMBOL_UDT_FIELD* UdtField
			)
		{

		}

		virtual
		void
		VisitUdtFieldBitFieldEnd(
			const SYMBOL_UDT_FIELD* UdtField
			)
		{

		}

		virtual
		void
		VisitUdtFieldBitField(
			const SYMBOL_UDT_FIELD* UdtField
			)
		{
			//
			// Call VisitUdtField by default.
			//

			VisitUdtField(UdtField);
		}
};
