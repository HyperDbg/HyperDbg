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
#include "platform/user/header/Environment.h"

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

//
// Program Defined Headers
//
#include "SDK/HyperDbgSdk.h"
#include "Definition.h"
#include "..\hyperdbg-test\header\namedpipe.h"
#include "..\hyperdbg-test\header\routines.h"
#include "..\hyperdbg-test\header\testcases.h"

//
// import libhyperdbg
//
#include "SDK/imports/user/HyperDbgLibImports.h"
