/**
 * @file pch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief pre-compiled headers
 * @details
 * @version 0.1
 * @date 2020-09-16
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// namespace
//
using namespace std;

//
// Environment headers
//
#include "platform/general/header/Environment.h"

//
// General Headers
//
#include <Windows.h>
#include <iostream>
#include <string>
#include <conio.h>
#include <vector>
#include <regex>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <strsafe.h>

//
// SDK and config headers
//
#include "SDK/HyperDbgSdk.h"
#include "config/Definition.h"

//
// Program Defined Headers
//
#include "header/namedpipe.h"
#include "header/routines.h"
#include "header/testcases.h"
#include "header/pdb-identity.h"
#include "header/codeview-rsds.h"

//
// Components
//
#include "../include/components/pe/header/pe-image-reader.h"

//
// Hardware Debugger Headers
//
#include "header/hwdbg-tests.h"

//
// import libhyperdbg
//
#include "SDK/imports/user/HyperDbgLibImports.h"
