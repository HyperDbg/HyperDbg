#pragma once
#include "PDB.h"
#include "PDBSymbolVisitorBase.h"

#include <vector>

class PDBSymbolSorterBase
	: public PDBSymbolVisitorBase
{
	public:
		enum class ImageArchitecture
		{
			None,
			x86,
			x64,
		};

		virtual
		std::vector<const SYMBOL*>&
		GetSortedSymbols() = 0;

		virtual
		ImageArchitecture
		GetImageArchitecture() const = 0;

		virtual
		void
		Clear() = 0;
};
