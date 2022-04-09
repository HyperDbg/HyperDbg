#include "PDBHeaderReconstructor.h"
#include "PDBReconstructorBase.h"

#include <iostream>
#include <numeric> // std::accumulate
#include <string>
#include <map>
#include <set>

#include <cassert>

PDBHeaderReconstructor::PDBHeaderReconstructor(
	Settings* VisitorSettings
	)
{
	static Settings DefaultSettings;

	if (VisitorSettings == nullptr)
	{
		VisitorSettings = &DefaultSettings;
	}

	m_Settings = VisitorSettings;
}

void
PDBHeaderReconstructor::Clear()
{
	assert(m_Depth == 0);

	m_AnonymousDataTypeCounter = 0;
	m_PaddingMemberCounter = 0;

	m_UnnamedSymbols.clear();
	m_CorrectedSymbolNames.clear();
	m_VisitedSymbols.clear();
}

const std::string&
PDBHeaderReconstructor::GetCorrectedSymbolName(
	const SYMBOL* Symbol
	) const
{
	if (!m_CorrectedSymbolNames.contains(Symbol))
	{
		//
		// Build corrected name:
		// SymbolPrefix
		//   + "_" (if Microsoft typedefs are enabled)
		//   + unnamed tag (if symbol does not have name)
		//   + symbol name (if symbol does have name)
		//
		// ...and cache the name.
		//

		std::string CorrectedName;

		CorrectedName += m_Settings->SymbolPrefix;

		if (PDB::IsUnnamedSymbol(Symbol))
		{
			if (m_Settings->MicrosoftTypedefs)
			{
				CorrectedName += "_";
			}

			m_UnnamedSymbols.insert(Symbol);

			CorrectedName += m_Settings->UnnamedTypePrefix + std::to_string(m_UnnamedSymbols.size());
		}
		else
		{
			CorrectedName += Symbol->Name;
		}

		CorrectedName += m_Settings->SymbolSuffix;

		m_CorrectedSymbolNames[Symbol] = CorrectedName;
	}

	return m_CorrectedSymbolNames[Symbol];
}

bool
PDBHeaderReconstructor::OnEnumType(
	const SYMBOL* Symbol
	)
{
	std::string CorrectedName = GetCorrectedSymbolName(Symbol);

	bool Expand = ShouldExpand(Symbol);

	MarkAsVisited(Symbol);

	if (!Expand)
	{
		Write("enum %s", CorrectedName.c_str());
	}

	return Expand;
}

void
PDBHeaderReconstructor::OnEnumTypeBegin(
	const SYMBOL* Symbol
	)
{
	std::string CorrectedName = GetCorrectedSymbolName(Symbol);

	//
	// Handle begin of the typedef.
	//

	WriteTypedefBegin(Symbol);

	Write("enum");

	Write(" %s", CorrectedName.c_str());

	Write("\n");

	WriteIndent();
	Write("{\n");

	m_Depth += 1;
}

void
PDBHeaderReconstructor::OnEnumTypeEnd(
	const SYMBOL* Symbol
	)
{
	m_Depth -= 1;

	WriteIndent();
	Write("}");

	//
	// Handle end of the typedef.
	//

	WriteTypedefEnd(Symbol);

	if (m_Depth == 0)
	{
		Write(";\n\n");
	}
}

void
PDBHeaderReconstructor::OnEnumField(
	const SYMBOL_ENUM_FIELD* EnumField
	)
{
	WriteIndent();
	Write("%s = ", EnumField->Name);

	WriteVariant(&EnumField->Value);
	Write(",\n");
}

bool
PDBHeaderReconstructor::OnUdt(
	const SYMBOL* Symbol
	)
{
	bool Expand = ShouldExpand(Symbol);

	MarkAsVisited(Symbol);

	if (!Expand)
	{
		std::string CorrectedName = GetCorrectedSymbolName(Symbol);

		WriteConstAndVolatile(Symbol);

		Write("%s %s", PDB::GetUdtKindString(Symbol->u.Udt.Kind), CorrectedName.c_str());

		//
		// If we're not expanding the type at the root level,
		// OnUdtEnd() won't be called, so print the semicolon here.
		//

		if (m_Depth == 0)
		{
			Write(";\n\n");
		}
	}

	return Expand;
}

void
PDBHeaderReconstructor::OnUdtBegin(
	const SYMBOL* Symbol
	)
{
	//
	// Handle begin of the typedef.
	//

	WriteTypedefBegin(Symbol);

	WriteConstAndVolatile(Symbol);

	Write("%s", PDB::GetUdtKindString(Symbol->u.Udt.Kind));

	if (!PDB::IsUnnamedSymbol(Symbol))
	{
		std::string CorrectedName = GetCorrectedSymbolName(Symbol);
		Write(" %s", CorrectedName.c_str());
	}

	Write("\n");

	WriteIndent();
	Write("{\n");

	m_Depth += 1;
}

