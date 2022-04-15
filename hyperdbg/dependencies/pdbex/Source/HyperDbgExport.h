
#pragma once

/**
 * @brief Callback type that can be used to be used
 * as a custom ShowMessages function
 *
 */
typedef int (*Callback)(const char * Text);

#define HYPERDBG_CODES

extern "C" {
__declspec(dllexport) int pdbex_export(int argc, char ** argv, bool is_struct, void * buffer_address);
__declspec(dllexport) void pdbex_set_logging_method_export(PVOID handler);
}

UINT64
ExtractBits(UINT64 Orig64Bit, UINT64 From, UINT64 To);

std::string
SymSeparateTo64BitValue(UINT64 Value);
