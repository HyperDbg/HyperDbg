#pragma once
#include "UdtFieldDefinitionBase.h"

#include <string>

class UdtFieldDefinition
	: public UdtFieldDefinitionBase
{
	public:
		struct Settings
		{
			bool UseStdInt = false;
		};

		void
		VisitBaseType(
			const SYMBOL* Symbol
			) override
		{
			//
			// BaseType:
			// short/int/long/...
			//

			if (Symbol->BaseType == btFloat && Symbol->Size == 10)
			{
				m_Comment += " /* 80-bit float */";
			}

			if (Symbol->IsConst)
			{
				m_TypePrefix += "const ";
			}

			if (Symbol->IsVolatile)
			{
				m_TypePrefix += "volatile ";
			}

			//
			// If this returns null, it probably means BasicTypeMapMSVC and/or BasicTypeMapStdInt are out of date compared to MS DIA.
			// Output the (non-compilable) type "<unknown_type>" instead of crashing.
			//

			const CHAR* BasicTypeString = PDB::GetBasicTypeString(Symbol, m_Settings->UseStdInt);
			m_TypePrefix += (BasicTypeString != nullptr ? BasicTypeString : "<unknown_type>");
		}

		void
		VisitPointerTypeEnd(
			const SYMBOL* Symbol
			) override
		{
			if (Symbol->u.Pointer.IsReference)
			{
				m_TypePrefix += "&";
			}
			else
			{
				m_TypePrefix += "*";
			}

			if (Symbol->IsConst)
			{
				m_TypePrefix += " const";
			}

			if (Symbol->IsVolatile)
			{
				m_TypePrefix += " volatile";
			}
		}

		void
		VisitArrayTypeEnd(
			const SYMBOL* Symbol
			) override
		{
			//
			// To my knowledge, array elements can't be declared like that.
			//
			//	if (Symbol->IsConst)
			//	{
			//		m_TypePrefix += " const";
			//	}
			//
			//	if (Symbol->IsVolatile)
			//	{
			//		m_TypePrefix += " volatile";
			//	}
			//

			if (Symbol->u.Array.ElementCount == 0)
			{
				//
				// Apparently array with 0 element count can exist in PDB.
				// But XYZ Name[0] is not compilable.
				// This hack "converts" the zero-sized array into the pointer.
				//
				// Also, size of the symbol is set to 1 instead of 0,
				// otherwise we would end up in anonymous union.
				//

				const_cast<SYMBOL*>(Symbol)->Size = 1;
				m_TypePrefix += "*";

				m_Comment += " /* zero-length array */";
			}
			else
			{
				m_TypeSuffix += "[" + std::to_string(Symbol->u.Array.ElementCount) + "]";
			}
		}

		void
		VisitFunctionTypeEnd(
			const SYMBOL* Symbol
			) override
		{
			//
			// #TODO:
			// Currently, show void* instead of functions.
			//

			m_TypePrefix += "void";

			m_Comment += " /* function */";
		}

		void
		SetMemberName(
			const CHAR* MemberName
			) override
		{
			m_MemberName = MemberName ? MemberName : std::string();
		}

		std::string
		GetPrintableDefinition() const override
		{
			return m_TypePrefix + " " + m_MemberName + m_TypeSuffix + m_Comment;
		}

		void
		SetSettings(
			void* MemberDefinitionSettings
			) override
		{
			static Settings DefaultSettings;

			if (MemberDefinitionSettings == nullptr)
			{
				MemberDefinitionSettings = &DefaultSettings;
			}

			m_Settings = static_cast<Settings*>(MemberDefinitionSettings);
		}

		virtual
		void*
		GetSettings() override
		{
			return &m_Settings;
		}

	private:
		std::string m_TypePrefix; // "int*"
		std::string m_MemberName; // "XYZ"
		std::string m_TypeSuffix; // "[8]"
		std::string m_Comment;

		Settings* m_Settings;
};
