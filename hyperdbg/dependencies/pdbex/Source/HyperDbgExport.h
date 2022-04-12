
#pragma once

/**
 * @brief Callback type that can be used to be used
 * as a custom ShowMessages function
 *
 */
typedef int (*Callback)(const char * Text);

#define HYPERDBG_CODES


extern "C" {
__declspec(dllexport) int pdbex_main_impl_export(int argc, char ** argv);
__declspec(dllexport) void pdbex_set_logging_method_export(PVOID handler);
}
