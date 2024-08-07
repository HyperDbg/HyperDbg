#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <string>

#include <dia2.h>

#include <set>
#include <unordered_set>
#include <unordered_map>

typedef struct _SYMBOL SYMBOL, *PSYMBOL;

//
// Representation of the enum field.
//
// enum XYZ
// {
//   // Name       Value
//   // -----------------
//      XYZ_1   =   2,
//      XYZ_2   =   4,
// };
//
// Note that 'Value' is represented as the VARIANT type.
//
typedef struct _SYMBOL_ENUM_FIELD
{
	//
	// Name of the enumeration field.
	//
	CHAR*                Name;

	//
	// Assigned value of the enumeration field.
	//
	VARIANT              Value;

	//
	// Parent enumeration.
	//
	SYMBOL*              Parent;

} SYMBOL_ENUM_FIELD, *PSYMBOL_ENUM_FIELD;

//
// Representation of the struct/class/union field.
//
// struct XYZ
// {
//    // Type     Name    Bits    Offset     BitPosition
//    // -------------------------------------------------
//       int     XYZ_1;        //  0            0
//       short   XYZ_2  :  3;  //  4            0
//       short   XYZ_3  :  13; //  4            3
// };
//
typedef struct _SYMBOL_UDT_FIELD
{
	//
	// Name of the UDT field.
	//
	CHAR*                Name;

	//
	// Type of the field.
	//
	SYMBOL*              Type;

	//
	// Offset from the start of the structure/class/union.
	//
	DWORD                Offset;

	//
	// Amount of bits this field takes.
	// If this value is 0, this field takes
	// all of the space of the field type (Type->Size bytes).
	//
	DWORD                Bits;

	//
	// Which bit this fields starts at (relative to the Offset).
	//
	DWORD                BitPosition;

	//
	// Parent UDT symbol.
	//
	SYMBOL*              Parent;

} SYMBOL_UDT_FIELD, *PSYMBOL_UDT_FIELD;

//
// Representation of the enumeration.
//
// Example for FieldCount = 3
//
// enum XYZ
// {
//   XYZ_1,
//   XYZ_2,
//   XYZ_3,
// };
//
typedef struct _SYMBOL_ENUM
{
	//
	// Count of fields in the enumeration.
	//
	DWORD                FieldCount;

	//
	// Pointer to the continuous array of structures of the enumeration fields.
	//
	SYMBOL_ENUM_FIELD*   Fields;

} SYMBOL_ENUM, *PSYMBOL_ENUM;

//
// Representation of the typedef statement.
//
typedef struct _SYMBOL_TYPEDEF
{
	//
	// Underlying type of the type definition.
	//
	SYMBOL*              Type;

} SYMBOL_TYPEDEF, *PSYMBOL_TYPEDEF;

//
// Representation of the pointer statement.
//
typedef struct _SYMBOL_POINTER
{
	//
	// Underlying type of the pointer definition.
	//
	SYMBOL*              Type;

	//
	// Specifies if the pointer represents the reference.
	//
	BOOL                 IsReference;

} SYMBOL_POINTER, *PSYMBOL_POINTER;

//
// Representation of the array.
//
typedef struct _SYMBOL_ARRAY
{
	//
	// Type of the array element.
	//
	SYMBOL*              ElementType;

	//
	// Array size in elements.
	//
	DWORD                ElementCount;

} SYMBOL_ARRAY, *PSYMBOL_ARRAY;

//
// Representation of the function.
//
typedef struct _SYMBOL_FUNCTION
{
	//
	// Return type of the function.
	//
	SYMBOL*              ReturnType;

	//
	// Calling convention of the function.
	//
	CV_call_e            CallingConvention;

	//
	// Number of arguments.
	//
	DWORD                ArgumentCount;

	//
	// Pointer to the continuous array of pointers to the symbol structure for arguments.
	// These symbols are of type SYMBOL_FUNCTIONARG and has tag SymTagFunctionArgType.
	//
	SYMBOL**             Arguments;

} SYMBOL_FUNCTION, *PSYMBOL_FUNCTION;

//
// Representation of the function argument type.
//
typedef struct _SYMBOL_FUNCTIONARG
{
	//
	// Underlying type of the argument.
	//
	SYMBOL*              Type;

} SYMBOL_FUNCTIONARG, *PSYMBOL_FUNCTIONARG;

