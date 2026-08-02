/**
 * @file ucpuid.cpp
 * @author Nikzad (Hossein Shirdel)
 * @brief ucpuid command - reads and decodes USER specified CPUID information
 * @details
 * @version 0.23
 * @date 2026-06-18
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

//
// Global Variables
//
extern BOOLEAN g_IsKdModuleLoaded;
extern BOOLEAN g_IsSerialConnectedToRemoteDebuggee;

/**
 * @brief help of the ucpuid command
 *
 * @return VOID
 */
VOID
CommandUserCpuidHelp()
{
    ShowMessages("ucpuid : reads CPUID information on the target debuggee.\n\n");

    ShowMessages("syntax : \tucpuid [Function (hex)] [SubFunction (hex)]\n\n");

    ShowMessages("\n");
    ShowMessages("\t\te.g : ucpuid 1\n");
    ShowMessages("\t\te.g : ucpuid 4 2\n");
    ShowMessages("\t\te.g : ucpuid D 0\n");
    ShowMessages("\t\te.g : ucpuid 0x80000008 0\n\n");

    ShowMessages("\n");
    ShowMessages("note 1: use `ucpuid 0` to see the maximum supported CPUID leaf\n");
    ShowMessages("note 2: use `ucpuid 0x80000000` to see the maximum supported extended leaf\n");
}

/**
 * @brief ucpuid command show messages
 *
 * @return VOID
 */