void
PDBHeaderReconstructor::OnUdtEnd(
	const SYMBOL* Symbol
	)
{
	m_Depth -= 1;

	WriteIndent();
	Write("}");

	//
	// Handle end of the typedef.
	//

	WriteTypedefEnd(Symbol);

	if (m_Depth == 0)
	{
		Write(";");
	}

	Write(" /* size: 0x%04x */", Symbol->Size);

	if (m_Depth == 0)
	{
		Write("\n\n");
	}
}

void
PDBHeaderReconstructor::OnUdtFieldBegin(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	WriteIndent();

	//
	// Do not show offsets for members which will be expanded.
	//

	if (UdtField->Type->Tag != SymTagUDT ||
	    ShouldExpand(UdtField->Type) == false)
	{
		WriteOffset(UdtField, GetParentOffset());
	}

	AppendToTest(UdtField);

	//
	// Push current offset in case we will be expanding
	// some UDT field.
	//

	m_OffsetStack.push_back(UdtField->Offset);
}

void
PDBHeaderReconstructor::OnUdtFieldEnd(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	//
	// Pop offset of the current UDT field.
	//

	m_OffsetStack.pop_back();
}

void
PDBHeaderReconstructor::OnUdtField(
	const SYMBOL_UDT_FIELD* UdtField,
	UdtFieldDefinitionBase* MemberDefinition
	)
{
	Write("%s", MemberDefinition->GetPrintableDefinition().c_str());

	//
	// BitField handling.
	//

	if (UdtField->Bits != 0)
	{
		Write(" : %i", UdtField->Bits);
	}

	Write(";");

	if (UdtField->Bits != 0)
	{
		Write(" /* bit position: %i */", UdtField->BitPosition);
	}

	Write("\n");
}

void
PDBHeaderReconstructor::OnAnonymousUdtBegin(
	UdtKind Kind,
	const SYMBOL_UDT_FIELD* FirstUdtField
	)
{
	WriteIndent();
	Write("%s\n", PDB::GetUdtKindString(Kind));

	WriteIndent();
	Write("{\n");

	m_Depth += 1;
}

void
PDBHeaderReconstructor::OnAnonymousUdtEnd(
	UdtKind Kind,
	const SYMBOL_UDT_FIELD* FirstUdtField,
	const SYMBOL_UDT_FIELD* LastUdtField,
	DWORD Size
	)
{
	m_Depth -= 1;
	WriteIndent();
	Write("}");

	WriteUnnamedDataType(Kind);

	Write(";");

	Write(" /* size: 0x%04x */", Size);

	Write("\n");
}

void
PDBHeaderReconstructor::OnUdtFieldBitFieldBegin(
	const SYMBOL_UDT_FIELD* FirstUdtFieldBitField,
	const SYMBOL_UDT_FIELD* LastUdtFieldBitField
	)
{
	if (m_Settings->AllowBitFieldsInUnion == false)
	{
		//
		// Do not wrap bitfields which have only one member.
		//
		if (FirstUdtFieldBitField != LastUdtFieldBitField)
		{
			WriteIndent();
			Write("%s /* bitfield */\n", PDB::GetUdtKindString(UdtStruct));

			WriteIndent();
			Write("{\n");

			m_Depth += 1;
		}
	}
}

void
PDBHeaderReconstructor::OnUdtFieldBitFieldEnd(
	const SYMBOL_UDT_FIELD* FirstUdtFieldBitField,
	const SYMBOL_UDT_FIELD* LastUdtFieldBitField
	)
{
	if (m_Settings->AllowBitFieldsInUnion == false)
	{
		if (FirstUdtFieldBitField != LastUdtFieldBitField)
		{
			m_Depth -= 1;

			WriteIndent();
			Write("}; /* bitfield */\n");
		}
	}
}

void
PDBHeaderReconstructor::OnPaddingMember(
	const SYMBOL_UDT_FIELD* UdtField,
	BasicType PaddingBasicType,
	DWORD PaddingBasicTypeSize,
	DWORD PaddingSize
	)
{
	if (m_Settings->CreatePaddingMembers)
	{
		WriteIndent();

		WriteOffset(UdtField, -((int)PaddingSize * (int)PaddingBasicTypeSize));

		Write(
			"%s %s%u",
			PDB::GetBasicTypeString(PaddingBasicType, PaddingBasicTypeSize),
			m_Settings->PaddingMemberPrefix.c_str(),
			m_PaddingMemberCounter++
			);

		if (PaddingSize > 1)
		{
			Write("[%u]", PaddingSize);
		}

		Write(";\n");
	}
}

