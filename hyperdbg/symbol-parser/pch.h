/**
 * @file pch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief pre-compiled headers for symbol parser
 * @details
 * @version 0.1
 * @date 2021-05-29
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Scope definitions
//
#define HYPERDBG_SYMBOL_PARSER

using namespace std;

//
// Environment headers
//
#include "platform/user/header/Environment.h"

#include <Windows.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <strsafe.h>
#define _NO_CVCONST_H // for symbol parsing
#include <DbgHelp.h>

//
// IA32-doc has structures for the entire intel SDM.
//

#define USE_LIB_IA32
#if defined(USE_LIB_IA32)
#    pragma warning(push, 0)
// #    pragma warning(disable : 4201) // suppress nameless struct/union warning
#    include <ia32-doc/out/ia32.h>
#    pragma warning(pop)
typedef RFLAGS * PRFLAGS;
#endif // USE_LIB_IA32

#include "SDK/HyperDbgSdk.h"
#include "config/Definition.h"
#include "SDK/imports/user/HyperDbgLibImports.h"
#include "../symbol-parser/header/common-utils.h"
#include "../symbol-parser/header/symbol-parser.h"

//
// Module imports/exports
//
#include "SDK/imports/user/HyperDbgSymImports.h"

//
// Needed to link symbol server
//
#pragma comment(lib, "dbghelp.lib")

//
// For URLDownloadToFileA
//
#pragma comment(lib, "Urlmon.lib")
