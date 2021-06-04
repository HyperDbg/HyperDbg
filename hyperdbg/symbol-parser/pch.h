/**
 * @file pch.h
 * @author Sina Karvandi (sina@rayanfam.com)
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

#include "Definition.h"
#include "symbol-parser.h"

using namespace std;

//
// Needed to link symbol server
//
#pragma comment(lib, "dbghelp.lib")
