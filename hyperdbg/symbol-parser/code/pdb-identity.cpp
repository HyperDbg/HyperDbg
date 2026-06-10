/**
 * @file pdb-identity.cpp
 * @author jtaw5649
 * @brief Internal PDB identity formatting helpers
 * @details
 * @version 0.19
 * @date 2026-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#include "header/codeview-rsds.h"
#include "header/pdb-identity.h"

/**
 * @brief Helper function to clear the output buffer
 *
 *
 * @param Buffer The output buffer pointer to check
 * @param BufferSize The size of the output buffer in bytes
 *
 * @return BOOLEAN TRUE if the buffer is valid for writing, FALSE otherwise
 */
static VOID
SymClearOutputBuffer(CHAR * Buffer, SIZE_T BufferSize)
{
    if (Buffer != NULL && BufferSize != 0)
    {
        Buffer[0] = '\0';
    }
}

/**
 * @brief Helper function to check if the output buffer is valid for writing
 *
 * A buffer is considered valid if it is either NULL (indicating the caller does not want that output) or if it has a non-zero size.
 *
 * @param Buffer The output buffer pointer to check
 * @param BufferSize The size of the output buffer in bytes
 *
 * @return BOOLEAN TRUE if the buffer is valid for writing, FALSE otherwise
 */
static BOOLEAN
SymOutputBufferHasSpace(const CHAR * Buffer, SIZE_T BufferSize)
{
    return Buffer == NULL || BufferSize != 0;
}

/**
 * @brief Helper function to format the PDB identity information into the specified output buffers
 *
 * The function formats the PDB file name, GUID, and age into a symbol server relative path and a combined GUID-and-age string according to the standard symbol server layout.
 *
 * @param PdbFile The base name of the PDB file (e.g., "example.pdb")
 * @param Guid The GUID associated with the PDB
 * @param Age The age associated with the PDB
 * @param SymbolServerRelativePath An optional output buffer to receive the formatted symbol server relative path. If not NULL, it must have enough space for the formatted string
 * @param SymbolServerRelativePathSize The size of the SymbolServerRelativePath buffer in bytes
 * @param GuidAndAgeDetails An optional output buffer to receive the formatted GUID and age details string. If not NULL, it must have enough space for the formatted string
 * @param GuidAndAgeDetailsSize The size of the GuidAndAgeDetails buffer in bytes
 *
 * @return BOOLEAN TRUE if the formatting succeeded and output buffers were filled as requested, FALSE if there was an error (e.g., invalid parameters or insufficient buffer sizes)
 */
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

/**
 * @brief Helper function to extract PDB identity information from a PE image using a specified extractor callback, with an optional fallback if extraction fails
 *
 * @param PeImageBytes A pointer to the bytes of the PE image in memory
 * @param PeImageSize The size of the PE image in bytes
 * @param SymbolServerRelativePath An optional output buffer to receive the formatted symbol server relative path. If not NULL, it must have enough space for the formatted string
 * @param SymbolServerRelativePathSize The size of the SymbolServerRelativePath buffer in bytes
 * @param PdbFilePath An optional output buffer to receive the extracted PDB file path. If not NULL, it must have enough space for the file path string
 * @param PdbFilePathSize The size of the PdbFilePath buffer in bytes
 * @param GuidAndAgeDetails An optional output buffer to receive the formatted GUID and age details string. If not NULL, it must have enough space for the formatted string
 * @param GuidAndAgeDetailsSize The size of the GuidAndAgeDetails buffer in bytes
 * @param ExtractorCallback A callback function that attempts to extract the PDB file name, GUID, and age from the PE image bytes. It should return TRUE on success and FALSE on failure
 * @param FallbackCallback An optional callback function that is invoked if the extractor callback fails. It should attempt to provide the same information as the extractor and return TRUE on success or FALSE on failure
 * @param FallbackContext An optional context pointer that is passed to the fallback callback when invoked
 *
 * @return BOOLEAN TRUE if either extraction method succeeded and output buffers were filled as requested, FALSE if both methods failed or if there was an error with parameters or buffer sizes
 */
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

/**
 * @brief Extracts PDB identity information from a PE image using the specified extractor callback, with an optional fallback if extraction fails
 *
 * @param PeImageBytes A pointer to the bytes of the PE image in memory
 * @param PeImageSize The size of the PE image in bytes
 * @param SymbolServerRelativePath An optional output buffer to receive the formatted symbol server relative path. If not NULL, it must have enough space for the formatted string
 * @param SymbolServerRelativePathSize The size of the SymbolServerRelativePath buffer in bytes
 * @param PdbFilePath An optional output buffer to receive the extracted PDB file path. If not NULL, it must have enough space for the file path string
 * @param PdbFilePathSize The size of the PdbFilePath buffer in bytes
 * @param GuidAndAgeDetails An optional output buffer to receive the formatted GUID and age details string. If not NULL, it must have enough space for the formatted string
 * @param GuidAndAgeDetailsSize The size of the GuidAndAgeDetails buffer in bytes
 * @param FallbackCallback An optional callback function that is invoked if the extractor callback fails. It should attempt to provide the same information as the extractor and return TRUE on success or FALSE on failure
 * @param FallbackContext An optional context pointer that is passed to the fallback callback when invoked
 *
 * @return BOOLEAN TRUE if either extraction method succeeded and output buffers were filled as requested, FALSE if both methods failed or if there was an error with parameters or buffer sizes
 */
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

/**
 * @brief Extracts PDB identity information from a PE image using the specified extractor callback that parses the image as if it is loaded in memory, with an optional fallback if extraction fails
 *
 * @param PeImageBytes A pointer to the bytes of the PE image in memory
 * @param PeImageSize The size of the PE image in bytes
 * @param SymbolServerRelativePath An optional output buffer to receive the formatted symbol server relative path. If not NULL, it must have enough space for the formatted string
 * @param SymbolServerRelativePathSize The size of the SymbolServerRelativePath buffer in bytes
 * @param PdbFilePath An optional output buffer to receive the extracted PDB file path. If not NULL, it must have enough space for the file path string
 * @param PdbFilePathSize The size of the PdbFilePath buffer in bytes
 * @param GuidAndAgeDetails An optional output buffer to receive the formatted GUID and age details string. If not NULL, it must have enough space for the formatted string
 * @param GuidAndAgeDetailsSize The size of the GuidAndAgeDetails buffer in bytes
 * @param FallbackCallback An optional callback function that is invoked if the extractor callback fails. It should attempt to provide the same information as the extractor and return TRUE on success or FALSE on failure
 * @param FallbackContext An optional context pointer that is passed to the fallback callback when invoked
 *
 * @return BOOLEAN TRUE if either extraction method succeeded and output buffers were filled as requested, FALSE if both methods failed or if there was an error with parameters or buffer sizes
 */
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
