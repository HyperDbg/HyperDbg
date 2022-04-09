#pragma once
#include "PDBSymbolSorterBase.h"

#include <cassert>
#include <string>
#include <vector>
#include <map>

class PDBSymbolSorter
	: public PDBSymbolSorterBase
{
	public:
		std::vector<const SYMBOL*>&
		GetSortedSymbols() override
		{
			return m_SortedSymbols;
		}

		ImageArchitecture
		GetImageArchitecture() const override
		{
			return m_Architecture;
		}

		void
		Clear() override
		{
			ImageArchitecture m_Architecture = ImageArchitecture::None;

			m_VisitedUdts.clear();
			m_SortedSymbols.clear();
		}

	protected:
		void
		VisitEnumType(
			const SYMBOL* Symbol
			) override
		{
			if (HasBeenVisited(Symbol)) return;

			AddSymbol(Symbol);
		}

		void
		VisitPointerType(
			const SYMBOL* Symbol
			) override
		{
			if (m_Architecture == ImageArchitecture::None)
			{
				switch (Symbol->Size)
				{
					case 4:
						m_Architecture = ImageArchitecture::x86;
						break;

					case 8:
						m_Architecture = ImageArchitecture::x64;
						break;

					default:
						assert(0);
						break;
				}
			}
		}

		void
		VisitUdt(
			const SYMBOL* Symbol
			) override
		{
			if (HasBeenVisited(Symbol)) return;

			PDBSymbolVisitorBase::VisitUdt(Symbol);

			AddSymbol(Symbol);
		}

		void
		VisitUdtField(
			const SYMBOL_UDT_FIELD* UdtField
			) override
		{
			Visit(UdtField->Type);
		}

	private:
		bool
		HasBeenVisited(
			const SYMBOL* Symbol
			)
		{
			static DWORD UnnamedCounter = 0;

			//
			// In one PDB there can be more than one symbol
			// with same name (and different definitions),
			// which would result into redefinitions of types
			// during the printing.
			//
			// Problem is solved by taking into account
			// and printing only the first definition of the symbol.
			//
			// Another solution could be appending a suffix (_1, _2, ...)
			// to the symbol names, but then it wouldn't reflect the real names.
			// So let's just assume all definitions are same
			// and/or the first one is the most correct one.
			//
			// Also, unnamed symbols must be handled as a special case.
			//

			std::string Key = Symbol->Name;
			if (m_VisitedUdts.find(Key) != m_VisitedUdts.end())
			{
				return true;
			}
			else
			{
				if (PDB::IsUnnamedSymbol(Symbol))
				{
					Key += std::to_string(++UnnamedCounter);
				}

				m_VisitedUdts[Key] = Symbol;
				return false;
			}
		}

		void
		AddSymbol(
			const SYMBOL* Symbol
			)
		{
			if (std::find(m_SortedSymbols.begin(), m_SortedSymbols.end(), Symbol) == m_SortedSymbols.end())
			{
				m_SortedSymbols.push_back(Symbol);
			}
		}

		ImageArchitecture m_Architecture = ImageArchitecture::None;

		std::map<std::string, const SYMBOL*> m_VisitedUdts;
		std::vector<const SYMBOL*> m_SortedSymbols;
};