void
PDBHeaderReconstructor::OnPaddingBitFieldField(
	const SYMBOL_UDT_FIELD* UdtField,
	const SYMBOL_UDT_FIELD* PreviousUdtField
	)
{
	WriteIndent();

	WriteOffset(UdtField, GetParentOffset());

	//
	// Bitfield fields can be unnamed.
	// Check if prefix is specified and if not,
	// simply do not print anything.
	//

	if (m_Settings->BitFieldPaddingMemberPrefix.empty())
	{
		Write(
			"%s",
			PDB::GetBasicTypeString(UdtField->Type) // TODO: UseStdInt
		);
	}
	else
	{
		Write(
			"%s %s%u",
			PDB::GetBasicTypeString(UdtField->Type), // TODO: UseStdInt
			m_Settings->PaddingMemberPrefix.c_str(),
			m_PaddingMemberCounter++
		);
	}

	//
	// BitField handling.
	//

	DWORD Bits = PreviousUdtField
		? UdtField->BitPosition - (PreviousUdtField->BitPosition + PreviousUdtField->Bits)
		: UdtField->BitPosition;

	DWORD BitPosition = PreviousUdtField
		? PreviousUdtField->BitPosition + PreviousUdtField->Bits
		: 0;

	assert(Bits != 0);

	Write(" : %i", Bits);

	Write(";");

	Write(" /* bit position: %i */", BitPosition);

	Write("\n");
}

void
PDBHeaderReconstructor::Write(
	const char* Format,
	...
	)
{
	char TempBuffer[8 * 1024];

	va_list ArgPtr;
	va_start(ArgPtr, Format);
	vsprintf_s(TempBuffer, Format, ArgPtr);
	va_end(ArgPtr);

	m_Settings->OutputFile->write(TempBuffer, strlen(TempBuffer));
}

void
PDBHeaderReconstructor::WriteIndent()
{
	for (DWORD i = 0; i < m_Depth; i++)
	{
		Write("  ");
	}
}

void
PDBHeaderReconstructor::WriteVariant(
	const VARIANT* v
	)
{
	switch (v->vt)
	{
		case VT_I1:
			Write("%d", (INT)v->cVal);
			break;

		case VT_UI1:
			Write("0x%x", (UINT)v->cVal);
			break;

		case VT_I2:
			Write("%d", (UINT)v->iVal);
			break;

		case VT_UI2:
			Write("0x%x", (UINT)v->iVal);
			break;

		case VT_INT:
		case VT_I4:
			Write("%d", (UINT)v->lVal);
			break;

		case VT_UINT:
		case VT_UI4:
			Write("0x%x", (UINT)v->lVal);
			break;
	}
}

void
PDBHeaderReconstructor::WriteUnnamedDataType(
	UdtKind Kind
	)
{
	if (m_Settings->AllowAnonymousDataTypes == false)
	{
		switch (Kind)
		{
			case UdtStruct:
			case UdtClass:
				Write(" %s", m_Settings->AnonymousStructPrefix.c_str());
				break;

			case UdtUnion:
				Write(" %s", m_Settings->AnonymousUnionPrefix.c_str());
				break;

			default:
				assert(0);
				break;
		}

		if (m_AnonymousDataTypeCounter++ > 0)
		{
			Write("%u", m_AnonymousDataTypeCounter);
		}
	}
}

void
PDBHeaderReconstructor::WriteTypedefBegin(
	const SYMBOL* Symbol
	)
{
	std::string CorrectedName = GetCorrectedSymbolName(Symbol);
	bool UseTypedef = m_Settings->MicrosoftTypedefs && CorrectedName[0] == '_';

	if (UseTypedef && m_Depth == 0)
	{
		Write("typedef ");
	}
}

void
PDBHeaderReconstructor::WriteTypedefEnd(
	const SYMBOL* Symbol
	)
{
	std::string CorrectedName = GetCorrectedSymbolName(Symbol);
	bool UseTypedef = m_Settings->MicrosoftTypedefs && CorrectedName[0] == '_';

	if (UseTypedef && m_Depth == 0)
	{
		Write(" %s, *P%s", &CorrectedName[1], &CorrectedName[1]);
	}
}

void
PDBHeaderReconstructor::WriteConstAndVolatile(
	const SYMBOL* Symbol
	)
{
	if (m_Depth != 0)
	{
		//
		// Allow only non-root UDTs to be "const" and/or "volatile".
		//

		if (Symbol->IsConst)
		{
			Write("const ");
		}

		if (Symbol->IsVolatile)
		{
			Write("volatile ");
		}
	}
}

