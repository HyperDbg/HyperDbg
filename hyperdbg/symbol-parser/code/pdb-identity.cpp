/**
 * @file pdb-identity.cpp
 * @author jtaw5649
 * @brief Internal PDB identity formatting helpers
 * @details
 * @version 0.1
 * @date 2026-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#include <strsafe.h>

#include "header/pdb-identity.h"

BOOLEAN
SymFormatPdbIdentity(const CHAR * PdbFile,
                     const GUID * Guid,
                     DWORD        Age,
                     CHAR *       SymbolServerRelativePath,
                     SIZE_T       SymbolServerRelativePathSize,
                     CHAR *       GuidAndAgeDetails,
                     SIZE_T       GuidAndAgeDetailsSize)
{
    const CHAR * FormatStrSymbolServerRelativePath = "%s/%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x%x/%s";
    const CHAR * FormatStrGuidAndAgeDetails        = "%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x%x";

    if (PdbFile == NULL || Guid == NULL || (SymbolServerRelativePath == NULL && GuidAndAgeDetails == NULL))
    {
        return FALSE;
    }

    if (SymbolServerRelativePath != NULL)
    {
        HRESULT Result = StringCchPrintfA(SymbolServerRelativePath,
                                          SymbolServerRelativePathSize,
                                          FormatStrSymbolServerRelativePath,
                                          PdbFile,
                                          Guid->Data1,
                                          Guid->Data2,
                                          Guid->Data3,
                                          Guid->Data4[0],
                                          Guid->Data4[1],
                                          Guid->Data4[2],
                                          Guid->Data4[3],
                                          Guid->Data4[4],
                                          Guid->Data4[5],
                                          Guid->Data4[6],
                                          Guid->Data4[7],
                                          Age,
                                          PdbFile);

        if (FAILED(Result))
        {
            return FALSE;
        }
    }

    if (GuidAndAgeDetails != NULL)
    {
        HRESULT Result = StringCchPrintfA(GuidAndAgeDetails,
                                          GuidAndAgeDetailsSize,
                                          FormatStrGuidAndAgeDetails,
                                          Guid->Data1,
                                          Guid->Data2,
                                          Guid->Data3,
                                          Guid->Data4[0],
                                          Guid->Data4[1],
                                          Guid->Data4[2],
                                          Guid->Data4[3],
                                          Guid->Data4[4],
                                          Guid->Data4[5],
                                          Guid->Data4[6],
                                          Guid->Data4[7],
                                          Age);

        if (FAILED(Result))
        {
            return FALSE;
        }
    }

    return TRUE;
}
