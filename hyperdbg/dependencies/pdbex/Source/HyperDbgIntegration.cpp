#include "PDBExtractor.h"
#include "HyperDbgExport.h"

Callback g_MessageHandler     = NULL;
BOOLEAN  g_IsOutputToFile     = FALSE;
BOOLEAN  g_ShowInOffestFormat = FALSE;

#pragma comment(lib, "dbghelp.lib")

int
pdbex_main_impl_export(int argc, char ** argv)
{
    PDBExtractor Instance;

    if (argv[0] == (char *)1)
    {
        g_ShowInOffestFormat = TRUE;
    }
    else
    {
        g_ShowInOffestFormat = FALSE;
    }

    return Instance.Run(argc, argv);
}

void
pdbex_set_logging_method_export(PVOID handler)
{
    g_MessageHandler = (Callback)handler;
}
