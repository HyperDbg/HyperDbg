#include "PDBExtractor.h"
#include "HyperDbgExport.h"
#include <sstream>
#include <iomanip>

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

    return Instance.Run(argc, argv);
}

void
pdbex_set_logging_method_export(PVOID handler)
{
    g_MessageHandler = (Callback)handler;
}

UINT64
ExtractBits(UINT64 Orig64Bit, UINT64 From, UINT64 To)
{
    UINT64 Mask = ((1ull << (To - From + 1ull)) - 1ull) << From;
    return (Orig64Bit & Mask) >> From;
}

std::string
SymSeparateTo64BitValue(UINT64 Value)
{
    std::ostringstream OstringStream;
    std::string        Temp;

    OstringStream << std::setw(16) << std::setfill('0') << std::hex << Value;
    Temp = OstringStream.str();

    Temp.insert(8, 1, '`');
    return Temp;
}
