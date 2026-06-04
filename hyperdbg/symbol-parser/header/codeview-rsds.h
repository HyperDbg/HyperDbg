/**
 * @file codeview-rsds.h
 * @author jtaw5649
 * @brief Bounded in-memory CodeView RSDS parser
 * @details
 * @version 0.1
 * @date 2026-06-02
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

BOOLEAN
SymExtractCodeViewRsdsInfoFromPeImage(const BYTE * ImageBase,
                                      SIZE_T       ImageSize,
                                      CHAR *       PdbFileName,
                                      SIZE_T       PdbFileNameSize,
                                      GUID *       Guid,
                                      DWORD *      Age);

BOOLEAN
SymExtractCodeViewRsdsInfoFromLoadedPeImage(const BYTE * ImageBase,
                                            SIZE_T       ImageSize,
                                            CHAR *       PdbFileName,
                                            SIZE_T       PdbFileNameSize,
                                            GUID *       Guid,
                                            DWORD *      Age);
