#include "PDBExtractor.h"
#include "export.h"

#pragma comment(lib, "dbghelp.lib")

int main_impl(int argc, char** argv)
{
	PDBExtractor Instance;
	return Instance.Run(argc, argv);
}

int main(int argc, char** argv)
{
	return main_impl(argc, argv);
}

int
pdbex_main_impl_export(int argc, char ** argv)
{
    PDBExtractor Instance;
    return Instance.Run(argc, argv);
}
