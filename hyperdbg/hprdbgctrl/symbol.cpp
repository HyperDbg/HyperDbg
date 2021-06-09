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
 * this buffer will be allocated by this function and needs to be freed by caller
 * @param StoredLength The length that stored on the BufferToStoreDetails
 * 
 * @return BOOLEAN shows whether the operation was successful or not
 */
BOOLEAN
SymbolBuildSymbolTable(PMODULE_SYMBOL_DETAIL * BufferToStoreDetails, PUINT32 StoredLength)
{
    PRTL_PROCESS_MODULES  ModuleInfo;
    NTSTATUS              Status;
    PMODULE_SYMBOL_DETAIL ModuleSymDetailArray                              = NULL;
    char                  SystemRoot[MAX_PATH]                              = {0};
    char                  ModuleSymbolPath[MAX_PATH]                        = {0};
    char                  ModuleSymbolGuidAndAge[MAXIMUM_GUID_AND_AGE_SIZE] = {0};
    BOOLEAN               IsSymbolPdbDetailAvailable                        = FALSE;

    //
    // Get system root
    //
    if (GetSystemDirectoryA(SystemRoot, MAX_PATH) == NULL)
    {
        ShowMessages("err, unable to get system directory (%d)\n",
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
        ShowMessages("err, unable to allocate memory for module list (%d)\n",
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
        ShowMessages("err, unable to query module list (%#x)\n", Status);

        VirtualFree(ModuleInfo, 0, MEM_RELEASE);
        return FALSE;
    }

    //
    // Allocate Details buffer
    //
    ModuleSymDetailArray = (PMODULE_SYMBOL_DETAIL)malloc(ModuleInfo->NumberOfModules * sizeof(MODULE_SYMBOL_DETAIL));

    if (ModuleSymDetailArray == NULL)
    {
        ShowMessages("err, unable to allocate memory for module list (%d)\n",
                     GetLastError());
        return FALSE;
    }

    //
    // Make sure buffer is zero
    //
    RtlZeroMemory(ModuleSymDetailArray, ModuleInfo->NumberOfModules * sizeof(MODULE_SYMBOL_DETAIL));

    for (int i = 0; i < ModuleInfo->NumberOfModules; i++)
    {
        auto PathName = ModuleInfo->Modules[i].FullPathName + ModuleInfo->Modules[i].OffsetToFileName;

        //
        // Read symbol signature details
        //
        RtlZeroMemory(ModuleSymbolPath, sizeof(ModuleSymbolPath));
        RtlZeroMemory(ModuleSymbolGuidAndAge, sizeof(ModuleSymbolGuidAndAge));

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

        if (ScriptEngineConvertFileToPdbFileAndGuidAndAgeDetailsWrapper(ModuleFullPath.c_str(), ModuleSymbolPath, ModuleSymbolGuidAndAge))
        {
            IsSymbolPdbDetailAvailable = TRUE;

            //
            // ShowMessages("Hash : %s , Symbol path : %s\n", ModuleSymbolGuidAndAge, ModuleSymbolPath);
            //
        }
        else
        {
            IsSymbolPdbDetailAvailable = FALSE;

            //
            // ShowMessages("err, unable to get module pdb details\n");
            //
        }

        //
        // Build the structure for this module
        //

        ModuleSymDetailArray[i].BaseAddress = (UINT64)ModuleInfo->Modules[i].ImageBase;
        memcpy(ModuleSymDetailArray[i].FilePath, ModuleFullPath.c_str(), ModuleFullPath.size());

        if (IsSymbolPdbDetailAvailable)
        {
            ModuleSymDetailArray[i].IsSymbolDetailsFound = TRUE;
            memcpy(ModuleSymDetailArray[i].ModuleSymbolGuidAndAge, ModuleSymbolGuidAndAge, MAXIMUM_GUID_AND_AGE_SIZE);
            memcpy(ModuleSymDetailArray[i].ModuleSymbolPath, ModuleSymbolPath, MAX_PATH);

            //
            // Check if pdb file name is a real path or a module name
            //
            string ModuleSymbolPathString(ModuleSymbolPath);
            if (ModuleSymbolPathString.find(":\\") != std::string::npos)
                ModuleSymDetailArray[i].IsRealSymbolPath = TRUE;
            else
                ModuleSymDetailArray[i].IsRealSymbolPath = FALSE;
        }
        else
        {
            ModuleSymDetailArray[i].IsSymbolDetailsFound = FALSE;
        }
    }
    //
    // Store the buffer and length of module symbols details
    //
    *BufferToStoreDetails = ModuleSymDetailArray;
    *StoredLength         = ModuleInfo->NumberOfModules * sizeof(MODULE_SYMBOL_DETAIL);

    VirtualFree(ModuleInfo, 0, MEM_RELEASE);

    return TRUE;
}
