/**
 * @file pe-parser.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Header for Protable Execuatable parser
 * @details
 * @version 0.1
 * @date 2021-12-26
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					  Functions                 //
//////////////////////////////////////////////////

BOOLEAN
PeShowSectionInformationAndDump(WCHAR * AddressOfFile, CHAR * SectionToShow, BOOLEAN Is32Bit);

BOOLEAN
PeGetEntryPoint(WCHAR * AddressOfFile, BOOLEAN Is32Bit, DWORD * EntryPoint);
