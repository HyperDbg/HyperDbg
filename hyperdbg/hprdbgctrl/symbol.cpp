/**
 * @file symbol.cpp
 * @author Alee Amini (aleeaminiz@gmail.com)
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief symbol parser
 * @details
 * @version 0.1
 * @date 2021-05-20
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

/**
 * @brief Load symbol for ntoskrnl
 * 
 * @param BaseAddress Base address of ntoskrnl
 * 
 * @return BOOLEAN shows whether the conversion was successful or not
 */
BOOLEAN
SymbolLoadNtoskrnlSymbol(UINT64 BaseAddress)
{
    ScriptEngineLoadFileSymbolWrapper(BaseAddress, "ntoskrnl.exe", "123");

    return TRUE;
}

/**
 * @brief check and convert string to a 64 bit unsigned interger and also
 *  check for symbol object names
 * 
 * @param TextToConvert the target string
 * @param Result result will be save to the pointer
 * 
 * @return BOOLEAN shows whether the conversion was successful or not
 */
BOOLEAN
SymbolConvertNameToAddress(string TextToConvert, PUINT64 Result)
{
    BOOLEAN IsFound = FALSE;
    UINT64  Address = NULL;

    if (!ConvertStringToUInt64(TextToConvert, &Address))
    {
        //
        // Check for symbol object names
        //
        Address = ScriptEngineConvertNameToAddressWrapper(TextToConvert.c_str(), &IsFound);

        if (!IsFound)
        {
            //
            // It's neither a number, nor a founded object name
            //
            IsFound = FALSE;
        }
        else
        {
            //
            // Object name is found
            //
            IsFound = TRUE;
        }
    }
    else
    {
        //
        // It's a hex number
        //
        IsFound = TRUE;
    }

    //
    // Set the number if the address is founded
    //
    if (IsFound)
    {
        *Result = Address;
    }

    return IsFound;
}
