#include <ntifs.h>
#include <ntddk.h>
#include <wdf.h>
#include <wdm.h>
#include <ntstrsafe.h>
#include <Windef.h>

//
// Scope definitions
//
#define SCRIPT_ENGINE_KERNEL_MODE
#define HYPERDBG_KERNEL_MODE

//
// HyperDbg SDK headers
//
#include "SDK/HyperDbgSdk.h"
#include "SDK/Imports/HyperDbgVmmImports.h"
#include "../../hprdbghv/header/common/RefactorTempStructs.h" // should be removed

#include "globals/Global.h"
#include "driver/Driver.h"
#include "driver/Loader.h"
