/**
 * @file pch.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Pre-compiled headers for reversing machine's module
 * @details
 *
 * @version 0.2
 * @date 2023-02-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//
// Environment headers
//
#include "platform/user/header/Environment.h"

//
// Windows SDK headers
//
#define WIN32_LEAN_AND_MEAN

//
// Scope definitions
//
#define HYPERDBG_USER_MODE_REVERSING_MODULE

#include <Windows.h>

#include <iostream>
#include <stdio.h>
#include <string>

//
// HyperDbg SDK headers
//
#include "Definition.h"
#include "SDK/HyperDbgSdk.h"
#include "SDK/Imports/HyperDbgCtrlImports.h"
