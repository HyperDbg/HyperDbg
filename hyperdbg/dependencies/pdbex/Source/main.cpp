#include "PDBExtractor.h"

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
