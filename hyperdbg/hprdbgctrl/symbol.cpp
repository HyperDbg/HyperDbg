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
    //
    // To be implemented
    //
    //ScriptEngineLoadFileSymbolWrapper(BaseAddress, "C:\\symbols\\ntkrnlmp.pdb\\3D4400784115718818EFC898413F36C41\\ntkrnlmp.pdb");

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

/**
 * @brief make the initial packet required for symbol server
 * or reload packet
 * 
 * @param BufferToStoreDetails Pointer to a buffer to store the symbols details
 * @param StoredLength The length that stored on the BufferToStoreDetails
 * 
 * @return BOOLEAN shows whether the operation was successful or not
 */
BOOLEAN
SymbolReloadLoadedModulesInformation(PVOID BufferToStoreDetails, PUINT StoredLength)
{
    PRTL_PROCESS_MODULES ModuleInfo;
    NTSTATUS             Status;
    char                 SystemRoot[MAX_PATH]         = {0};
    char                 ModuleSymbolDetail[MAX_PATH] = {0};

    //
    // Get system root
    //
    if (GetSystemDirectoryA(SystemRoot, MAX_PATH) == NULL)
    {
        ShowMessages("\nerr, unable to get system directory (%d)\n",
                     GetLastError());

        return FALSE;
    }

    string SystemRootString(SystemRoot);

    //
    // Convert root path to lower-case
    //
    transform(SystemRootString.begin(),
              SystemRootString.end(),
              SystemRootString.begin(),
              [](unsigned char c) { return std::tolower(c); });

    //
    // Remove system32 from the root
    //
    Replace(SystemRootString, "\\system32", "");

    //
    // Allocate memory for the module list
    //
    ModuleInfo = (PRTL_PROCESS_MODULES)VirtualAlloc(
        NULL,
        1024 * 1024,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);

    if (!ModuleInfo)
    {
        ShowMessages("\nUnable to allocate memory for module list (%d)\n",
                     GetLastError());
        return FALSE;
    }

    //
    // 11 = SystemModuleInformation
    //
    if (!NT_SUCCESS(
            Status = NtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)11,
                                              ModuleInfo,
                                              1024 * 1024,
                                              NULL)))
    {
        ShowMessages("\nError: Unable to query module list (%#x)\n", Status);

        VirtualFree(ModuleInfo, 0, MEM_RELEASE);
        return FALSE;
    }

    for (int i = 0; i < ModuleInfo->NumberOfModules; i++)
    {
        //ShowMessages("%s\t", SeparateTo64BitValue((UINT64)ModuleInfo->Modules[i].ImageBase).c_str());

        auto PathName = ModuleInfo->Modules[i].FullPathName + ModuleInfo->Modules[i].OffsetToFileName;

        //ShowMessages("%s\t", PathName);

        //ShowMessages("%s\t", ModuleInfo->Modules[i].FullPathName);

        //
        // Read symbol signature details
        //
        RtlZeroMemory(ModuleSymbolDetail, sizeof(ModuleSymbolDetail));
        string ModuleFullPath((const char *)ModuleInfo->Modules[i].FullPathName);

        if (ModuleFullPath.rfind("\\SystemRoot\\", 0) == 0)
        {
            //
            // Path starts with \SystemRoot\
            // we should change it to the real system root
            // path
            //
            Replace(ModuleFullPath, "\\SystemRoot", SystemRootString);
        }

        if (ScriptEngineConvertFileToPdbPathWrapper(ModuleFullPath.c_str(), ModuleSymbolDetail))
        {
            ShowMessages("Symbol details : %s\n", ModuleSymbolDetail);
        }
        else
        {
            ShowMessages("err, unable to get module pdb details\n");
        }
    }

    VirtualFree(ModuleInfo, 0, MEM_RELEASE);

    return TRUE;
}
