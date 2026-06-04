/**
 * @file pdb-identity.h
 * @author jtaw5649
 * @brief Internal PDB identity formatting helpers
 * @details
 * @version 0.1
 * @date 2026-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

typedef BOOLEAN (*PSYM_PDB_IDENTITY_FALLBACK_CALLBACK)(PVOID   Context,
                                                       CHAR *  PdbFile,
                                                       SIZE_T  PdbFileSize,
                                                       GUID *  Guid,
                                                       DWORD * Age);

BOOLEAN
SymFormatPdbIdentity(const CHAR * PdbFile,
                     const GUID * Guid,
                     DWORD        Age,
                     CHAR *       SymbolServerRelativePath,
                     SIZE_T       SymbolServerRelativePathSize,
                     CHAR *       GuidAndAgeDetails,
                     SIZE_T       GuidAndAgeDetailsSize);

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
                                          PVOID                               FallbackContext);
