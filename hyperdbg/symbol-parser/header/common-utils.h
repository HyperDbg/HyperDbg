/**
 * @file common-utils.h
 * @author Alee Amini (alee@hyperdbg.org)
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief common utils headers
 * @details 
 * @version 0.1
 * @date 2021-06-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Functions                   //
//////////////////////////////////////////////////

BOOLEAN
IsFileExists(const std::string & FileName);

BOOLEAN
IsDirExists(const std::string & DirPath);

BOOLEAN
CreateDirectoryRecursive(const std::string & Path);

const std::vector<std::string>
Split(const std::string & s, const char & c);
