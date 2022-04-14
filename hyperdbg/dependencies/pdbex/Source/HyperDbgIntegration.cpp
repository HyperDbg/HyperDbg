#include "PDBExtractor.h"
#include "HyperDbgExport.h"

Callback g_MessageHandler       = NULL;
BOOLEAN  g_IsOutputToFile       = FALSE;
BOOLEAN  g_ShowInOffestFormat   = FALSE;
CHAR *   g_MappingBufferAddress = FALSE;

#pragma comment(lib, "dbghelp.lib")

int
pdbex_export(int     argc,
             char ** argv,
             bool    is_struct,
             void *  buffer_address)
{
    PDBExtractor Instance;

    if (!is_struct)
    {
        g_ShowInOffestFormat = TRUE;
    }
    else
    {
        g_ShowInOffestFormat = FALSE;
    }

    //
    // Set the buffer address for mapping data
    //
    g_MappingBufferAddress = (CHAR *)buffer_address;

	//
	// Temp setting buffer, should be removed
	//
    g_MappingBufferAddress = (CHAR *)malloc(0x6000);
    memset(g_MappingBufferAddress, 0x55555555, 0x6000);

    return Instance.Run(argc, argv);
}

void
pdbex_set_logging_method_export(PVOID handler)
{
    g_MessageHandler = (Callback)handler;
}

UINT64
ExtractBits(UINT64 Orig64Bit, unsigned int From, unsigned int To)
{
    UINT64 Mask = ((1 << (To - From + 1)) - 1) << From;
    return (Orig64Bit & Mask) >> From;
}