VOID
CommandShowUserCpuidMessage(UINT32                           FunctionId,
                            UINT32                           SubFunctionId,
                            PDEBUGGER_CPUID_REQUEST_RESPONSE CpuidRequest)
{
    CONST CHAR * TypeName = NULL;
    CONST CHAR * AssocName;

    //
    // validate the pointer before using it
    //
    if (CpuidRequest == NULL)
    {
        ShowMessages("  NULL value!\n");
        return;
    }

    //
    // display result
    //
    switch (FunctionId)
    {
    case 0x0:
    {
        CHAR Vendor[13] = {0};
        memcpy(&Vendor[0], &CpuidRequest->EBX, 4);
        memcpy(&Vendor[4], &CpuidRequest->EDX, 4);
        memcpy(&Vendor[8], &CpuidRequest->ECX, 4);
        Vendor[12] = '\0';

        ShowMessages("  *******************************************************\n"
                     "  *               LEAF 0 HAS NO SUBLEAVES               *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        ShowMessages("  Vendor : %s\n", Vendor);
        ShowMessages("  Maximum supported basic leaf : %u\n", CpuidRequest->EAX);

        break;
    }
    case 0x1:
        //
        // EAX
        //
        ShowMessages("==== CPUID.(EAX=01H) Version / Additional / Feature Information ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *               LEAF 1 HAS NO SUBLEAVES               *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        ShowMessages("-- EAX: Version Information --\n\n");
        ShowMessages("  SteppingId       = %u\n",
                     CPUID_VERSION_INFORMATION_STEPPING_ID(CpuidRequest->EAX));
        ShowMessages("  Model            = %u\n",
                     CPUID_VERSION_INFORMATION_MODEL(CpuidRequest->EAX));
        ShowMessages("  FamilyId         = %u\n",
                     CPUID_VERSION_INFORMATION_FAMILY_ID(CpuidRequest->EAX));
        ShowMessages("  ProcessorType    = %u\n",
                     CPUID_VERSION_INFORMATION_PROCESSOR_TYPE(CpuidRequest->EAX));
        ShowMessages("  ExtendedModelId  = %u\n",
                     CPUID_VERSION_INFORMATION_EXTENDED_MODEL_ID(CpuidRequest->EAX));
        ShowMessages("  ExtendedFamilyId = %u\n\n",
                     CPUID_VERSION_INFORMATION_EXTENDED_FAMILY_ID(CpuidRequest->EAX));

        //
        // EBX: additional information
        //
        ShowMessages("-- EBX: Additional Information --\n\n");
        ShowMessages("  BrandIndex        = %u\n",
                     CPUID_ADDITIONAL_INFORMATION_BRAND_INDEX(CpuidRequest->EBX));
        ShowMessages("  ClflushLineSize   = %u (cache line = %u bytes)\n",
                     CPUID_ADDITIONAL_INFORMATION_CLFLUSH_LINE_SIZE(CpuidRequest->EBX),
                     CPUID_ADDITIONAL_INFORMATION_CLFLUSH_LINE_SIZE(CpuidRequest->EBX) * 8);
        ShowMessages("  MaxAddressableIds = %u\n",
                     CPUID_ADDITIONAL_INFORMATION_MAX_ADDRESSABLE_IDS(CpuidRequest->EBX));
        ShowMessages("  InitialApicId     = %u\n\n",
                     CPUID_ADDITIONAL_INFORMATION_INITIAL_APIC_ID(CpuidRequest->EBX));

        //
        // ECX: feature information
        //
        ShowMessages("-- ECX: Feature Information --\n\n");
        ShowMessages("  SSE3                  = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_STREAMING_SIMD_EXTENSIONS_3(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  PCLMULQDQ             = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_PCLMULQDQ_INSTRUCTION(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  DTES64                = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_DS_AREA_64BIT_LAYOUT(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  MONITOR/MWAIT         = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_MONITOR_MWAIT_INSTRUCTION(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  CPL Qualified DS      = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_CPL_QUALIFIED_DEBUG_STORE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  VMX                   = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_VIRTUAL_MACHINE_EXTENSIONS(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  SMX                   = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_SAFER_MODE_EXTENSIONS(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  EIST (SpeedStep)      = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_ENHANCED_INTEL_SPEEDSTEP_TECHNOLOGY(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  TM2                   = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_THERMAL_MONITOR_2(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  SSSE3                 = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_SUPPLEMENTAL_STREAMING_SIMD_EXTENSIONS_3(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  L1 Context ID         = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_L1_CONTEXT_ID(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  Silicon Debug         = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_SILICON_DEBUG(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  FMA                   = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_FMA_EXTENSIONS(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  CMPXCHG16B            = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_CMPXCHG16B_INSTRUCTION(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  xTPR Update Control   = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_XTPR_UPDATE_CONTROL(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  PDCM                  = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_PERFMON_AND_DEBUG_CAPABILITY(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  PCID                  = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_PROCESS_CONTEXT_IDENTIFIERS(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  DCA                   = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_DIRECT_CACHE_ACCESS(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  SSE4.1                = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_SSE41_SUPPORT(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  SSE4.2                = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_SSE42_SUPPORT(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  x2APIC                = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_X2APIC_SUPPORT(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  MOVBE                 = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_MOVBE_INSTRUCTION(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  POPCNT                = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_POPCNT_INSTRUCTION(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  TSC-Deadline          = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_TSC_DEADLINE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  AESNI                 = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_AESNI_INSTRUCTION_EXTENSIONS(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  XSAVE/XRSTOR          = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_XSAVE_XRSTOR_INSTRUCTION(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  OSXSAVE               = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_OSX_SAVE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  AVX                   = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_AVX_SUPPORT(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  F16C                  = %s\n",
                     CPUID_FEATURE_INFORMATION_ECX_HALF_PRECISION_CONVERSION_INSTRUCTIONS(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  RDRAND                = %s\n\n",
                     CPUID_FEATURE_INFORMATION_ECX_RDRAND_INSTRUCTION(CpuidRequest->ECX) ? "TRUE" : "FALSE");

        //
        // EDX: feature information
        //
        ShowMessages("-- EDX: Feature Information --\n\n");
        ShowMessages("  FPU                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_FLOATING_POINT_UNIT_ON_CHIP(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  VME                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_VIRTUAL_8086_MODE_ENHANCEMENTS(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  DE                    = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_DEBUGGING_EXTENSIONS(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  PSE                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  TSC                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_TIMESTAMP_COUNTER(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  MSR (RDMSR/WRMSR)     = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_RDMSR_WRMSR_INSTRUCTIONS(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  PAE                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_PHYSICAL_ADDRESS_EXTENSION(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  MCE                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_EXCEPTION(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  CX8 (CMPXCHG8B)       = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_CMPXCHG8B(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  APIC On-Chip          = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_APIC_ON_CHIP(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  SEP (SYSENTER/EXIT)   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_SYSENTER_SYSEXIT_INSTRUCTIONS(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  MTRR                  = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_MEMORY_TYPE_RANGE_REGISTERS(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  PGE                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_PAGE_GLOBAL_BIT(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  MCA                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_MACHINE_CHECK_ARCHITECTURE(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  CMOV                  = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_CONDITIONAL_MOVE_INSTRUCTIONS(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  PAT                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_PAGE_ATTRIBUTE_TABLE(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  PSE-36                = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_PAGE_SIZE_EXTENSION_36BIT(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  PSN                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_PROCESSOR_SERIAL_NUMBER(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  CLFSH                 = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_CLFLUSH(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  DS (Debug Store)      = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_DEBUG_STORE(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  ACPI (Thermal/Clock)  = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_THERMAL_CONTROL_MSRS_FOR_ACPI(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  MMX                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_MMX_SUPPORT(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  FXSR (FXSAVE/FXRSTOR) = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_FXSAVE_FXRSTOR_INSTRUCTIONS(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  SSE                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_SSE_SUPPORT(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  SSE2                  = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_SSE2_SUPPORT(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  SS (Self Snoop)       = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_SELF_SNOOP(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  HTT                   = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_HYPER_THREADING_TECHNOLOGY(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  TM (Thermal Monitor)  = %s\n",
                     CPUID_FEATURE_INFORMATION_EDX_THERMAL_MONITOR(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  PBE                   = %s\n\n",
                     CPUID_FEATURE_INFORMATION_EDX_PENDING_BREAK_ENABLE(CpuidRequest->EDX) ? "TRUE" : "FALSE");

        break;

    case 0x2:
        ShowMessages("==== CPUID.(EAX=02H) Legacy Cache Descriptor ====\n\n");
        ShowMessages("  This leaf is deprecated and returns legacy cache information.\n");
        ShowMessages("  Use CPUID.(EAX=04H) for deterministic cache parameters instead.\n\n");
        ShowMessages("  Raw data:\n");
        ShowMessages("  EAX: 0x%08X\n", CpuidRequest->EAX);
        ShowMessages("  EBX: 0x%08X\n", CpuidRequest->EBX);
        ShowMessages("  ECX: 0x%08X\n", CpuidRequest->ECX);
        ShowMessages("  EDX: 0x%08X\n\n", CpuidRequest->EDX);
        break;

    case 0x3:
        ShowMessages("==== CPUID.(EAX=03H) ====\n\n");
        ShowMessages("  This leaf is reserved/not implemented on modern processors.\n");
        ShowMessages("  (Was previously used for Processor Serial Number on older CPUs)\n\n");
        ShowMessages("  Raw data:\n");
        ShowMessages("  EAX: 0x%08X\n", CpuidRequest->EAX);
        ShowMessages("  EBX: 0x%08X\n", CpuidRequest->EBX);
        ShowMessages("  ECX: 0x%08X\n", CpuidRequest->ECX);
        ShowMessages("  EDX: 0x%08X\n\n", CpuidRequest->EDX);
        break;

    case 0x4:
    {
        UINT32 LineSize       = CPUID_EBX_SYSTEM_COHERENCY_LINE_SIZE(CpuidRequest->EBX);
        UINT32 Partitions     = CPUID_EBX_PHYSICAL_LINE_PARTITIONS(CpuidRequest->EBX);
        UINT32 Ways           = CPUID_EBX_WAYS_OF_ASSOCIATIVITY(CpuidRequest->EBX);
        UINT32 Sets           = CPUID_ECX_NUMBER_OF_SETS(CpuidRequest->ECX);
        UINT64 CacheSizeBytes = (UINT64)(Ways + 1) *
                                (UINT64)(Partitions + 1) *
                                (UINT64)(LineSize + 1) *
                                (UINT64)(Sets + 1);
        ShowMessages("==== CPUID.(EAX=04H) Deterministic Cache Parameters ====\n\n");

        ShowMessages("  *******************************************************\n"
                     "  *             Max NumberOfSubLeaves = %u               *\n"
                     "  *******************************************************\n\n",
                     CpuidRequest->Leaf4MaxSubLeaf);

        ShowMessages("---- CPUID.(EAX=04H, ECX=%u) ----\n\n", SubFunctionId);

        //
        // EAX
        //
        switch (CPUID_EAX_CACHE_TYPE_FIELD(CpuidRequest->EAX))
        {
        case 0:
            TypeName = "Null (no more caches)";
            break;
        case 1:
            TypeName = "Data Cache";
            break;
        case 2:
            TypeName = "Instruction Cache";
            break;
        case 3:
            TypeName = "Unified Cache";
            break;
        default:
            TypeName = "Reserved";
            break;
        }

        ShowMessages("-- EAX --\n\n");
        ShowMessages("  CacheTypeField                   = %u (%s)\n",
                     CPUID_EAX_CACHE_TYPE_FIELD(CpuidRequest->EAX),
                     TypeName);

        if (CPUID_EAX_CACHE_TYPE_FIELD(CpuidRequest->EAX) == 0)
        {
            ShowMessages("  (no more caches; stopping enumeration)\n\n");
            break;
        }

        ShowMessages("  CacheLevel                       = %u\n",
                     CPUID_EAX_CACHE_LEVEL(CpuidRequest->EAX));
        ShowMessages("  SelfInitializingCacheLevel       = %s\n",
                     CPUID_EAX_SELF_INITIALIZING_CACHE_LEVEL(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  FullyAssociativeCache            = %s%s\n",
                     CPUID_EAX_FULLY_ASSOCIATIVE_CACHE(CpuidRequest->EAX) ? "TRUE" : "FALSE",
                     CPUID_EAX_FULLY_ASSOCIATIVE_CACHE(CpuidRequest->EAX) ? " (fully associative)" : "");

        ShowMessages("  MaxAddressableIds(LogicalProcs)  (raw) = %u -> actual = %u (raw + 1)\n",
                     CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_SHARING_THIS_CACHE(CpuidRequest->EAX),
                     CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS_SHARING_THIS_CACHE(CpuidRequest->EAX) + 1);
        ShowMessages("  MaxAddressableIds(Cores)         (raw) = %u -> actual = %u (raw + 1)\n\n",
                     CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_PROCESSOR_CORES_IN_PHYSICAL_PACKAGE(CpuidRequest->EAX),
                     CPUID_EAX_MAX_ADDRESSABLE_IDS_FOR_PROCESSOR_CORES_IN_PHYSICAL_PACKAGE(CpuidRequest->EAX) + 1);

        //
        // EBX
        //
        ShowMessages("-- EBX --\n\n");

        ShowMessages("  SystemCoherencyLineSize          (raw) = %u -> actual = %u bytes (raw + 1)\n",
                     LineSize,
                     LineSize + 1);
        ShowMessages("  PhysicalLinePartitions           (raw) = %u -> actual = %u (raw + 1)\n",
                     Partitions,
                     Partitions + 1);
        ShowMessages("  WaysOfAssociativity              (raw) = %u -> actual = %u (raw + 1)\n\n",
                     Ways,
                     Ways + 1);

        //
        // ECX
        //
        ShowMessages("-- ECX --\n\n");

        ShowMessages("  NumberOfSets                     (raw) = %u -> actual = %u (raw + 1)\n\n",
                     Sets,
                     Sets + 1);

        //
        // EDX
        //
        ShowMessages("-- EDX --\n\n");
        ShowMessages("  WriteBackInvalidate              = %s\n",
                     CPUID_EDX_WRITE_BACK_INVALIDATE(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        ShowMessages("  CacheInclusiveness               = %s%s\n",
                     CPUID_EDX_CACHE_INCLUSIVENESS(CpuidRequest->EDX) ? "TRUE" : "FALSE",
                     CPUID_EDX_CACHE_INCLUSIVENESS(CpuidRequest->EDX) ? " (inclusive of lower levels)" : "");
        ShowMessages("  ComplexCacheIndexing             = %s%s\n\n",
                     CPUID_EDX_COMPLEX_CACHE_INDEXING(CpuidRequest->EDX) ? "TRUE" : "FALSE",
                     CPUID_EDX_COMPLEX_CACHE_INDEXING(CpuidRequest->EDX) ? " (complex/hashed indexing)" : " (direct mapped)");

        //
        // Cache Size Calculation (from spec formula)
        //
        ShowMessages("-- Cache Size --\n\n");
        ShowMessages("  Cache Size (per spec formula)    = %llu bytes (%llu KB, %llu MB)\n\n",
                     (ULONG64)CacheSizeBytes,
                     (ULONG64)(CacheSizeBytes / 1024),
                     (ULONG64)(CacheSizeBytes / (1024 * 1024)));

        break;
    }
    case 0x5:
        ShowMessages("==== CPUID.(EAX=05H) MONITOR/MWAIT Leaf ====\n\n");

        ShowMessages("  *******************************************************\n"
                     "  *               LEAF 5 HAS NO SUBLEAVES               *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        //
        // EAX
        //
        ShowMessages("-- EAX --\n\n");
        ShowMessages("  SmallestMonitorLineSize = %u bytes\n\n",
                     CPUID_EAX_SMALLEST_MONITOR_LINE_SIZE(CpuidRequest->EAX));

        //
        // EBX
        //
        ShowMessages("-- EBX --\n\n");
        ShowMessages("  LargestMonitorLineSize  = %u bytes\n\n",
                     CPUID_EBX_LARGEST_MONITOR_LINE_SIZE(CpuidRequest->EBX));

        //
        // ECX
        //
        ShowMessages("-- ECX --\n\n");
        ShowMessages("  EnumerationOfMonitorMwaitExtensions             = %s\n",
                     CPUID_ECX_ENUMERATION_OF_MONITOR_MWAIT_EXTENSIONS(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  SupportsTreatingInterruptsAsBreakEventForMwait  = %s\n\n",
                     CPUID_ECX_SUPPORTS_TREATING_INTERRUPTS_AS_BREAK_EVENT_FOR_MWAIT(CpuidRequest->ECX) ? "TRUE" : "FALSE");

        //
        // EDX
        //
        ShowMessages("-- EDX --\n\n");
        ShowMessages("  NumberOfC0SubCStates = %u\n", CPUID_EDX_NUMBER_OF_C0_SUB_C_STATES(CpuidRequest->EDX));
        ShowMessages("  NumberOfC1SubCStates = %u\n", CPUID_EDX_NUMBER_OF_C1_SUB_C_STATES(CpuidRequest->EDX));
        ShowMessages("  NumberOfC2SubCStates = %u\n", CPUID_EDX_NUMBER_OF_C2_SUB_C_STATES(CpuidRequest->EDX));
        ShowMessages("  NumberOfC3SubCStates = %u\n", CPUID_EDX_NUMBER_OF_C3_SUB_C_STATES(CpuidRequest->EDX));
        ShowMessages("  NumberOfC4SubCStates = %u\n", CPUID_EDX_NUMBER_OF_C4_SUB_C_STATES(CpuidRequest->EDX));
        ShowMessages("  NumberOfC5SubCStates = %u\n", CPUID_EDX_NUMBER_OF_C5_SUB_C_STATES(CpuidRequest->EDX));
        ShowMessages("  NumberOfC6SubCStates = %u\n", CPUID_EDX_NUMBER_OF_C6_SUB_C_STATES(CpuidRequest->EDX));
        ShowMessages("  NumberOfC7SubCStates = %u\n\n", CPUID_EDX_NUMBER_OF_C7_SUB_C_STATES(CpuidRequest->EDX));

        break;

    case 0x6:
        ShowMessages("==== CPUID.(EAX=06H) Thermal and Power Management Leaf ====\n\n");

        ShowMessages("  *******************************************************\n"
                     "  *               LEAF 6 HAS NO SUBLEAVES               *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        //
        // EAX
        //
        ShowMessages("-- EAX --\n\n");
        ShowMessages("  TemperatureSensorSupported              = %s\n",
                     CPUID_EAX_TEMPERATURE_SENSOR_SUPPORTED(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  IntelTurboBoostTechnologyAvailable       = %s\n",
                     CPUID_EAX_INTEL_TURBO_BOOST_TECHNOLOGY_AVAILABLE(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  ARAT (ApicTimerAlwaysRunning)            = %s\n",
                     CPUID_EAX_APIC_TIMER_ALWAYS_RUNNING(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  PLN (PowerLimitNotification)             = %s\n",
                     CPUID_EAX_POWER_LIMIT_NOTIFICATION(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  ECMD (ClockModulationDuty)               = %s\n",
                     CPUID_EAX_CLOCK_MODULATION_DUTY(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  PTM (PackageThermalManagement)           = %s\n",
                     CPUID_EAX_PACKAGE_THERMAL_MANAGEMENT(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  HWP Base Registers                       = %s\n",
                     CPUID_EAX_HWP_BASE_REGISTERS(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  HWP_Notification                         = %s\n",
                     CPUID_EAX_HWP_NOTIFICATION(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  HWP_Activity_Window                      = %s\n",
                     CPUID_EAX_HWP_ACTIVITY_WINDOW(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  HWP_Energy_Performance_Preference        = %s\n",
                     CPUID_EAX_HWP_ENERGY_PERFORMANCE_PREFERENCE(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  HWP_Package_Level_Request                = %s\n",
                     CPUID_EAX_HWP_PACKAGE_LEVEL_REQUEST(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  HDC                                      = %s\n",
                     CPUID_EAX_HDC(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  Intel Turbo Boost Max Technology 3.0     = %s\n",
                     CPUID_EAX_INTEL_TURBO_BOOST_MAX_TECHNOLOGY_3_AVAILABLE(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  HWP Capabilities                         = %s\n",
                     CPUID_EAX_HWP_CAPABILITIES(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  HWP PECI Override                        = %s\n",
                     CPUID_EAX_HWP_PECI_OVERRIDE(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  Flexible HWP                             = %s\n",
                     CPUID_EAX_FLEXIBLE_HWP(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  Fast Access Mode for HWP Request MSR     = %s\n",
                     CPUID_EAX_FAST_ACCESS_MODE_FOR_HWP_REQUEST_MSR(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  Ignoring Idle Logical Proc HWP Request   = %s\n",
                     CPUID_EAX_IGNORING_IDLE_LOGICAL_PROCESSOR_HWP_REQUEST(CpuidRequest->EAX) ? "TRUE" : "FALSE");
        ShowMessages("  Intel Thread Director                    = %s\n\n",
                     CPUID_EAX_INTEL_THREAD_DIRECTOR(CpuidRequest->EAX) ? "TRUE" : "FALSE");

        //
        // EBX
        //
        ShowMessages("-- EBX --\n\n");
        ShowMessages("  NumberOfInterruptThresholdsInThermalSensor = %u\n\n",
                     CPUID_EBX_NUMBER_OF_INTERRUPT_THRESHOLDS_IN_THERMAL_SENSOR(CpuidRequest->EBX));

        //
        // ECX
        //
        ShowMessages("-- ECX --\n\n");
        ShowMessages("  HardwareCoordinationFeedbackCapability   = %s\n",
                     CPUID_ECX_HARDWARE_COORDINATION_FEEDBACK_CAPABILITY(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  NumberOfIntelThreadDirectorClasses (bit) = %s\n",
                     CPUID_ECX_NUMBER_OF_INTEL_THREAD_DIRECTOR_CLASSES(CpuidRequest->ECX) ? "TRUE" : "FALSE");
        ShowMessages("  PerformanceEnergyBiasPreference          = %u\n\n",
                     CPUID_ECX_PERFORMANCE_ENERGY_BIAS_PREFERENCE(CpuidRequest->ECX));

        //
        // EDX
        // EDX is fully reserved for this leaf; still routed through its macro for consistency.
        //
        ShowMessages("-- EDX --\n\n");
        ShowMessages("  Reserved                                 = 0x%08X\n\n", CPUID_EDX_RESERVED(CpuidRequest->EDX));

        break;

    case 0x7:
        ShowMessages("==== CPUID.(EAX=07H) Structured Extended Feature Flags ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *             Max NumberOfSubLeaves = %u               *\n"
                     "  *******************************************************\n\n",
                     CpuidRequest->LeafEaxMaxSubleaf);

        if (SubFunctionId == 0)
        {
            ShowMessages("---- CPUID.(EAX=07H, ECX=%u) ----\n\n", SubFunctionId);

            //
            // EAX
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  NumberOfSubLeaves (max ECX input) = %u\n\n", CPUID_EAX_NUMBER_OF_SUB_LEAVES(CpuidRequest->EAX));

            //
            // EBX
            //
            ShowMessages("-- EBX --\n\n");
            ShowMessages("  FSGSBASE                       = %s\n", CPUID_EBX_FSGSBASE(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  IA32_TSC_ADJUST MSR            = %s\n", CPUID_EBX_IA32_TSC_ADJUST_MSR(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  SGX                            = %s\n", CPUID_EBX_SGX(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  BMI1                           = %s\n", CPUID_EBX_BMI1(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  HLE                            = %s\n", CPUID_EBX_HLE(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX2                           = %s\n", CPUID_EBX_AVX2(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  FDP_EXCPTN_ONLY                = %s\n", CPUID_EBX_FDP_EXCPTN_ONLY(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  SMEP                           = %s\n", CPUID_EBX_SMEP(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  BMI2                           = %s\n", CPUID_EBX_BMI2(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  Enhanced REP MOVSB/STOSB       = %s\n", CPUID_EBX_ENHANCED_REP_MOVSB_STOSB(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  INVPCID                        = %s\n", CPUID_EBX_INVPCID(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  RTM                            = %s\n", CPUID_EBX_RTM(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  RDT-M (Monitoring)             = %s\n", CPUID_EBX_RDT_M(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  Deprecates FPU CS/DS           = %s\n", CPUID_EBX_DEPRECATES(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  MPX                            = %s\n", CPUID_EBX_MPX(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  RDT-A (Allocation)             = %s\n", CPUID_EBX_RDT(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512F                        = %s\n", CPUID_EBX_AVX512F(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512DQ                       = %s\n", CPUID_EBX_AVX512DQ(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  RDSEED                         = %s\n", CPUID_EBX_RDSEED(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  ADX                            = %s\n", CPUID_EBX_ADX(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  SMAP                           = %s\n", CPUID_EBX_SMAP(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512_IFMA                    = %s\n", CPUID_EBX_AVX512_IFMA(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  CLFLUSHOPT                     = %s\n", CPUID_EBX_CLFLUSHOPT(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  CLWB                           = %s\n", CPUID_EBX_CLWB(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  Intel Processor Trace          = %s\n", CPUID_EBX_INTEL(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512PF (Xeon Phi only)       = %s\n", CPUID_EBX_AVX512PF(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512ER (Xeon Phi only)       = %s\n", CPUID_EBX_AVX512ER(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512CD                       = %s\n", CPUID_EBX_AVX512CD(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  SHA                            = %s\n", CPUID_EBX_SHA(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512BW                       = %s\n", CPUID_EBX_AVX512BW(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512VL                       = %s\n\n", CPUID_EBX_AVX512VL(CpuidRequest->EBX) ? "TRUE" : "FALSE");

            //
            // ECX
            //
            ShowMessages("-- ECX --\n\n");
            ShowMessages("  PREFETCHWT1 (Xeon Phi only)    = %s\n", CPUID_ECX_PREFETCHWT1(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512_VBMI                    = %s\n", CPUID_ECX_AVX512_VBMI(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  UMIP                           = %s\n", CPUID_ECX_UMIP(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  PKU                            = %s\n", CPUID_ECX_PKU(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  OSPKE                          = %s\n", CPUID_ECX_OSPKE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  WAITPKG                        = %s\n", CPUID_ECX_WAITPKG(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512_VBMI2                   = %s\n", CPUID_ECX_AVX512_VBMI2(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  CET_SS (shadow stack)          = %s\n", CPUID_ECX_CET_SS(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  GFNI                           = %s\n", CPUID_ECX_GFNI(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  VAES                           = %s\n", CPUID_ECX_VAES(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  VPCLMULQDQ                     = %s\n", CPUID_ECX_VPCLMULQDQ(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512_VNNI                    = %s\n", CPUID_ECX_AVX512_VNNI(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512_BITALG                  = %s\n", CPUID_ECX_AVX512_BITALG(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  TME_EN                         = %s\n", CPUID_ECX_TME_EN(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512_VPOPCNTDQ               = %s\n", CPUID_ECX_AVX512_VPOPCNTDQ(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  LA57 (5-level paging)          = %s\n", CPUID_ECX_LA57(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  MAWAU (BNDLDX/BNDSTX)          = %u (NOT BOOLEAN)\n", CPUID_ECX_MAWAU(CpuidRequest->ECX));
            ShowMessages("  RDPID                          = %s\n", CPUID_ECX_RDPID(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  KL (Key Locker)                = %s\n", CPUID_ECX_KL(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  CLDEMOTE                       = %s\n", CPUID_ECX_CLDEMOTE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  MOVDIRI                        = %s\n", CPUID_ECX_MOVDIRI(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  MOVDIR64B                      = %s\n", CPUID_ECX_MOVDIR64B(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  SGX_LC (Launch Config)         = %s\n", CPUID_ECX_SGX_LC(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  PKS                            = %s\n\n", CPUID_ECX_PKS(CpuidRequest->ECX) ? "TRUE" : "FALSE");

            //
            // EDX
            //
            ShowMessages("-- EDX --\n\n");
            ShowMessages("  AVX512_4VNNIW (Xeon Phi only)  = %s\n", CPUID_EDX_AVX512_4VNNIW(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512_4FMAPS (Xeon Phi only)  = %s\n", CPUID_EDX_AVX512_4FMAPS(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  Fast Short REP MOV             = %s\n", CPUID_EDX_FAST_SHORT_REP_MOV(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  AVX512_VP2INTERSECT            = %s\n", CPUID_EDX_AVX512_VP2INTERSECT(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  MD_CLEAR                       = %s\n", CPUID_EDX_MD_CLEAR(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  SERIALIZE                      = %s\n", CPUID_EDX_SERIALIZE(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  Hybrid part                    = %s\n", CPUID_EDX_HYBRID(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  PCONFIG                        = %s\n", CPUID_EDX_PCONFIG(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  CET_IBT (branch tracking)      = %s\n", CPUID_EDX_CET_IBT(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  IBRS/IBPB                      = %s\n", CPUID_EDX_IBRS_IBPB(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  STIBP                          = %s\n", CPUID_EDX_STIBP(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  L1D_FLUSH                      = %s\n", CPUID_EDX_L1D_FLUSH(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  IA32_ARCH_CAPABILITIES MSR     = %s\n", CPUID_EDX_IA32_ARCH_CAPABILITIES(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  IA32_CORE_CAPABILITIES MSR     = %s\n", CPUID_EDX_IA32_CORE_CAPABILITIES(CpuidRequest->EDX) ? "TRUE" : "FALSE");
            ShowMessages("  SSBD                           = %s\n\n", CPUID_EDX_SSBD(CpuidRequest->EDX) ? "TRUE" : "FALSE");
        }
        else
        {
            //
            // This header only defines the EBX/ECX/EDX bitfield layout for sub-leaf
            // (ECX) = 0. If the processor reports further sub-leaves, decoding them
            // with the sub-leaf-0 struct would silently misinterpret the bits, so
            // instead their raw dwords are printed undecoded - consistent with this
            // file's rule of only reading a field through a macro that's actually
            // defined for it.
            //
            ShowMessages("  No bitfield macros are defined for these in ia32.h, so their\n");
            ShowMessages("  raw dwords are shown without decoding:\n");
            ShowMessages("    ECX=%u: EAX=0x%08X EBX=0x%08X ECX=0x%08X EDX=0x%08X\n\n",
                         SubFunctionId,
                         CpuidRequest->EAX,
                         CpuidRequest->EBX,
                         CpuidRequest->ECX,
                         CpuidRequest->EDX);
        }

        break;

    case 0x8:
        ShowMessages("==== CPUID.(EAX=08H) ====\n\n");
        ShowMessages("  This leaf is reserved/not implemented on modern processors.\n");
        ShowMessages("  (Was previously used for Processor Serial Number on some CPUs)\n\n");
        ShowMessages("  Raw data:\n");
        ShowMessages("  EAX: 0x%08X\n", CpuidRequest->EAX);
        ShowMessages("  EBX: 0x%08X\n", CpuidRequest->EBX);
        ShowMessages("  ECX: 0x%08X\n", CpuidRequest->ECX);
        ShowMessages("  EDX: 0x%08X\n", CpuidRequest->EDX);
        break;

    case 0x9:
        ShowMessages("==== CPUID.(EAX=09H) Direct Cache Access Information ====\n\n");

        ShowMessages("  *******************************************************\n"
                     "  *               LEAF 9 HAS NO SUBLEAVES               *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        //
        // EAX mirrors IA32_PLATFORM_DCA_CAP[31:0] verbatim. ia32.h doesn't split
        // it into sub-fields here (those bit meanings live in the MSR's own
        // spec, not in this CPUID leaf), so it's read through its single
        // whole-dword macro and shown as raw hex rather than decoded further.
        //
        ShowMessages("  IA32_PLATFORM_DCA_CAP (EAX, mirrors MSR 1F8H) = 0x%08X\n",
                     CPUID_EAX_IA32_PLATFORM_DCA_CAP(CpuidRequest->EAX));

        //
        // EBX/ECX/EDX are fully reserved for this leaf.
        //
        ShowMessages("  Reserved (EBX) = 0x%08X\n", CPUID_EBX_RESERVED(CpuidRequest->EBX));
        ShowMessages("  Reserved (ECX) = 0x%08X\n", CPUID_ECX_RESERVED(CpuidRequest->ECX));
        ShowMessages("  Reserved (EDX) = 0x%08X\n\n", CPUID_EDX_RESERVED(CpuidRequest->EDX));

        break;

    case 0xA:
        ShowMessages("==== CPUID.(EAX=0AH) Architectural Performance Monitoring Leaf ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *               LEAF 10 HAS NO SUBLEAVES              *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        if (CPUID_EAX_VERSION_ID_OF_ARCHITECTURAL_PERFORMANCE_MONITORING(CpuidRequest->EAX) == 0)
        {
            //
            // Per the spec: architectural perfmon is only supported if VersionId > 0.
            // At version 0 the rest of this leaf isn't architecturally meaningful, so stop here
            //
            ShowMessages("  VersionId = 0 -> architectural performance monitoring not supported;\n"
                         "  remaining fields in this leaf are not architecturally defined.\n\n");

            break;
        }

        //
        // EAX
        //
        ShowMessages("-- EAX --\n\n");
        ShowMessages("  VersionId                               = %u\n",
                     CPUID_EAX_VERSION_ID_OF_ARCHITECTURAL_PERFORMANCE_MONITORING(CpuidRequest->EAX));
        ShowMessages("  NumberOfCountersPerLogicalProcessor     = %u\n",
                     CPUID_EAX_NUMBER_OF_PERFORMANCE_MONITORING_COUNTER_PER_LOGICAL_PROCESSOR(CpuidRequest->EAX));
        ShowMessages("  BitWidthOfPerformanceMonitoringCounter  = %u\n",
                     CPUID_EAX_BIT_WIDTH_OF_PERFORMANCE_MONITORING_COUNTER(CpuidRequest->EAX));
        ShowMessages("  EbxBitVectorLength                      = %u\n\n",
                     CPUID_EAX_EBX_BIT_VECTOR_LENGTH(CpuidRequest->EAX));

        //
        // EBX
        //
        ShowMessages("-- EBX (bit = 1 means the event is NOT available) --\n\n");
        ShowMessages("  CoreCycleEventNotAvailable                = %u\n",
                     CPUID_EBX_CORE_CYCLE_EVENT_NOT_AVAILABLE(CpuidRequest->EBX));
        ShowMessages("  InstructionRetiredEventNotAvailable       = %u\n",
                     CPUID_EBX_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE(CpuidRequest->EBX));
        ShowMessages("  ReferenceCyclesEventNotAvailable          = %u\n",
                     CPUID_EBX_REFERENCE_CYCLES_EVENT_NOT_AVAILABLE(CpuidRequest->EBX));
        ShowMessages("  LastLevelCacheReferenceEventNotAvailable  = %u\n",
                     CPUID_EBX_LAST_LEVEL_CACHE_REFERENCE_EVENT_NOT_AVAILABLE(CpuidRequest->EBX));
        ShowMessages("  LastLevelCacheMissesEventNotAvailable     = %u\n",
                     CPUID_EBX_LAST_LEVEL_CACHE_MISSES_EVENT_NOT_AVAILABLE(CpuidRequest->EBX));
        ShowMessages("  BranchInstructionRetiredEventNotAvailable = %u\n",
                     CPUID_EBX_BRANCH_INSTRUCTION_RETIRED_EVENT_NOT_AVAILABLE(CpuidRequest->EBX));
        ShowMessages("  BranchMispredictRetiredEventNotAvailable  = %u\n\n",
                     CPUID_EBX_BRANCH_MISPREDICT_RETIRED_EVENT_NOT_AVAILABLE(CpuidRequest->EBX));

        if (CPUID_EAX_EBX_BIT_VECTOR_LENGTH(CpuidRequest->EAX) < 7)
        {
            ShowMessages("  NOTE: EbxBitVectorLength=%u (<7): bits at/after this length are not\n"
                         "        architecturally defined on this CPU; treat them as unreliable.\n",
                         CPUID_EAX_EBX_BIT_VECTOR_LENGTH(CpuidRequest->EAX));
        }

        //
        // ECX
        //
        ShowMessages("-- ECX --\n\n");
        ShowMessages("  Reserved                                   = 0x%08X\n\n",
                     CPUID_ECX_RESERVED(CpuidRequest->ECX));

        //
        // EDX: fixed-function counter fields only defined if VersionId > 1
        //
        ShowMessages("-- EDX --\n\n");
        if (CPUID_EAX_VERSION_ID_OF_ARCHITECTURAL_PERFORMANCE_MONITORING(CpuidRequest->EAX) > 1)
        {
            ShowMessages("  NumberOfFixedFunctionPerformanceCounters   = %u\n",
                         CPUID_EDX_NUMBER_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS(CpuidRequest->EDX));
            ShowMessages("  BitWidthOfFixedFunctionPerformanceCounters = %u\n\n",
                         CPUID_EDX_BIT_WIDTH_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS(CpuidRequest->EDX));
        }
        else
        {
            ShowMessages("  NOTE: VersionId=%u (<=1): fixed-function counter fields are not\n",
                         CPUID_EAX_VERSION_ID_OF_ARCHITECTURAL_PERFORMANCE_MONITORING(CpuidRequest->EAX));
            ShowMessages("        architecturally defined; showing raw macro output anyway:\n");
            ShowMessages("  NumberOfFixedFunctionPerformanceCounters   = %u (undefined)\n",
                         CPUID_EDX_NUMBER_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS(CpuidRequest->EDX));
            ShowMessages("  BitWidthOfFixedFunctionPerformanceCounters = %u (undefined)\n",
                         CPUID_EDX_BIT_WIDTH_OF_FIXED_FUNCTION_PERFORMANCE_COUNTERS(CpuidRequest->EDX));
        }

        ShowMessages("  AnyThreadDeprecation                       = %s\n\n",
                     CPUID_EDX_ANY_THREAD_DEPRECATION(CpuidRequest->EDX) ? "TRUE" : "FALSE");

        break;

    case 0xB:

        //
        // Check if this leaf is supported or not
        //
        if (!CpuidRequest->LeafBSupported)
        {
            ShowMessages("==== CPUID.(EAX=0BH) Extended Topology Enumeration ====\n");
            ShowMessages("  Leaf presence check failed: CPUID.0BH:EBX[15:0] == 0 at sub-leaf 0.\n"
                         "  This processor does not implement leaf 0BH (consider leaf 1FH instead).\n\n");
            break;
        }
        ShowMessages("==== CPUID.(EAX=0BH) Extended Topology Enumeration ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *             Max NumberOfSubLeaves = %u               *\n"
                     "  *******************************************************\n\n",
                     CpuidRequest->LeafBMaxSubleaf);

        switch (CPUID_ECX_LEVEL_TYPE(CpuidRequest->ECX))
        {
        case 0:
            TypeName = "Invalid";
            break;
        case 1:
            TypeName = "SMT (Hyper-Threading)";
            break;
        case 2:
            TypeName = "Core";
            break;
        default:
            TypeName = "Reserved";
            break;
        }

        ShowMessages("---- CPUID.(EAX=0BH, ECX=%u) ----\n\n", SubFunctionId);

        //
        // ECX: Level number and type
        //
        ShowMessages("-- ECX --\n\n");
        ShowMessages("  LevelNumber (echoes ECX input) = %u\n",
                     CPUID_ECX_LEVEL_NUMBER(CpuidRequest->ECX));
        ShowMessages("  LevelType                      = %u (%s)\n\n",
                     CPUID_ECX_LEVEL_TYPE(CpuidRequest->ECX),
                     TypeName);
        //
        // EAX: Shift count
        //
        ShowMessages("-- EAX --\n\n");
        ShowMessages("  X2ApicIdToUniqueTopologyIdShift = %u\n\n",
                     CPUID_EAX_X2APIC_ID_TO_UNIQUE_TOPOLOGY_ID_SHIFT(CpuidRequest->EAX));

        //
        // EBX: Number of logical processors (diagnostic only)
        //
        ShowMessages("-- EBX --\n\n");
        ShowMessages("  NumberOfLogicalProcessorsAtThisLevelType = %u (DIAGNOSTIC ONLY - do not use for topology enumeration)\n\n",
                     CPUID_EBX_NUMBER_OF_LOGICAL_PROCESSORS_AT_THIS_LEVEL_TYPE(CpuidRequest->EBX));

        //
        // EDX: x2APIC ID (constant across all sub-leaves)
        //
        ShowMessages("-- EDX --\n\n");
        ShowMessages("  X2ApicId (current logical processor) = %u\n\n",
                     CPUID_EDX_X2APIC_ID(CpuidRequest->EDX));

        break;

    case 0xC:
        ShowMessages("==== CPUID.(EAX=0CH) ====\n\n");
        ShowMessages("  This leaf is reserved (not implemented).\n\n");
        ShowMessages("  Raw data:\n");
        ShowMessages("  EAX: 0x%08X\n", CpuidRequest->EAX);
        ShowMessages("  EBX: 0x%08X\n", CpuidRequest->EBX);
        ShowMessages("  ECX: 0x%08X\n", CpuidRequest->ECX);
        ShowMessages("  EDX: 0x%08X\n\n", CpuidRequest->EDX);
        break;

    case 0xD:
        ShowMessages("==== CPUID.(EAX=0DH) Processor Extended State Enumeration ====\n");
        ShowMessages("  *******************************************************\n"
                     "  *             Max NumberOfSubLeaves = 62              *\n"
                     "  *******************************************************\n\n");

        if (SubFunctionId == 0)
        {
            ShowMessages("-- EAX (XCR0 lower 32 bits) --\n\n");
            ShowMessages("  X87State (bit 0)               = %s\n", CPUID_EAX_X87_STATE(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  SSEState (bit 1)               = %s\n", CPUID_EAX_SSE_STATE(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  AVXState (bit 2)               = %s\n", CPUID_EAX_AVX_STATE(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  MPXState (bits 4:3)            = %u\n", CPUID_EAX_MPX_STATE(CpuidRequest->EAX));
            ShowMessages("  AVX512State (bits 7:5)         = %u\n", CPUID_EAX_AVX_512_STATE(CpuidRequest->EAX));
            ShowMessages("  UsedForIa32Xss1 (bit 8)        = %s\n", CPUID_EAX_USED_FOR_IA32_XSS_1(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  PKRUState (bit 9)              = %s\n", CPUID_EAX_PKRU_STATE(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  UsedForIa32Xss2 (bit 13)       = %s\n\n", CPUID_EAX_USED_FOR_IA32_XSS_2(CpuidRequest->EAX) ? "TRUE" : "FALSE");

            ShowMessages("-- EBX --\n\n");
            ShowMessages("  MaxSizeRequiredByEnabledFeaturesInXcr0 = %u bytes\n\n",
                         CPUID_EBX_MAX_SIZE_REQUIRED_BY_ENABLED_FEATURES_IN_XCR0(CpuidRequest->EBX));

            ShowMessages("-- ECX --\n\n");
            ShowMessages("  MaxSizeOfXsaveXrstorSaveArea           = %u bytes\n\n",
                         CPUID_ECX_MAX_SIZE_OF_XSAVE_XRSTOR_SAVE_AREA(CpuidRequest->ECX));
            ShowMessages("-- EDX (XCR0 upper 32 bits) --\n\n");
            ShowMessages("  Xcr0SupportedBits (bits 63:32 of XCR0) = 0x%08X\n\n",
                         CPUID_EDX_XCR0_SUPPORTED_BITS(CpuidRequest->EDX));
        }

        else if (SubFunctionId == 1)
        {
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  SupportsXsavecAndCompactedXrstor (XSAVEC) = %s\n",
                         CPUID_EAX_SUPPORTS_XSAVEC_AND_COMPACTED_XRSTOR(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  SupportsXgetbvWithEcx1                     = %s\n",
                         CPUID_EAX_SUPPORTS_XGETBV_WITH_ECX_1(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  SupportsXsaveXrstorAndIa32Xss (XSAVES)     = %s\n\n",
                         CPUID_EAX_SUPPORTS_XSAVE_XRSTOR_AND_IA32_XSS(CpuidRequest->EAX) ? "TRUE" : "FALSE");

            ShowMessages("-- EBX --\n\n");
            ShowMessages("  SizeOfXsaveArea (XCR0 | IA32_XSS enabled) = %u bytes\n",
                         CPUID_EBX_SIZE_OF_XSAVE_AREAD(CpuidRequest->EBX));

            ShowMessages("-- ECX (IA32_XSS lower 32 bits) --\n\n");
            ShowMessages("  UsedForXcr01 (bits 7:0)              = 0x%02X\n",
                         CPUID_ECX_USED_FOR_XCR0_1(CpuidRequest->ECX));
            ShowMessages("  PtState (bit 8)                      = %s\n",
                         CPUID_ECX_PT_STATE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  UsedForXcr02 (bit 9)                 = %s\n",
                         CPUID_ECX_USED_FOR_XCR0_2(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  CetUserState (bit 11)                = %s\n",
                         CPUID_ECX_CET_USER_STATE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  CetSupervisorState (bit 12)          = %s\n",
                         CPUID_ECX_CET_SUPERVISOR_STATE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  HdcState (bit 13)                    = %s\n",
                         CPUID_ECX_HDC_STATE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  LbrState (bit 15)                    = %s\n",
                         CPUID_ECX_LBR_STATE(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  HwpState (bit 16)                    = %s\n\n",
                         CPUID_ECX_HWP_STATE(CpuidRequest->ECX) ? "TRUE" : "FALSE");

            ShowMessages("-- EDX (IA32_XSS upper 32 bits) --\n\n");
            ShowMessages("  SupportedUpperIa32XssBits (bits 63:32) = 0x%08X\n\n",
                         CPUID_EDX_SUPPORTED_UPPER_IA32_XSS_BITS(CpuidRequest->EDX));
        }

        else if (SubFunctionId >= 2)
        {
            //
            // Check if the user input is valid
            // Need to check both XCR0 and IA32_XSS vectors
            //
            UINT64 ValidBits = CpuidRequest->XCR0Vector | CpuidRequest->IA32_XSS_Vector;

            //
            // Validate if this sub-leaf is supported
            //
            if (((ValidBits >> SubFunctionId) & (UINT64)1) == 0)
            {
                ShowMessages("  Sub-leaf %u is NOT supported on this CPU\n", SubFunctionId);
                ShowMessages("  Valid sub-leaves are those with bits set in:\n");
                ShowMessages("    XCR0 vector:     0x%016llX\n", (ULONG64)CpuidRequest->XCR0Vector);
                ShowMessages("    IA32_XSS vector: 0x%016llX\n", (ULONG64)CpuidRequest->IA32_XSS_Vector);
                ShowMessages("    Combined vector: 0x%016llX\n\n", (ULONG64)ValidBits);

                break;
            }

            ShowMessages("-- State Component Information --\n\n");
            ShowMessages("  SaveAreaSize (EAX)   = %u bytes\n",
                         CPUID_EAX_IA32_PLATFORM_DCA_CAP(CpuidRequest->EAX));
            ShowMessages("  SaveAreaOffset (EBX) = %u bytes\n",
                         CPUID_EBX_RESERVED(CpuidRequest->EBX));
            ShowMessages("  ManagedViaIa32Xss (ECX bit 0) = %u (%s)\n",
                         CPUID_ECX_ECX_2(CpuidRequest->ECX),
                         CPUID_ECX_ECX_2(CpuidRequest->ECX) ? "IA32_XSS" : "XCR0");
            ShowMessages("  Aligned64ByteBoundary (ECX bit 1) = %u%s\n",
                         CPUID_ECX_ECX_1(CpuidRequest->ECX),
                         CPUID_ECX_ECX_1(CpuidRequest->ECX) ? " (next 64-byte boundary)" : " (immediately following)");
            ShowMessages("  Reserved (EDX) = 0x%08X\n\n",
                         CPUID_EDX_RESERVED(CpuidRequest->EDX));
        }

        break;

    case 0xE:
        ShowMessages("==== CPUID.(EAX=0EH) ====\n\n");
        ShowMessages("  This leaf is reserved (not implemented).\n\n");
        ShowMessages("  Raw data:\n");
        ShowMessages("  EAX: 0x%08X\n", CpuidRequest->EAX);
        ShowMessages("  EBX: 0x%08X\n", CpuidRequest->EBX);
        ShowMessages("  ECX: 0x%08X\n", CpuidRequest->ECX);
        ShowMessages("  EDX: 0x%08X\n\n", CpuidRequest->EDX);
        break;

    case 0xF: // 0xF = 15 (decimal); CPUID_INTEL_RESOURCE_DIRECTOR_TECHNOLOGY_MONITORING_INFORMATION
        ShowMessages("==== CPUID.(EAX=0FH) Intel RDT Monitoring ====\n\n");
        ShowMessages("  This leaf is not yet implemented in HyperDbg.\n");
        ShowMessages("  (Intel Resource Director Technology - Monitoring)\n\n");
        ShowMessages("  Raw data:\n");
        ShowMessages("  EAX: 0x%08X\n", CpuidRequest->EAX);
        ShowMessages("  EBX: 0x%08X\n", CpuidRequest->EBX);
        ShowMessages("  ECX: 0x%08X\n", CpuidRequest->ECX);
        ShowMessages("  EDX: 0x%08X\n\n", CpuidRequest->EDX);
        break;

    case 0x10:
        ShowMessages("==== CPUID.(EAX=10H, ECX=%u) Intel RDT Allocation Information ====\n\n", SubFunctionId);
        ShowMessages("  *******************************************************\n"
                     "  *             Max NumberOfSubLeaves = 3               *\n"
                     "  *******************************************************\n\n");

        if (SubFunctionId == 0)
        {
            //
            // Resource type enumeration
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  Ia32PlatformDcaCap = 0x%08X\n\n",
                         CPUID_EAX_IA32_PLATFORM_DCA_CAP(CpuidRequest->EAX));

            //
            // EBX
            //
            ShowMessages("-- EBX (Supported Allocation Types) --\n\n");
            ShowMessages("  Raw value = 0x%08X\n", CpuidRequest->EBX);
            ShowMessages("  L3 Cache Allocation (bit 1) = %s\n",
                         CPUID_EBX_SUPPORTS_L3_CACHE_ALLOCATION_TECHNOLOGY(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  L2 Cache Allocation (bit 2) = %s\n",
                         CPUID_EBX_SUPPORTS_L2_CACHE_ALLOCATION_TECHNOLOGY(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  Memory Bandwidth Allocation (bit 3) = %s\n\n",
                         CPUID_EBX_SUPPORTS_MEMORY_BANDWIDTH_ALLOCATION(CpuidRequest->EBX) ? "TRUE" : "FALSE");

            //
            // ECX
            //
            ShowMessages("-- ECX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n", CPUID_ECX_RESERVED(CpuidRequest->ECX));

            //
            // EDX
            //
            ShowMessages("-- EDX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EDX_RESERVED(CpuidRequest->EDX));
        }

        else if (SubFunctionId == 1)
        {
            //
            // L3 cache allocation
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  LengthOfCapacityBitMask (minus-one) = %u (actual = %u bits)\n\n",
                         CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK(CpuidRequest->EAX),
                         CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK(CpuidRequest->EAX) + 1);

            ShowMessages("-- EBX --\n\n");
            ShowMessages("  Bit-granular map = 0x%08X\n\n", CPUID_EBX_EBX_0(CpuidRequest->EBX));

            ShowMessages("-- ECX --\n\n");
            ShowMessages("  CodeAndDataPriorizationTechnologySupported = %s%s\n\n",
                         CPUID_ECX_CODE_AND_DATA_PRIORIZATION_TECHNOLOGY_SUPPORTED(CpuidRequest->ECX) ? "TRUE" : "FALSE",
                         CPUID_ECX_CODE_AND_DATA_PRIORIZATION_TECHNOLOGY_SUPPORTED(CpuidRequest->ECX) ? " (CDP supported)" : "");

            ShowMessages("-- EDX --\n\n");
            ShowMessages("  HighestCosNumberSupported = %u (actual = %u COS)\n\n",
                         CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED(CpuidRequest->EDX),
                         CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED(CpuidRequest->EDX) + 1);
        }

        else if (SubFunctionId == 2)
        {
            //
            // Sub-leaf 2 (L2 Cache Allocation)
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  LengthOfCapacityBitMask (minus-one) = %u (actual = %u bits)\n\n",
                         CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK(CpuidRequest->EAX),
                         CPUID_EAX_LENGTH_OF_CAPACITY_BIT_MASK(CpuidRequest->EAX) + 1);

            ShowMessages("-- EBX --\n\n");
            ShowMessages("  Bit-granular map = 0x%08X\n\n", CPUID_EBX_EBX_0(CpuidRequest->EBX));

            ShowMessages("-- ECX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n", CPUID_ECX_RESERVED(CpuidRequest->ECX));

            ShowMessages("-- EDX --\n\n");
            ShowMessages("  HighestCosNumberSupported = %u (actual = %u COS)\n\n",
                         CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED(CpuidRequest->EDX),
                         CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED(CpuidRequest->EDX) + 1);
        }

        else if (SubFunctionId == 3)
        {
            //
            // Sub-leaf 3 (Memory Bandwidth Allocation)
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  MaxMbaThrottlingValue (minus-one) = %u (actual = %u)\n\n",
                         CPUID_EAX_MAX_MBA_THROTTLING_VALUE(CpuidRequest->EAX),
                         CPUID_EAX_MAX_MBA_THROTTLING_VALUE(CpuidRequest->EAX) + 1);

            ShowMessages("-- EBX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EBX_RESERVED(CpuidRequest->EBX));

            ShowMessages("-- ECX --\n\n");
            ShowMessages("  ResponseOfDelayIsLinear = %s%s\n",
                         CPUID_ECX_RESPONSE_OF_DELAY_IS_LINEAR(CpuidRequest->ECX) ? "TRUE" : "FALSE",
                         CPUID_ECX_RESPONSE_OF_DELAY_IS_LINEAR(CpuidRequest->ECX) ? " (linear response)" : " (non-linear)\n\n");

            ShowMessages("-- EDX --\n\n");
            ShowMessages("  HighestCosNumberSupported = %u (actual = %u COS)\n\n",
                         CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED(CpuidRequest->EDX),
                         CPUID_EDX_HIGHEST_COS_NUMBER_SUPPORTED(CpuidRequest->EDX) + 1);
        }

        else
        {
            ShowMessages("  Sub-leaf %u is NOT supported on this CPU\n", SubFunctionId);
            ShowMessages("  raw bytes: EAX = %u\n  EBX = %u\n  ECX = %u\n  EDX = %u\n\n",
                         CpuidRequest->EAX,
                         CpuidRequest->EBX,
                         CpuidRequest->ECX,
                         CpuidRequest->EDX);
        }

        break;

    case 0x11:
        ShowMessages("==== CPUID.(EAX=11H) ====\n\n");
        ShowMessages("  This leaf is reserved (not implemented).\n\n");
        ShowMessages("  Raw data:\n");
        ShowMessages("  EAX: 0x%08X\n", CpuidRequest->EAX);
        ShowMessages("  EBX: 0x%08X\n", CpuidRequest->EBX);
        ShowMessages("  ECX: 0x%08X\n", CpuidRequest->ECX);
        ShowMessages("  EDX: 0x%08X\n\n", CpuidRequest->EDX);
        break;

    case 0x12:
    {
        //
        // SGX, not DAT. 0x12 = 18 (decimal)
        //
        ShowMessages("==== CPUID.(EAX=12H) Intel SGX Information ====\n\n");

        if (!CpuidRequest->Leaf12Supported)
        {
            ShowMessages("  SGX is not supported on this CPU (CPUID.7H:EBX[2] = 0).\n\n");
            break;
        }

        ShowMessages("  *******************************************************\n"
                     "  *             Max NumberOfSubLeaves = %u               *\n"
                     "  *******************************************************\n\n",
                     CpuidRequest->Leaf12MaxSubLeaf);

        if (SubFunctionId == 0)
        {
            //
            // SGX capabilities
            //
            ShowMessages("-- EAX (SGX Capabilities) --\n\n");
            ShowMessages("  SGX1 Support (bit 0)           = %s\n",
                         CPUID_EAX_SGX1(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  SGX2 Support (bit 1)           = %s\n",
                         CPUID_EAX_SGX2(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  ENCLV Advanced (bit 5)         = %s\n",
                         CPUID_EAX_SGX_ENCLV_ADVANCED(CpuidRequest->EAX) ? "TRUE" : "FALSE");
            ShowMessages("  ENCLS Advanced (bit 6)         = %s\n\n",
                         CPUID_EAX_SGX_ENCLS_ADVANCED(CpuidRequest->EAX) ? "TRUE" : "FALSE");

            ShowMessages("-- EBX (MISCSELECT - Extended SGX Features) --\n\n");
            ShowMessages("  Miscselect = 0x%08X\n\n",
                         CPUID_EBX_MISCSELECT(CpuidRequest->EBX));

            ShowMessages("-- ECX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n",
                         CPUID_ECX_RESERVED(CpuidRequest->ECX));

            ShowMessages("-- EDX (Maximum Enclave Sizes) --\n\n");
            ShowMessages("  MaxEnclaveSizeNot64 = %u (2^%u = %llu bytes)\n",
                         CPUID_EDX_MAX_ENCLAVE_SIZE_NOT64(CpuidRequest->EDX),
                         CPUID_EDX_MAX_ENCLAVE_SIZE_NOT64(CpuidRequest->EDX),
                         (ULONG64)(1ULL << CPUID_EDX_MAX_ENCLAVE_SIZE_NOT64(CpuidRequest->EDX)));
            ShowMessages("  MaxEnclaveSize64     = %u (2^%u = %llu bytes)\n\n",
                         CPUID_EDX_MAX_ENCLAVE_SIZE_64(CpuidRequest->EDX),
                         CPUID_EDX_MAX_ENCLAVE_SIZE_64(CpuidRequest->EDX),
                         (ULONG64)(1ULL << CPUID_EDX_MAX_ENCLAVE_SIZE_64(CpuidRequest->EDX)));
        }

        else if (SubFunctionId == 1)
        {
            //
            // SGX attributes
            //
            ShowMessages("-- EAX (SECS.ATTRIBUTES[31:0]) --\n\n");
            ShowMessages("  ValidSecsAttributes0 = 0x%08X\n\n",
                         CPUID_EAX_VALID_SECS_ATTRIBUTES_0(CpuidRequest->EAX));

            ShowMessages("-- EBX (SECS.ATTRIBUTES[63:32]) --\n\n");
            ShowMessages("  ValidSecsAttributes1 = 0x%08X\n\n",
                         CPUID_EBX_VALID_SECS_ATTRIBUTES_1(CpuidRequest->EBX));

            ShowMessages("-- ECX (SECS.ATTRIBUTES[95:64]) --\n\n");
            ShowMessages("  ValidSecsAttributes2 = 0x%08X\n\n",
                         CPUID_ECX_VALID_SECS_ATTRIBUTES_2(CpuidRequest->ECX));

            ShowMessages("-- EDX (SECS.ATTRIBUTES[127:96]) --\n\n");
            ShowMessages("  ValidSecsAttributes3 = 0x%08X\n\n",
                         CPUID_EDX_VALID_SECS_ATTRIBUTES_3(CpuidRequest->EDX));
        }

        else
        {
            //
            // Subleaf 2+ (EPC sections)
            //
            if (CPUID_EAX_SUB_LEAF_TYPE(CpuidRequest->EAX) == 0)
            {
                //
                // Type 0: invalid subleaf
                //
                ShowMessages("-- EAX --\n\n");
                ShowMessages("  SubLeafType = %u (Invalid - no EPC section)\n",
                             CPUID_EAX_SUB_LEAF_TYPE(CpuidRequest->EAX));

                ShowMessages("\n-- EBX --\n");
                ShowMessages("  Zero = 0x%08X\n", CPUID_EBX_ZERO(CpuidRequest->EBX));

                ShowMessages("\n-- ECX --\n");
                ShowMessages("  Zero = 0x%08X\n", CPUID_ECX_ZERO(CpuidRequest->ECX));

                ShowMessages("\n-- EDX --\n");
                ShowMessages("  Zero = 0x%08X\n", CPUID_EDX_ZERO(CpuidRequest->EDX));
            }

            else if (CPUID_EAX_SUB_LEAF_TYPE(CpuidRequest->EAX) == 1)
            {
                //
                // Reconstruct base address from EAX and EBX
                //
                UINT64 BaseLow     = (UINT64)CPUID_EAX_EPC_BASE_PHYSICAL_ADDRESS_1(CpuidRequest->EAX);
                UINT64 BaseHigh    = (UINT64)CPUID_EBX_EPC_BASE_PHYSICAL_ADDRESS_2(CpuidRequest->EBX);
                UINT64 BaseAddress = (BaseHigh << 32) | (BaseLow << 12);

                //
                // Reconstruct size from ECX and EDX
                //
                UINT64 SizeLow   = (UINT64)CPUID_ECX_EPC_SIZE_1(CpuidRequest->ECX);
                UINT64 SizeHigh  = (UINT64)CPUID_EDX_EPC_SIZE_2(CpuidRequest->EDX);
                UINT64 SizeBytes = (SizeHigh << 32) | (SizeLow << 12);

                ShowMessages("-- EAX --\n\n");
                ShowMessages("  SubLeafType = %u (EPC Section)\n",
                             CPUID_EAX_SUB_LEAF_TYPE(CpuidRequest->EAX));
                ShowMessages("  EpcBasePhysicalAddress1 (bits 31:12) = 0x%05X\n\n",
                             CPUID_EAX_EPC_BASE_PHYSICAL_ADDRESS_1(CpuidRequest->EAX));

                ShowMessages("-- EBX --\n\n");
                ShowMessages("  EpcBasePhysicalAddress2 (bits 51:32) = 0x%05X\n\n",
                             CPUID_EBX_EPC_BASE_PHYSICAL_ADDRESS_2(CpuidRequest->EBX));

                ShowMessages("-- ECX --\n\n");
                ShowMessages("  EpcSectionProperty = %u",
                             CPUID_ECX_EPC_SECTION_PROPERTY(CpuidRequest->ECX));
                switch (CPUID_ECX_EPC_SECTION_PROPERTY(CpuidRequest->ECX))
                {
                case 0:
                    ShowMessages(" (No confidentiality/integrity)\n\n");
                    break;
                case 1:
                    ShowMessages(" (Confidentiality and integrity protection)\n\n");
                    break;
                default:
                    ShowMessages(" (Reserved)\n\n");
                    break;
                }
                ShowMessages("  EpcSize1 (bits 31:12) = 0x%05X\n\n",
                             CPUID_ECX_EPC_SIZE_1(CpuidRequest->ECX));

                ShowMessages("-- EDX --\n\n");
                ShowMessages("  EpcSize2 (bits 51:32) = 0x%05X\n\n",
                             CPUID_EDX_EPC_SIZE_2(CpuidRequest->EDX));

                ShowMessages("-- EPC Section Information --\n\n");
                ShowMessages("  Base Address = 0x%016llX\n", (ULONG64)BaseAddress);
                ShowMessages("  Size         = 0x%016llX bytes (%llu MB)\n\n",
                             (ULONG64)SizeBytes,
                             (ULONG64)(SizeBytes / (1024 * 1024)));
            }

            else
            {
                //
                // Unknown subleaf type (reserved)
                //
                ShowMessages("-- Raw Data for Sub-leaf %u (Type %u) --\n\n",
                             SubFunctionId,
                             CPUID_EAX_SUB_LEAF_TYPE(CpuidRequest->EAX));

                ShowMessages("  EAX = 0x%08X\n", CpuidRequest->EAX);
                ShowMessages("  EBX = 0x%08X\n", CpuidRequest->EBX);
                ShowMessages("  ECX = 0x%08X\n", CpuidRequest->ECX);
                ShowMessages("  EDX = 0x%08X\n\n", CpuidRequest->EDX);
                ShowMessages("  Note: Reserved sub-leaf type. Please refer to the latest Intel SDM.\n\n");
            }
        }

        break;
    }

    case 0x13:
        ShowMessages("==== CPUID.(EAX=13H) ====\n\n");
        ShowMessages("  This leaf is reserved (not implemented).\n\n");
        ShowMessages("  Raw data:\n");
        ShowMessages("  EAX: 0x%08X\n", CpuidRequest->EAX);
        ShowMessages("  EBX: 0x%08X\n", CpuidRequest->EBX);
        ShowMessages("  ECX: 0x%08X\n", CpuidRequest->ECX);
        ShowMessages("  EDX: 0x%08X\n\n", CpuidRequest->EDX);
        break;

    case 0x14:
        ShowMessages("==== CPUID.(EAX=14H) Intel Processor Trace Information ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *             Max NumberOfSubLeaves = %u               *\n"
                     "  *******************************************************\n\n",
                     CpuidRequest->LeafEaxMaxSubleaf);

        ShowMessages("==== CPUID.(EAX=14H, ECX=%u) Intel Processor Trace Information ====\n\n", SubFunctionId);

        if (SubFunctionId == 0)
        {
            //
            // main leaf
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  MaxSubLeaf = %u\n\n",
                         CPUID_EAX_MAX_SUB_LEAF(CpuidRequest->EAX));

            ShowMessages("-- EBX (Supported Features) --\n\n");
            ShowMessages("  CR3Filter support (bit 0)                    = %s\n",
                         CPUID_EBX_FLAG0(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  Configurable PSB & Cycle-Accurate (bit 1)    = %s\n",
                         CPUID_EBX_FLAG1(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  IP Filtering & TraceStop (bit 2)             = %s\n",
                         CPUID_EBX_FLAG2(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  MTC & COFI suppression (bit 3)               = %s\n",
                         CPUID_EBX_FLAG3(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  PTWRITE support (bit 4)                      = %s\n",
                         CPUID_EBX_FLAG4(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  Power Event Trace (bit 5)                    = %s\n",
                         CPUID_EBX_FLAG5(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  PSB/PMI preservation (bit 6)                 = %s\n",
                         CPUID_EBX_FLAG6(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  Event Trace (bit 7)                          = %s\n",
                         CPUID_EBX_FLAG7(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  Disable TNT (bit 8)                          = %s\n\n",
                         CPUID_EBX_FLAG8(CpuidRequest->EBX) ? "TRUE" : "FALSE");

            ShowMessages("-- ECX (Output Schemes) --\n\n");
            ShowMessages("  ToPA output scheme (bit 0)                   = %s\n",
                         CPUID_ECX_FLAG0(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  ToPA tables with any entries (bit 1)         = %s\n",
                         CPUID_ECX_FLAG1(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  Single-Range Output scheme (bit 2)           = %s\n",
                         CPUID_ECX_FLAG2(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  Trace Transport subsystem (bit 3)            = %s\n",
                         CPUID_ECX_FLAG3(CpuidRequest->ECX) ? "TRUE" : "FALSE");
            ShowMessages("  LIP values include CS base (bit 31)          = %s\n\n",
                         CPUID_ECX_FLAG31(CpuidRequest->ECX) ? "TRUE" : "FALSE");

            ShowMessages("-- EDX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n",
                         CPUID_EDX_RESERVED(CpuidRequest->EDX));
        }

        else if (SubFunctionId == 1)
        {
            //
            // Packet generation
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  NumberOfConfigurableAddressRangesForFiltering = %u\n",
                         CPUID_EAX_NUMBER_OF_CONFIGURABLE_ADDRESS_RANGES_FOR_FILTERING(CpuidRequest->EAX));
            ShowMessages("  BitmapOfSupportedMtcPeriodEncodings          = 0x%04X\n\n",
                         CPUID_EAX_BITMAP_OF_SUPPORTED_MTC_PERIOD_ENCODINGS(CpuidRequest->EAX));

            ShowMessages("-- EBX --\n\n");
            ShowMessages("  BitmapOfSupportedCycleThresholdValueEncodings = 0x%04X\n",
                         CPUID_EBX_BITMAP_OF_SUPPORTED_CYCLE_THRESHOLD_VALUE_ENCODINGS(CpuidRequest->EBX));
            ShowMessages("  BitmapOfSupportedConfigurablePsbFrequencyEncodings = 0x%04X\n\n",
                         CPUID_EBX_BITMAP_OF_SUPPORTED_CONFIGURABLE_PSB_FREQUENCY_ENCODINGS(CpuidRequest->EBX));

            ShowMessages("-- ECX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n",
                         CPUID_ECX_RESERVED(CpuidRequest->ECX));

            ShowMessages("-- EDX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n",
                         CPUID_EDX_RESERVED(CpuidRequest->EDX));
        }

        else
        {
            //
            // Subleaves > 1 (if they exist)
            //
            ShowMessages("-- Raw Data for Sub-leaf %u --\n\n", SubFunctionId);
            ShowMessages("  EAX = 0x%08X\n", CpuidRequest->EAX);
            ShowMessages("  EBX = 0x%08X\n", CpuidRequest->EBX);
            ShowMessages("  ECX = 0x%08X\n", CpuidRequest->ECX);
            ShowMessages("  EDX = 0x%08X\n\n", CpuidRequest->EDX);
            ShowMessages("  Note: This sub-leaf may be for future Intel PT features.\n");
            ShowMessages("  Please refer to the latest Intel SDM for interpretation.\n\n");
        }

        break;

    case 0x15:
    {
        ShowMessages("==== CPUID.(EAX=15H) TSC and Core Crystal Clock Information ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *               LEAF 15h HAS NO SUBLEAVES             *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");
        //
        // EAX: Denominator
        //
        UINT32 Denominator = CPUID_EAX_DENOMINATOR(CpuidRequest->EAX);
        ShowMessages("-- EAX --\n\n");
        ShowMessages("  Denominator = %u\n\n", Denominator);

        //
        // EBX: Numerator
        //
        UINT32 Numerator = CPUID_EBX_NUMERATOR(CpuidRequest->EBX);
        ShowMessages("-- EBX --\n\n");
        ShowMessages("  Numerator = %u\n\n", Numerator);

        //
        // ECX: Nominal frequency (if available)
        //
        UINT32 NominalFreq = CPUID_ECX_NOMINAL_FREQUENCY(CpuidRequest->ECX);
        ShowMessages("-- ECX --\n\n");
        ShowMessages("  NominalFrequency = %u Hz", NominalFreq);
        if (NominalFreq > 0)
        {
            ShowMessages(" (%u MHz, %u GHz)\n",
                         NominalFreq / 1000000,
                         NominalFreq / 1000000000);
        }
        else
        {
            ShowMessages("  (not enumerated)\n\n");
        }

        //
        // EDX: reserved
        //
        ShowMessages("\n-- EDX --\n");
        ShowMessages("  Reserved = 0x%08X\n",
                     CPUID_EDX_RESERVED(CpuidRequest->EDX));

        //
        // Calculate and display TSC frequency if possible
        //
        ShowMessages("-- TSC Frequency Information --\n\n");

        if (Denominator == 0 || Numerator == 0)
        {
            ShowMessages("  TSC ratio not enumerated (denominator or numerator is 0)\n\n");
        }

        else
        {
            ShowMessages("  TSC / Core Crystal Clock ratio = %u / %u\n",
                         Numerator,
                         Denominator);
            ShowMessages("  TSC frequency = Core Crystal Clock * (%u/%u)\n",
                         Numerator,
                         Denominator);
            if (NominalFreq > 0)
            {
                UINT64 TSCFreqHz = (UINT64)NominalFreq * Numerator / Denominator;
                ShowMessages("  TSC frequency = %llu Hz (%llu MHz, %llu GHz)\n\n",
                             (ULONG64)TSCFreqHz,
                             (ULONG64)(TSCFreqHz / 1000000),
                             (ULONG64)(TSCFreqHz / 1000000000));
            }
            else
            {
                ShowMessages("  TSC frequency cannot be calculated (nominal frequency not enumerated)\n\n");
            }
        }

        break;
    }

    case 0x16:
    {
        ShowMessages("==== CPUID.(EAX=16H) Processor Frequency Information ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *               LEAF 16h HAS NO SUBLEAVES             *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        //
        // EAX: Processor base frequency
        //
        UINT32 BaseFreq = CPUID_EAX_PROCESOR_BASE_FREQUENCY_MHZ(CpuidRequest->EAX);
        ShowMessages("-- EAX --\n\n");
        if (BaseFreq == 0)
        {
            ShowMessages("  ProcessorBaseFrequencyMhz = 0 (not supported)\n\n");
        }
        else
        {
            ShowMessages("  ProcessorBaseFrequencyMhz = %u MHz\n", BaseFreq);
        }

        //
        // EBX: Maximum frequency
        //
        UINT32 MaxFreq = CPUID_EBX_PROCESSOR_MAXIMUM_FREQUENCY_MHZ(CpuidRequest->EBX);
        ShowMessages("-- EBX --\n\n");
        if (MaxFreq == 0)
        {
            ShowMessages("  ProcessorMaximumFrequencyMhz = 0 (not supported)\n\n");
        }
        else
        {
            ShowMessages("  ProcessorMaximumFrequencyMhz = %u MHz\n", MaxFreq);
        }

        //
        // ECX: Bus (reference) frequency
        //
        UINT32 BusFreq = CPUID_ECX_BUS_FREQUENCY_MHZ(CpuidRequest->ECX);
        ShowMessages("-- ECX --\n\n");
        if (BusFreq == 0)
        {
            ShowMessages("  BusFrequencyMhz = 0 (not supported)\n\n");
        }
        else
        {
            ShowMessages("  BusFrequencyMhz = %u MHz\n\n", BusFreq);
        }

        //
        // EDX: reserved
        //
        ShowMessages("-- EDX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n",
                     CPUID_EDX_RESERVED(CpuidRequest->EDX));

        //
        // Display summary
        //
        ShowMessages("-- Frequency Summary --\n\n");
        ShowMessages("  Base Frequency:     ");
        if (BaseFreq == 0)
        {
            ShowMessages("Not Supported\n");
        }
        else
        {
            ShowMessages("%u MHz\n", BaseFreq);
        }

        //
        // Maximum frequency
        //
        ShowMessages("  Maximum Frequency:  ");
        if (MaxFreq == 0)
        {
            ShowMessages("Not Supported\n");
        }
        else
        {
            ShowMessages("%u MHz\n", MaxFreq);
        }

        //
        // Bus frequency
        //
        ShowMessages("  Bus Frequency:      ");
        if (BusFreq == 0)
        {
            ShowMessages("Not Supported\n");
        }
        else
        {
            ShowMessages("%u MHz\n\n", BusFreq);
        }

        //
        // Show turbo boost if both base and max are supported and max > base
        //
        if (BaseFreq > 0 && MaxFreq > 0 && MaxFreq > BaseFreq)
        {
            UINT32 TurboBoost   = MaxFreq - BaseFreq;
            FLOAT  TurboPercent = ((FLOAT)TurboBoost / BaseFreq) * 100.0f;
            ShowMessages("  Turbo Boost:        %u MHz above base (%.1f%% increase)\n\n",
                         TurboBoost,
                         TurboPercent);
        }

        ShowMessages("  *******************************************************\n");
        ShowMessages("  *  NOTE: These values are for display purposes only   *\n");
        ShowMessages("  *  They do not reflect actual running frequencies     *\n");
        ShowMessages("  *  Actual frequencies depend on workload, power, etc. *\n");
        ShowMessages("  *******************************************************\n\n");

        break;
    }

    case 0x17:
    {
        ShowMessages("==== CPUID.(EAX=17H) SoC Vendor Information ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *             Max NumberOfSubLeaves = %u               *\n"
                     "  *******************************************************\n\n",
                     CPUID_EAX_MAX_SOC_ID_INDEX(CpuidRequest->EAX));

        ShowMessages("==== CPUID.(EAX=17H, ECX=%u) SoC Vendor Information ====\n\n", SubFunctionId);

        if (SubFunctionId == 0)
        {
            //
            // Main
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  MaxSocIdIndex = %u\n\n", CPUID_EAX_MAX_SOC_ID_INDEX(CpuidRequest->EAX));

            ShowMessages("-- EBX --\n\n");
            ShowMessages("  SocVendorId = 0x%04X\n",
                         CPUID_EBX_SOC_VENDOR_ID(CpuidRequest->EBX));
            ShowMessages("  IsVendorScheme = %s\n\n",
                         CPUID_EBX_IS_VENDOR_SCHEME(CpuidRequest->EBX) ? "TRUE (Industry Standard)" : "FALSE (Intel Assigned)");

            ShowMessages("-- ECX --\n\n");
            ShowMessages("  ProjectId = 0x%08X\n\n",
                         CPUID_ECX_PROJECT_ID(CpuidRequest->ECX));

            ShowMessages("-- EDX --\n\n");
            ShowMessages("  SteppingId = 0x%08X\n\n",
                         CPUID_EDX_STEPPING_ID(CpuidRequest->EDX));
        }

        else if (SubFunctionId >= 1 && SubFunctionId <= 3)
        {
            //
            // subleaves 1-3 (brand string)
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  SocVendorBrandString[0..3] = 0x%08X (%c%c%c%c)\n\n",
                         CPUID_EAX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EAX),
                         (CPUID_EAX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EAX) >> 0) & 0xFF,
                         (CPUID_EAX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EAX) >> 8) & 0xFF,
                         (CPUID_EAX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EAX) >> 16) & 0xFF,
                         (CPUID_EAX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EAX) >> 24) & 0xFF);

            ShowMessages("-- EBX --\n\n");
            ShowMessages("  SocVendorBrandString[4..7] = 0x%08X (%c%c%c%c)\n\n",
                         CPUID_EBX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EBX),
                         (CPUID_EBX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EBX) >> 0) & 0xFF,
                         (CPUID_EBX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EBX) >> 8) & 0xFF,
                         (CPUID_EBX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EBX) >> 16) & 0xFF,
                         (CPUID_EBX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EBX) >> 24) & 0xFF);

            ShowMessages("-- ECX --\n\n");
            ShowMessages("  SocVendorBrandString[8..11] = 0x%08X (%c%c%c%c)\n\n",
                         CPUID_ECX_SOC_VENDOR_BRAND_STRING(CpuidRequest->ECX),
                         (CPUID_ECX_SOC_VENDOR_BRAND_STRING(CpuidRequest->ECX) >> 0) & 0xFF,
                         (CPUID_ECX_SOC_VENDOR_BRAND_STRING(CpuidRequest->ECX) >> 8) & 0xFF,
                         (CPUID_ECX_SOC_VENDOR_BRAND_STRING(CpuidRequest->ECX) >> 16) & 0xFF,
                         (CPUID_ECX_SOC_VENDOR_BRAND_STRING(CpuidRequest->ECX) >> 24) & 0xFF);

            ShowMessages("-- EDX --\n\n");
            ShowMessages("  SocVendorBrandString[12..15] = 0x%08X (%c%c%c%c)\n\n",
                         CPUID_EDX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EDX),
                         (CPUID_EDX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EDX) >> 0) & 0xFF,
                         (CPUID_EDX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EDX) >> 8) & 0xFF,
                         (CPUID_EDX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EDX) >> 16) & 0xFF,
                         (CPUID_EDX_SOC_VENDOR_BRAND_STRING(CpuidRequest->EDX) >> 24) & 0xFF);
        }

        else
        {
            //
            // Sub-leaves > MaxSOCID_Index (Reserved, should return all zeros)
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EAX_RESERVED(CpuidRequest->EAX));

            ShowMessages("-- EBX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EBX_RESERVED(CpuidRequest->EBX));

            ShowMessages("-- ECX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n", CPUID_ECX_RESERVED(CpuidRequest->ECX));

            ShowMessages("-- EDX --\n\n");
            ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EDX_RESERVED(CpuidRequest->EDX));
        }
        break;
    }

    case 0x18:
        //
        // DAT, 0x18 = 24 (decimal)
        //
        ShowMessages("==== CPUID.(EAX=18H) Deterministic Address Translation Parameters ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *             Max NumberOfSubLeaves = %u               *\n"
                     "  *******************************************************\n\n",
                     CpuidRequest->LeafEaxMaxSubleaf);

        ShowMessages("==== CPUID.(EAX=18H, ECX=%u) Deterministic Address Translation Parameters ====\n\n", SubFunctionId);

        if (SubFunctionId == 0)
        {
            //
            // main
            //
            ShowMessages("-- EAX --\n\n");
            ShowMessages("  MaxSubLeaf = %u\n\n", CPUID_EAX_MAX_SUB_LEAF(CpuidRequest->EAX));

            //
            // EBX: Page size support and associativity
            //
            ShowMessages("-- EBX --\n\n");
            ShowMessages("  PageEntries4KbSupported    = %s\n",
                         CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  PageEntries2MbSupported    = %s\n",
                         CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  PageEntries4MbSupported    = %s\n",
                         CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  PageEntries1GbSupported    = %s\n",
                         CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED(CpuidRequest->EBX) ? "TRUE" : "FALSE");
            ShowMessages("  Partitioning               = %u%s\n",
                         CPUID_EBX_PARTITIONING(CpuidRequest->EBX),
                         CPUID_EBX_PARTITIONING(CpuidRequest->EBX) == 0 ? " (soft partitioning)" : "");
            ShowMessages("  WaysOfAssociativity (W)    = %u\n\n",
                         CPUID_EBX_WAYS_OF_ASSOCIATIVITY_00(CpuidRequest->EBX));

            //
            // ECX: Number of Sets
            //
            ShowMessages("-- ECX --\n\n");
            ShowMessages("  NumberOfSets               = %u\n\n",
                         CPUID_ECX_NUMBER_OF_SETS(CpuidRequest->ECX));

            //
            // EDX: Translation cache type and properties
            //
            ShowMessages("-- EDX --\n\n");

            switch (CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD(CpuidRequest->EDX))
            {
            case 0:
                TypeName = "Null (sub-leaf not valid)";
                break;
            case 1:
                TypeName = "Data TLB";
                break;
            case 2:
                TypeName = "Instruction TLB";
                break;
            case 3:
                TypeName = "Unified TLB";
                break;
            default:
                TypeName = "Reserved";
                break;
            }
            ShowMessages("  TranslationCacheTypeField  = %u (%s)\n",
                         CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD(CpuidRequest->EDX),
                         TypeName);
            ShowMessages("  TranslationCacheLevel      = %u\n",
                         CPUID_EDX_TRANSLATION_CACHE_LEVEL(CpuidRequest->EDX));
            ShowMessages("  FullyAssociativeStructure  = %s%s\n",
                         CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE(CpuidRequest->EDX) ? "TRUE" : "FALSE",
                         CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE(CpuidRequest->EDX) ? " (fully associative)" : "");

            ShowMessages("  MaxAddressableIdsForLogicalProcessors (raw) = %u -> actual = %u (raw + 1)\n",
                         CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS(CpuidRequest->EDX),
                         CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS(CpuidRequest->EDX) + 1);
        }

        else
        {
            //
            // subleaves >= 1 (Translation Structure Information)
            //

            //
            // Check if this sub-leaf is valid (EDX[4:0] != 0)
            //
            if (CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD(CpuidRequest->EDX) == 0)
            {
                ShowMessages("  Sub-leaf %u is invalid (TranslationCacheTypeField = 0)\n", SubFunctionId);
                ShowMessages("  All registers should be zero according to spec:\n");
                ShowMessages("  EAX = 0x%08X\n", CPUID_EAX_RESERVED(CpuidRequest->EAX));
                ShowMessages("  EBX = 0x%08X\n", CpuidRequest->EBX);
                ShowMessages("  ECX = 0x%08X\n", CpuidRequest->ECX);
                ShowMessages("  EDX = 0x%08X\n", CpuidRequest->EDX);
            }

            else
            {
                //
                // EAX: Reserved for sub-leaves >= 1
                //
                ShowMessages("-- EAX --\n\n");
                ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EAX_RESERVED(CpuidRequest->EAX));

                //
                // EBX: Page size support and associativity
                //
                ShowMessages("-- EBX --\n\n");
                ShowMessages("  PageEntries4KbSupported    = %s\n",
                             CPUID_EBX_PAGE_ENTRIES_4KB_SUPPORTED(CpuidRequest->EBX) ? "TRUE" : "FALSE");
                ShowMessages("  PageEntries2MbSupported    = %s\n",
                             CPUID_EBX_PAGE_ENTRIES_2MB_SUPPORTED(CpuidRequest->EBX) ? "TRUE" : "FALSE");
                ShowMessages("  PageEntries4MbSupported    = %s\n",
                             CPUID_EBX_PAGE_ENTRIES_4MB_SUPPORTED(CpuidRequest->EBX) ? "TRUE" : "FALSE");
                ShowMessages("  PageEntries1GbSupported    = %s\n",
                             CPUID_EBX_PAGE_ENTRIES_1GB_SUPPORTED(CpuidRequest->EBX) ? "TRUE" : "FALSE");
                ShowMessages("  Partitioning               = %u%s\n",
                             CPUID_EBX_PARTITIONING(CpuidRequest->EBX),
                             CPUID_EBX_PARTITIONING(CpuidRequest->EBX) == 0 ? " (soft partitioning)" : "");
                ShowMessages("  WaysOfAssociativity (W)    = %u\n\n",
                             CPUID_EBX_WAYS_OF_ASSOCIATIVITY_01(CpuidRequest->EBX));

                // ECX: Number of Sets
                ShowMessages("-- ECX --\n\n");
                ShowMessages("  NumberOfSets               = %u\n\n",
                             CPUID_ECX_NUMBER_OF_SETS(CpuidRequest->ECX));

                // EDX: Translation cache type and properties
                ShowMessages("-- EDX --\n\n");
                switch (CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD(CpuidRequest->EDX))
                {
                case 0:
                    TypeName = "Null (sub-leaf not valid)";
                    break;
                case 1:
                    TypeName = "Data TLB";
                    break;
                case 2:
                    TypeName = "Instruction TLB";
                    break;
                case 3:
                    TypeName = "Unified TLB";
                    break;
                default:
                    TypeName = "Reserved";
                    break;
                }
                ShowMessages("  TranslationCacheTypeField  = %u (%s)\n",
                             CPUID_EDX_TRANSLATION_CACHE_TYPE_FIELD(CpuidRequest->EDX),
                             TypeName);
                ShowMessages("  TranslationCacheLevel      = %u\n",
                             CPUID_EDX_TRANSLATION_CACHE_LEVEL(CpuidRequest->EDX));
                ShowMessages("  FullyAssociativeStructure  = %s%s\n",
                             CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE(CpuidRequest->EDX) ? "TRUE" : "FALSE",
                             CPUID_EDX_FULLY_ASSOCIATIVE_STRUCTURE(CpuidRequest->EDX) ? " (fully associative)" : "");

                ShowMessages("  MaxAddressableIdsForLogicalProcessors (raw) = %u -> actual = %u (raw + 1)\n",
                             CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS(CpuidRequest->EDX),
                             CPUID_EDX_MAX_ADDRESSABLE_IDS_FOR_LOGICAL_PROCESSORS(CpuidRequest->EDX) + 1);
            }
        }

        break;

    case 0x80000000:
        ShowMessages("==== CPUID.(EAX=80000000H) Extended Function Information ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *           LEAF 0x80000000 HAS NO SUBLEAVES          *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        ShowMessages("-- EAX --\n\n");
        ShowMessages("  MaxExtendedFunctions = 0x%08X (%u)\n\n",
                     CPUID_EAX_MAX_EXTENDED_FUNCTIONS(CpuidRequest->EAX),
                     CPUID_EAX_MAX_EXTENDED_FUNCTIONS(CpuidRequest->EAX));

        ShowMessages("-- EBX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EBX_RESERVED(CpuidRequest->EBX));

        ShowMessages("-- ECX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_ECX_RESERVED(CpuidRequest->ECX));

        ShowMessages("-- EDX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EDX_RESERVED(CpuidRequest->EDX));

        break;

    case 0x80000001:
        ShowMessages("==== CPUID.(EAX=80000001H) Extended CPU Signature ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *           LEAF 0x80000001 HAS NO SUBLEAVES          *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        ShowMessages("-- EAX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EAX_RESERVED(CpuidRequest->EAX));

        ShowMessages("-- EBX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EBX_RESERVED(CpuidRequest->EBX));

        ShowMessages("-- ECX --\n\n");
        ShowMessages("  LAHF/SAHF Available in 64-bit Mode = %s\n",
                     CPUID_ECX_LAHF_SAHF_AVAILABLE_IN_64_BIT_MODE(CpuidRequest->ECX) ? "True" : "False");
        ShowMessages("  LZCNT                              = %s\n",
                     CPUID_ECX_LZCNT(CpuidRequest->ECX) ? "True" : "False");
        ShowMessages("  PREFETCHW                          = %s\n\n",
                     CPUID_ECX_PREFETCHW(CpuidRequest->ECX) ? "True" : "False");

        ShowMessages("-- EDX --\n\n");
        ShowMessages("  SYSCALL/SYSRET Available in 64-bit Mode = %s\n",
                     CPUID_EDX_SYSCALL_SYSRET_AVAILABLE_IN_64_BIT_MODE(CpuidRequest->EDX) ? "True" : "False");
        ShowMessages("  Execute Disable Bit Available            = %s\n",
                     CPUID_EDX_EXECUTE_DISABLE_BIT_AVAILABLE(CpuidRequest->EDX) ? "True" : "False");
        ShowMessages("  1-GByte Pages Available                 = %s\n",
                     CPUID_EDX_PAGES_1GB_AVAILABLE(CpuidRequest->EDX) ? "True" : "False");
        ShowMessages("  RDTSCP Available                        = %s\n",
                     CPUID_EDX_RDTSCP_AVAILABLE(CpuidRequest->EDX) ? "True" : "False");
        ShowMessages("  Intel 64 Architecture Available         = %s\n\n",
                     CPUID_EDX_IA64_AVAILABLE(CpuidRequest->EDX) ? "True" : "False");

        break;
    //
    // 0x80000002-0x80000004 - Processor brand string
    //
    case 0x80000002:
    case 0x80000003:
    case 0x80000004:
    {
        ShowMessages("==== CPUID.(EAX=80000002H-80000004H) Processor Information ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *         LEAF 0x80000002-4 HAS NO SUBLEAVES          *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");
        ShowMessages("  Brand String = \"%s\"\n\n", CpuidRequest->BrandString);

        break;
    }
    case 0x80000005:
        ShowMessages("EAX = 0x80000005: not implemented.\n");
        break;

    case 0x80000006:
    {
        ShowMessages("==== CPUID.(EAX=80000006H) Extended Cache Information ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *          LEAF 0x80000006 HAS NO SUBLEAVES           *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        ShowMessages("-- EAX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EAX_RESERVED(CpuidRequest->EAX));

        ShowMessages("-- EBX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EBX_RESERVED(CpuidRequest->EBX));

        ShowMessages("-- ECX (L2 Cache Information) --\n");

        UINT32 LineSize  = CPUID_ECX_CACHE_LINE_SIZE_IN_BYTES(CpuidRequest->ECX);
        UINT32 Assoc     = CPUID_ECX_L2_ASSOCIATIVITY_FIELD(CpuidRequest->ECX);
        UINT32 CacheSize = CPUID_ECX_CACHE_SIZE_IN_1K_UNITS(CpuidRequest->ECX);

        //
        // decode associativity
        //
        switch (Assoc)
        {
        case 0x00:
            AssocName = "Disabled";
            break;
        case 0x01:
            AssocName = "Direct mapped";
            break;
        case 0x02:
            AssocName = "2-way";
            break;
        case 0x04:
            AssocName = "4-way";
            break;
        case 0x06:
            AssocName = "8-way";
            break;
        case 0x08:
            AssocName = "16-way";
            break;
        case 0x0F:
            AssocName = "Fully associative";
            break;
        default:
            AssocName = "Reserved";
            break;
        }
        ShowMessages("  CacheLineSizeInBytes       = %u bytes\n", LineSize);
        ShowMessages("  L2AssociativityField       = 0x%02X (%s)\n", Assoc, AssocName);
        ShowMessages("  CacheSizeIn1KUnits         = %u KB (%u MB)\n",
                     CacheSize,
                     CacheSize / 1024);

        ShowMessages("-- EDX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EDX_RESERVED(CpuidRequest->EDX));

        break;
    }

    case 0x80000007:
        ShowMessages("==== CPUID.(EAX=80000007H) Extended Time Stamp Counter ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *          LEAF 0x80000007 HAS NO SUBLEAVES           *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        ShowMessages("-- EAX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EAX_RESERVED(CpuidRequest->EAX));

        ShowMessages("-- EBX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EBX_RESERVED(CpuidRequest->EBX));

        ShowMessages("-- ECX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_ECX_RESERVED(CpuidRequest->ECX));

        ShowMessages("-- EDX --\n\n");
        ShowMessages("  InvariantTscAvailable = %u%s\n\n",
                     CPUID_EDX_INVARIANT_TSC_AVAILABLE(CpuidRequest->EDX),
                     CPUID_EDX_INVARIANT_TSC_AVAILABLE(CpuidRequest->EDX) ? " (TSC runs at constant rate)" : "");

        break;

    case 0x80000008:
    {
        ShowMessages("==== CPUID.(EAX=80000008H) Virtual & Physical Address Sizes ====\n\n");
        ShowMessages("  *******************************************************\n"
                     "  *          LEAF 0x80000008 HAS NO SUBLEAVES           *\n"
                     "  *       ANY SUBLEAF YOU ENTER WILL DEFAULT TO 0       *\n"
                     "  *      AND THE PROCESSOR RETURNS UNDEFINED VALUES     *\n"
                     "  *******************************************************\n\n");

        ShowMessages("-- EAX --\n\n");
        UINT32 PhysicalBits = CPUID_EAX_NUMBER_OF_PHYSICAL_ADDRESS_BITS(CpuidRequest->EAX);
        UINT32 LinearBits   = CPUID_EAX_NUMBER_OF_LINEAR_ADDRESS_BITS(CpuidRequest->EAX);

        ShowMessages("  NumberOfPhysicalAddressBits = %u (max physical address = 2^%u = %llu bytes)\n",
                     PhysicalBits,
                     PhysicalBits,
                     (ULONG64)(1ULL << PhysicalBits));
        ShowMessages("  NumberOfLinearAddressBits  = %u (max linear address = 2^%u = %llu bytes)\n",
                     LinearBits,
                     LinearBits,
                     (ULONG64)(1ULL << LinearBits));

        ShowMessages("-- EBX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EBX_RESERVED(CpuidRequest->EBX));

        ShowMessages("-- ECX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_ECX_RESERVED(CpuidRequest->ECX));

        ShowMessages("-- EDX --\n\n");
        ShowMessages("  Reserved = 0x%08X\n\n", CPUID_EDX_RESERVED(CpuidRequest->EDX));

        break;
    }

    default:
        ShowMessages("==== CPUID.(EAX=%08XH) ====\n\n", FunctionId);
        ShowMessages("  CPUID leaf 0x%08X is not implemented in HyperDbg.\n", FunctionId);
        ShowMessages("  You can decode the raw values yourself:\n");
        ShowMessages("    EAX: 0x%08X\n", CpuidRequest->EAX);
        ShowMessages("    EBX: 0x%08X\n", CpuidRequest->EBX);
        ShowMessages("    ECX: 0x%08X\n", CpuidRequest->ECX);
        ShowMessages("    EDX: 0x%08X\n\n", CpuidRequest->EDX);
    }
}

/**
 * @brief ucpuid command handler
 *
 * @return VOID
 */
VOID
CommandCpuidRequestCpuid(UINT32 FunctionId, UINT32 SubFunctionId)
{
    BOOL                             Status;
    ULONG                            ReturnedLength;
    DEBUGGER_CPUID_REQUEST_RESPONSE  CpuidRequestBuffer = {0};
    PDEBUGGER_CPUID_REQUEST_RESPONSE CpuidRequest       = &CpuidRequestBuffer;
    CpuidRequest->FunctionId                            = FunctionId;
    CpuidRequest->SubFunctionId                         = SubFunctionId;

    if (g_IsSerialConnectedToRemoteDebuggee)
    {
        //
        // It's on a debugger mode
        //
        KdSendUserCpuidPacketToDebuggee(FunctionId, SubFunctionId);
        return;
    }
    else
    {
        //
        // It's on a local debugging mode
        //
        AssertShowMessageReturnStmt(g_IsKdModuleLoaded, g_DeviceHandle, ASSERT_MESSAGE_KD_NOT_LOADED, ASSERT_MESSAGE_DRIVER_NOT_LOADED, AssertReturn);

        //
        // By the way, we don't need to send an input buffer
        // to the kernel, but let's keep it like this, if we
        // want to pass some other arguments to the kernel in
        // the future
        //
        Status = PlatformDeviceIoControl(
            g_DeviceHandle,                         // Handle to device
            IOCTL_DEBUGGER_CPUID,                   // IO Control Code (IOCTL)
            CpuidRequest,                           // Input Buffer to driver.
            SIZEOF_DEBUGGER_CPUID_REQUEST_RESPONSE, // Input buffer length
            CpuidRequest,                           // Output Buffer from driver.
            SIZEOF_DEBUGGER_CPUID_REQUEST_RESPONSE, // Length of output buffer in
                                                    // bytes.
            &ReturnedLength,                        // Bytes placed in buffer.
            NULL                                    // synchronous call
        );

        if (!Status)
        {
            ShowMessages("ioctl failed with code 0x%x\n", PlatformGetLastError());
            return;
        }

        if (CpuidRequest->KernelStatus == DEBUGGER_OPERATION_WAS_SUCCESSFUL)
        {
            CommandShowUserCpuidMessage(FunctionId, SubFunctionId, CpuidRequest);
        }

        else
        {
            ShowMessages("Receiving CPUID result was not successful :(\n");
        }
    }
}

/**
 * @brief ucpuid command handler
 *
 * @param CommandTokens
 * @param Command
 *
 * @return VOID
 */
VOID
CommandUserCpuid(vector<CommandToken> CommandTokens, string Command)
{
    UINT32  FunctionId     = 0;
    UINT32  SubFunctionId  = 0;
    BOOL    SetFunctionId  = FALSE;
    BOOLEAN IsFirstCommand = TRUE;

    if (CommandTokens.size() > 3)
    {
        ShowMessages("incorrect use of the '%s'\n\n",
                     GetCaseSensitiveStringFromCommandToken(CommandTokens.at(0)).c_str());
        CommandUserCpuidHelp();
        return;
    }

    for (auto Section : CommandTokens)
    {
        if (IsFirstCommand == TRUE)
        {
            IsFirstCommand = FALSE;
            continue;
        }

        //
        // Parse FunctionId (first numeric parameter)
        //
        if (!SetFunctionId)
        {
            if (!ConvertTokenToUInt32(Section, &FunctionId))
            {
                ShowMessages("please specify a correct hex value for function id\n\n");
                CommandUserCpuidHelp();
                return;
            }
            SetFunctionId = TRUE;
            continue;
        }

        //
        // Parse SubFunctionId (second numeric parameter, optional)
        //
        if (!ConvertTokenToUInt32(Section, &SubFunctionId))
        {
            ShowMessages("please specify a correct hex value for sub-function id\n\n");
            CommandUserCpuidHelp();
            return;
        }
    }

    //
    // Check if FunctionId was provided
    //
    if (!SetFunctionId)
    {
        ShowMessages("please specify a cpuid function id\n\n");
        CommandUserCpuidHelp();
        return;
    }

    //
    // Call CPUID with user-provided inputs
    //
    CommandCpuidRequestCpuid(FunctionId, SubFunctionId);
}