//
// Representation of the UDT (struct/class/union).
//
typedef struct _SYMBOL_UDT
{
	//
	// Kind of the UDT.
	// It may be either UdtStruct, UdtClass or UdtUnion.
	//
	UdtKind              Kind;

	//
	// Number of fields (members) in the UDT.
	//
	DWORD                FieldCount;

	//
	// Pointer to the continuous array of structures of the UDT.
	//
	SYMBOL_UDT_FIELD*    Fields;

} SYMBOL_UDT, *PSYMBOL_UDT;

//
// Representation of the debug symbol.
//
struct _SYMBOL
{
	//
	// Type of the symbol.
	//
	enum SymTagEnum      Tag;

	//
	// Data kind.
	// Only sef it Tag == SymTagData.
	//
	enum DataKind        DataKind;

	//
	// Base type.
	// Only set if Tag == SymTagBaseType.
	//
	BasicType            BaseType;

	//
	// Internal ID of the type.
	//
	DWORD                TypeId;

	//
	// Total size of the type which this symbol represents.
	//
	DWORD                Size;

	//
	// Specifies constness.
	//
	BOOL                 IsConst;

	//
	// Specifies volatileness.
	//
	BOOL                 IsVolatile;

	//
	// Name of the type.
	//
	CHAR*                Name;

	union
	{
		SYMBOL_ENUM        Enum;
		SYMBOL_TYPEDEF     Typedef;
		SYMBOL_POINTER     Pointer;
		SYMBOL_ARRAY       Array;
		SYMBOL_FUNCTION    Function;
		SYMBOL_FUNCTIONARG FunctionArg;
		SYMBOL_UDT         Udt;
	} u;
};

class SymbolModule;

using SymbolMap     = std::unordered_map<DWORD, SYMBOL*>;
using SymbolNameMap = std::unordered_map<std::string, SYMBOL*>;
using SymbolSet     = std::unordered_set<SYMBOL*>;
using FunctionSet   = std::set<std::string>;

class PDB
{
	public:
		//
		// Default constructor.
		//
		PDB();

		//
		// Instantiates PDB class with particular PDB file.
		//
		PDB(
			IN const CHAR* Path
			);

		//
		// Destructor.
		//
		~PDB();

		//
		// Opens particular PDB file and parses it.
		//
		// Returns non-zero value on success.
		//
		BOOL
		Open(
			IN const CHAR* Path
			);

		//
		// Returns TRUE if a PDB file is opened.
		//
		BOOL
		IsOpened() const;

		//
		// Returns path of the currently opened PDB file.
		//
		const CHAR*
		GetPath() const;

		//
		// Closes all resources which holds the opened PDB file.
		//
		VOID
		Close();

		//
		// Get machine type for which was the PDB compiled for.
		//
		DWORD
		GetMachineType() const;

		//
		// Get language type of the global symbol.
		//
		CV_CFL_LANG
		GetLanguage() const;

		//
		// Returns a SYMBOL structure of particular name.
		//
		// Returns non-NULL value on success.
		//
		const SYMBOL*
		GetSymbolByName(
			IN const CHAR* SymbolName
			);

		//
		// Returns a SYMBOL structure of particular Type ID.
		//
		// Returns non-NULL value on success.
		//
		const SYMBOL*
		GetSymbolByTypeId(
			IN DWORD TypeId
			);

		//
		// Returns collection of all symbols.
		//
		const SymbolMap&
		GetSymbolMap() const;

		//
		// Returns collection of all named symbols.
		//
		const SymbolNameMap&
		GetSymbolNameMap() const;

		//
		// Returns collection of all named functions.
		//
		const FunctionSet&
		GetFunctionSet() const;

		//
		// Returns C-like name of the type of provided symbol.
		// The symbol must be BaseType.
		//
		// Returns non-NULL value on success.
		//
		static
		const CHAR*
		GetBasicTypeString(
			IN BasicType BaseType,
			IN DWORD Size,
			IN BOOL UseStdInt = FALSE
			);

		//
		// Returns C-like name of the type of provided symbol.
		// The symbol must be BaseType.
		//
		// Returns non-NULL value on success.
		//
		static
		const CHAR*
		GetBasicTypeString(
			IN const SYMBOL* Symbol,
			IN BOOL UseStdInt = FALSE
			);

		//
		// Returns string representing the kind
		// of provided user defined type.
		//
		// Returns non-NULL value on success.
		//
		static
		const CHAR*
		GetUdtKindString(
			IN UdtKind Kind
			);

		//
		// Returns TRUE if the provided symbol's name
		// starts with "<anonymous-", "<unnamed-" or "__unnamed".
		//
		static
		BOOL
		IsUnnamedSymbol(
			const SYMBOL* Symbol
			);

	private:
		SymbolModule* m_Impl;
};
