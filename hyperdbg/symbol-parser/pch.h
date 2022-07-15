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

#include <Windows.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <vector>

#define _NO_CVCONST_H // for symbol parsing
#include <DbgHelp.h>

//
// IA32-doc has structures for the entire intel SDM.
//

#define USE_LIB_IA32
#if defined(USE_LIB_IA32)
#    pragma warning(push, 0)
//#    pragma warning(disable : 4201) // suppress nameless struct/union warning
#    include <ia32-doc/out/ia32.h>
#    pragma warning(pop)
typedef RFLAGS * PRFLAGS;
#endif // USE_LIB_IA32

#include "SDK/HyperDbgSdk.h"
#include "SDK/Imports/HyperDbgCtrlImports.h"
#include "Definition.h"
#include "..\symbol-parser\header\common-utils.h"
#include "..\symbol-parser\header\symbol-parser.h"

using namespace std;

//
// Needed to link symbol server
//
#pragma comment(lib, "dbghelp.lib")

//
// For URLDownloadToFileA
//
#pragma comment(lib, "Urlmon.lib")
