/**
 * @file script-engine.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief General script-engine functions and wrappers
 * @details
 * @version 0.1
 * @date 2021-09-23
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//    Pdb Parser Wrapper (from script-engine)   //
//////////////////////////////////////////////////
UINT64
ScriptEngineConvertNameToAddressWrapper(const CHAR * FunctionOrVariableName, PBOOLEAN WasFound);

UINT32
ScriptEngineLoadFileSymbolWrapper(UINT64 BaseAddress, const CHAR * PdbFileName, const CHAR * CustomModuleName);

VOID
ScriptEngineSetTextMessageCallbackWrapper(PVOID Handler);

UINT32
ScriptEngineUnloadAllSymbolsWrapper();

UINT32
ScriptEngineUnloadModuleSymbolWrapper(CHAR * ModuleName);

UINT32
ScriptEngineSearchSymbolForMaskWrapper(const CHAR * SearchMask);

BOOLEAN
ScriptEngineGetFieldOffsetWrapper(CHAR * TypeName, CHAR * FieldName, UINT32 * FieldOffset);

BOOLEAN
ScriptEngineGetDataTypeSizeWrapper(CHAR * TypeName, UINT64 * TypeSize);

BOOLEAN
ScriptEngineCreateSymbolTableForDisassemblerWrapper(VOID * CallbackFunction);

BOOLEAN
ScriptEngineConvertFileToPdbPathWrapper(const CHAR * LocalFilePath, CHAR * ResultPath, SIZE_T ResultPathSize);

BOOLEAN
ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetailsWrapper(const CHAR * LocalFilePath,
                                                            CHAR *       PdbFilePath,
                                                            CHAR *       GuidAndAgeDetails,
                                                            BOOLEAN      Is32BitModule);

BOOLEAN
ScriptEngineConvertLoadedModuleToPdbFileAndGuidAndAgeDetailsWrapper(const BYTE * LoadedImageBytes,
                                                                    SIZE_T       LoadedImageSize,
                                                                    const CHAR * LocalFilePath,
                                                                    CHAR *       PdbFilePath,
                                                                    CHAR *       GuidAndAgeDetails,
                                                                    BOOLEAN      Is32BitModule);

BOOLEAN
ScriptEngineSymbolInitLoadWrapper(PMODULE_SYMBOL_DETAIL BufferToStoreDetails,
                                  UINT32                StoredLength,
                                  BOOLEAN               DownloadIfAvailable,
                                  const CHAR *          SymbolPath,
                                  BOOLEAN               IsSilentLoad);

BOOLEAN
ScriptEngineShowDataBasedOnSymbolTypesWrapper(
    const CHAR * TypeName,
    UINT64       Address,
    BOOLEAN      IsStruct,
    PVOID        BufferAddress,
    const CHAR * AdditionalParameters);

VOID
ScriptEngineSymbolAbortLoadingWrapper();

//////////////////////////////////////////////////
//          Script Engine Wrapper               //
//////////////////////////////////////////////////

VOID
ScriptEngineWrapperTestParser(const string & Expr);

VOID
ScriptEngineWrapperTestParserForHwdbg(const string & Expr);

BOOLEAN
ScriptAutomaticStatementsTestWrapper(const string & Expr, UINT64 ExpectationValue, BOOLEAN ExceptError);

PVOID
ScriptEngineParseWrapper(CHAR * Expr, BOOLEAN ShowErrorMessageIfAny);

VOID
PrintSymbolBufferWrapper(PVOID SymbolBuffer);

UINT64
ScriptEngineWrapperGetHead(PVOID SymbolBuffer);

UINT32
ScriptEngineWrapperGetSize(PVOID SymbolBuffer);

UINT32
ScriptEngineWrapperGetPointer(PVOID SymbolBuffer);

VOID
ScriptEngineWrapperRemoveSymbolBuffer(PVOID SymbolBuffer);

UINT64
ScriptEngineEvalUInt64StyleExpressionWrapper(const string & Expr, PBOOLEAN HasError);

//////////////////////////////////////////////////
//          Script Engine Functions             //
//////////////////////////////////////////////////

UINT64
ScriptEngineEvalSingleExpression(string Expr, PBOOLEAN HasError);

BOOLEAN
ScriptEngineExecuteSingleExpression(CHAR * Expr, BOOLEAN ShowErrorMessageIfAny, BOOLEAN IsFormat);