void
PDBHeaderReconstructor::WriteOffset(
	const SYMBOL_UDT_FIELD* UdtField,
	int PaddingOffset
	)
{
	if (m_Settings->ShowOffsets)
	{
		Write("/* 0x%04x */ ", UdtField->Offset + PaddingOffset);
	}
}

bool
PDBHeaderReconstructor::HasBeenVisited(
	const SYMBOL* Symbol
	) const
{
	std::string CorrectedName = GetCorrectedSymbolName(Symbol);
	return m_VisitedSymbols.find(CorrectedName) != m_VisitedSymbols.end();
}

void
PDBHeaderReconstructor::MarkAsVisited(
	const SYMBOL* Symbol
	)
{
	std::string CorrectedName = GetCorrectedSymbolName(Symbol);
	m_VisitedSymbols.insert(CorrectedName);
}

DWORD
PDBHeaderReconstructor::GetParentOffset() const
{
	return std::accumulate(m_OffsetStack.begin(), m_OffsetStack.end(), (DWORD)0);
}

void
PDBHeaderReconstructor::AppendToTest(
	const SYMBOL_UDT_FIELD* UdtField
	)
{
	//
	// Test of the current member is produced when:
	//   - Test file was specified.
	//   - We're in the root UDT.
	//   - This field is not a part of the bitfield.
	//

	if (m_Settings->TestFile != nullptr &&
	    m_OffsetStack.empty() &&
	    UdtField->Bits == 0)
	{
		//
		// Line of the test:
		//
		// printf(
		//   \"[%%c] 0x%%04x - 0x%04x (%s %s.%s)\\n\",
		//   offsetof(%s %s, %s) == %u ? ' ' : '!',
		//   (unsigned)offsetof(%s %s, %s)
		// );
		//
		//
		// Example:
		//
		// printf(
		//   "[%c] 0x%04x - 0x000c (struct _DEVICE_OBJECT.NextDevice)\n",
		//   offsetof(struct _DEVICE_OBJECT, NextDevice) == 12 ? ' ' : '!',
		//   (unsigned)offsetof(struct _DEVICE_OBJECT, NextDevice)
		// );
		//
		static const char TestFormatString[] =
			"\t"
			"printf("
				"\"[%%c] 0x%%04x - 0x%04x (%s %s.%s)\\n\", "
				"offsetof(%s %s, %s) == %u ? ' ' : '!', "
				"(unsigned)offsetof(%s %s, %s)"
			");";

		//
		// Delimiter for the structs.
		//
		// printf("\n");
		//
		static const char TestDelimiterString[] =
			"\t"
			"printf("
			"\"\\n\""
			");";

		//
		// Holds corrected name of a last symbol
		// which test was produced for.
		// This is used for delimiting tests
		// with extra new line.
		//
		static std::string LastTestedUdt;

		std::string CorrectedSymbolName = GetCorrectedSymbolName(UdtField->Parent);

		//
		// If the current member is part of a different UDT
		// than the previous one, delimit the output of the test
		// by extra new line.
		//
		if (CorrectedSymbolName != LastTestedUdt)
		{
			(*m_Settings->TestFile) << TestDelimiterString << std::endl;
		}

		LastTestedUdt = CorrectedSymbolName;

		//
		// Build the line for the test.
		//
		static char FormattedStringBuffer[4096];
		sprintf_s(
			FormattedStringBuffer,
			TestFormatString,
			UdtField->Offset,

			PDB::GetUdtKindString(UdtField->Parent->u.Udt.Kind),
			CorrectedSymbolName.c_str(),
			UdtField->Name,

			PDB::GetUdtKindString(UdtField->Parent->u.Udt.Kind),
			CorrectedSymbolName.c_str(),
			UdtField->Name,
			UdtField->Offset,

			PDB::GetUdtKindString(UdtField->Parent->u.Udt.Kind),
			CorrectedSymbolName.c_str(),
			UdtField->Name
			);

		(*m_Settings->TestFile) << FormattedStringBuffer << std::endl;
	}
}

bool
PDBHeaderReconstructor::ShouldExpand(
	const SYMBOL* Symbol
	) const
{
	bool Expand = false;

	switch (m_Settings->MemberStructExpansion)
	{
		default:
		case MemberStructExpansionType::None:
			Expand = m_Depth == 0;
			break;

		case MemberStructExpansionType::InlineUnnamed:
			Expand = m_Depth == 0 || (Symbol->Tag == SymTagUDT && PDB::IsUnnamedSymbol(Symbol));
			break;

		case MemberStructExpansionType::InlineAll:
			Expand = !HasBeenVisited(Symbol);
			break;
	}

	return Expand && Symbol->Size > 0;
}
