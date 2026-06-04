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
#include <vector>

#include "header/codeview-rsds.h"
#include "header/pdb-identity.h"

static VOID
SymClearOutputBuffer(CHAR * Buffer, SIZE_T BufferSize)
{
    if (Buffer != NULL && BufferSize != 0)
    {
        Buffer[0] = '\0';
    }
}

static BOOLEAN
SymOutputBufferHasSpace(const CHAR * Buffer, SIZE_T BufferSize)
{
    return Buffer == NULL || BufferSize != 0;
}

typedef BOOLEAN (*PSYM_PDB_IDENTITY_EXTRACTOR_CALLBACK)(const BYTE * PeImageBytes,
                                                        SIZE_T       PeImageSize,
                                                        CHAR *       PdbFile,
                                                        SIZE_T       PdbFileSize,
                                                        GUID *       Guid,
                                                        DWORD *      Age);

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

static BOOLEAN
SymFormatPdbIdentityFromExtractorOrFallback(const BYTE *                         PeImageBytes,
                                            SIZE_T                               PeImageSize,
                                            CHAR *                               SymbolServerRelativePath,
                                            SIZE_T                               SymbolServerRelativePathSize,
                                            CHAR *                               PdbFilePath,
                                            SIZE_T                               PdbFilePathSize,
                                            CHAR *                               GuidAndAgeDetails,
                                            SIZE_T                               GuidAndAgeDetailsSize,
                                            PSYM_PDB_IDENTITY_EXTRACTOR_CALLBACK ExtractorCallback,
                                            PSYM_PDB_IDENTITY_FALLBACK_CALLBACK  FallbackCallback,
                                            PVOID                                FallbackContext)
{
    CHAR              PdbFile[MAX_PATH] = {0};
    GUID              Guid              = {0};
    DWORD             Age               = 0;
    std::vector<CHAR> SymbolServerRelativePathBuffer;
    std::vector<CHAR> PdbFilePathBuffer;
    std::vector<CHAR> GuidAndAgeDetailsBuffer;

    SymClearOutputBuffer(SymbolServerRelativePath, SymbolServerRelativePathSize);
    SymClearOutputBuffer(PdbFilePath, PdbFilePathSize);
    SymClearOutputBuffer(GuidAndAgeDetails, GuidAndAgeDetailsSize);

    if (!SymOutputBufferHasSpace(SymbolServerRelativePath, SymbolServerRelativePathSize) ||
        !SymOutputBufferHasSpace(PdbFilePath, PdbFilePathSize) ||
        !SymOutputBufferHasSpace(GuidAndAgeDetails, GuidAndAgeDetailsSize))
    {
        return FALSE;
    }

    if (SymbolServerRelativePath == NULL && PdbFilePath == NULL && GuidAndAgeDetails == NULL)
    {
        return FALSE;
    }

    if (PeImageBytes == NULL || ExtractorCallback == NULL ||
        !ExtractorCallback(PeImageBytes, PeImageSize, PdbFile, sizeof(PdbFile), &Guid, &Age))
    {
        if (FallbackCallback == NULL || !FallbackCallback(FallbackContext, PdbFile, sizeof(PdbFile), &Guid, &Age))
        {
            return FALSE;
        }
    }

    if (SymbolServerRelativePath != NULL)
    {
        SymbolServerRelativePathBuffer.resize(SymbolServerRelativePathSize);
    }

    if (PdbFilePath != NULL)
    {
        PdbFilePathBuffer.resize(PdbFilePathSize);

        HRESULT Result = StringCchCopyA(PdbFilePathBuffer.data(), PdbFilePathBuffer.size(), PdbFile);
        if (FAILED(Result))
        {
            return FALSE;
        }
    }

    if (GuidAndAgeDetails != NULL)
    {
        GuidAndAgeDetailsBuffer.resize(GuidAndAgeDetailsSize);
    }

    if ((SymbolServerRelativePath != NULL || GuidAndAgeDetails != NULL) &&
        !SymFormatPdbIdentity(PdbFile,
                              &Guid,
                              Age,
                              SymbolServerRelativePath == NULL ? NULL : SymbolServerRelativePathBuffer.data(),
                              SymbolServerRelativePathBuffer.size(),
                              GuidAndAgeDetails == NULL ? NULL : GuidAndAgeDetailsBuffer.data(),
                              GuidAndAgeDetailsBuffer.size()))
    {
        return FALSE;
    }

    if (SymbolServerRelativePath != NULL &&
        FAILED(StringCchCopyA(SymbolServerRelativePath, SymbolServerRelativePathSize, SymbolServerRelativePathBuffer.data())))
    {
        return FALSE;
    }

    if (PdbFilePath != NULL && FAILED(StringCchCopyA(PdbFilePath, PdbFilePathSize, PdbFilePathBuffer.data())))
    {
        return FALSE;
    }

    if (GuidAndAgeDetails != NULL && FAILED(StringCchCopyA(GuidAndAgeDetails, GuidAndAgeDetailsSize, GuidAndAgeDetailsBuffer.data())))
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
SymFormatPdbIdentityFromPeImageOrFallback(const BYTE *                        PeImageBytes,
                                          SIZE_T                              PeImageSize,
                                          CHAR *                              SymbolServerRelativePath,
                                          SIZE_T                              SymbolServerRelativePathSize,
                                          CHAR *                              PdbFilePath,
                                          SIZE_T                              PdbFilePathSize,
                                          CHAR *                              GuidAndAgeDetails,
                                          SIZE_T                              GuidAndAgeDetailsSize,
                                          PSYM_PDB_IDENTITY_FALLBACK_CALLBACK FallbackCallback,
                                          PVOID                               FallbackContext)
{
    return SymFormatPdbIdentityFromExtractorOrFallback(PeImageBytes,
                                                       PeImageSize,
                                                       SymbolServerRelativePath,
                                                       SymbolServerRelativePathSize,
                                                       PdbFilePath,
                                                       PdbFilePathSize,
                                                       GuidAndAgeDetails,
                                                       GuidAndAgeDetailsSize,
                                                       SymExtractCodeViewRsdsInfoFromPeImage,
                                                       FallbackCallback,
                                                       FallbackContext);
}

BOOLEAN
SymFormatPdbIdentityFromLoadedPeImageOrFallback(const BYTE *                        PeImageBytes,
                                                SIZE_T                              PeImageSize,
                                                CHAR *                              SymbolServerRelativePath,
                                                SIZE_T                              SymbolServerRelativePathSize,
                                                CHAR *                              PdbFilePath,
                                                SIZE_T                              PdbFilePathSize,
                                                CHAR *                              GuidAndAgeDetails,
                                                SIZE_T                              GuidAndAgeDetailsSize,
                                                PSYM_PDB_IDENTITY_FALLBACK_CALLBACK FallbackCallback,
                                                PVOID                               FallbackContext)
{
    return SymFormatPdbIdentityFromExtractorOrFallback(PeImageBytes,
                                                       PeImageSize,
                                                       SymbolServerRelativePath,
                                                       SymbolServerRelativePathSize,
                                                       PdbFilePath,
                                                       PdbFilePathSize,
                                                       GuidAndAgeDetails,
                                                       GuidAndAgeDetailsSize,
                                                       SymExtractCodeViewRsdsInfoFromLoadedPeImage,
                                                       FallbackCallback,
                                                       FallbackContext);
}
