/**
 * @file Transparency.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief hide the debugger from anti-debugging and anti-hypervisor methods (headers)
 * @details
 * @version 0.1
 * @date 2020-07-07
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				      Locks 	    			//
//////////////////////////////////////////////////

/**
 * @brief The lock for modifying list of process/thread for transparent-mode trap flags
 *
 */
volatile LONG TransparentModeTrapListLock;

//////////////////////////////////////////////////
//				   Definitions					//
//////////////////////////////////////////////////

/**
 * @brief IA32_TIME_STAMP_COUNTER MSR (rcx)
 *
 */
#define MSR_IA32_TIME_STAMP_COUNTER 0x10

/**
 * @brief Maximum value that can be returned by the rand function
 *
 */
#define RAND_MAX 0x7fff

/**
 * @brief maximum number of thread/process ids to be allocated for keeping track of
 * of the trap flag
 *
 */
#define MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRANSPARENT_MODE_TRAPS 500

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

/**
 * @brief The measurements from user-mode and kernel-mode
 *
 */
typedef struct _TRANSPARENCY_MEASUREMENTS
{
    UINT64 CpuidAverage;
    UINT64 CpuidStandardDeviation;
    UINT64 CpuidMedian;

    UINT64 RdtscAverage;
    UINT64 RdtscStandardDeviation;
    UINT64 RdtscMedian;

    LIST_ENTRY ProcessList;

} TRANSPARENCY_MEASUREMENTS, *PTRANSPARENCY_MEASUREMENTS;

/**
 * @brief The ProcessList of TRANSPARENCY_MEASUREMENTS is from this architecture
 *
 */
typedef struct _TRANSPARENCY_PROCESS
{
    UINT32     ProcessId;
    PVOID      ProcessName;
    PVOID      BufferAddress;
    BOOLEAN    TrueIfProcessIdAndFalseIfProcessName;
    LIST_ENTRY OtherProcesses;

} TRANSPARENCY_PROCESS, *PTRANSPARENCY_PROCESS;

/**
 * @brief The thread/process information
 *
 */
typedef struct _TRANSPARENT_MODE_PROCESS_THREAD_INFORMATION
{
    union
    {
        UINT64 asUInt;

        struct
        {
            UINT32 ProcessId;
            UINT32 ThreadId;
        } Fields;
    };

} TRANSPARENT_MODE_PROCESS_THREAD_INFORMATION, *PTRANSPARENT_MODE_PROCESS_THREAD_INFORMATION;

/**
 * @brief The (optional) context parameters for the transparent-mode
 *
 */
typedef struct _TRANSPARENT_MODE_CONTEXT_PARAMS
{
    UINT64 OptionalParam1; // Optional parameter
    UINT64 OptionalParam2; // Optional parameter
    UINT64 OptionalParam3; // Optional parameter
    UINT64 OptionalParam4; // Optional parameter

} TRANSPARENT_MODE_CONTEXT_PARAMS, *PTRANSPARENT_MODE_CONTEXT_PARAMS;

/**
 * @brief The threads that we expect to get the trap flag
 *
 * @details Used for keeping track of the threads that we expect to get the trap flag
 *
 */
typedef struct _TRANSPARENT_MODE_TRAP_FLAG_STATE
{
    UINT32                                      NumberOfItems;
    TRANSPARENT_MODE_PROCESS_THREAD_INFORMATION ThreadInformation[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRANSPARENT_MODE_TRAPS];
    UINT64                                      Context[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRANSPARENT_MODE_TRAPS];
    TRANSPARENT_MODE_CONTEXT_PARAMS             Params[MAXIMUM_NUMBER_OF_THREAD_INFORMATION_FOR_TRANSPARENT_MODE_TRAPS];

} TRANSPARENT_MODE_TRAP_FLAG_STATE, *PTRANSPARENT_MODE_TRAP_FLAG_STATE;

/**
 * @brief System Information for Code Integrity
 *
 */
typedef struct _SYSTEM_CODEINTEGRITY_INFORMATION
{
    ULONG Length;
    ULONG CodeIntegrityOptions;

} SYSTEM_CODEINTEGRITY_INFORMATION, *PSYSTEM_CODEINTEGRITY_INFORMATION;

/**
 * @brief System Information for running processes
 *
 */
typedef struct _SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    BYTE Reserved1[48];
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    PVOID Reserved2;
    ULONG HandleCount;
    ULONG SessionId;
    PVOID Reserved3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG Reserved4;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    PVOID Reserved5;
    SIZE_T QuotaPagedPoolUsage;
    PVOID Reserved6;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved7[6];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

/**
 * @brief A list of common Hypervisor specific process executables
 *
 */
static const PWCH HV_Processes[] = {
    L"hyperdbg-cli.exe",
    L"vboxservice.exe",
    L"vmsrvc.exe",
    L"xenservice.exe",
    L"vm3dservice.exe",
    L"VGAuthService.exe",
    L"vmtoolsd.exe",
    L"vdagent.exe",
    L"vdservice.exe",
    L"qemuwmi.exe",
    L"vboxtray.exe",
    L"VBoxControl.exe",
    L"VGAuthService.exe",

    //
    // Common Debugging Tools
    //
    L"ollydbg.exe",
    L"ollyice.exe",
    L"ProcessHacker.exe",
    L"tcpview.exe",
    L"autoruns.exe",
    L"autorunsc.exe",
    L"filemon.exe",
    L"procmon.exe",
    L"regmon.exe",
    L"procexp.exe",
    L"idaq.exe",
    L"idaq64.exe",
    L"ImmunityDebugger.exe",
    L"Wireshark.exe",
    L"dumpcap.exe",
    L"HookExplorer.exe",
    L"ImportREC.exe",
    L"PETools.exe",
    L"LordPE.exe",
    L"SysInspector.exe",
    L"proc_analyzer.exe",
    L"sysAnalyzer.exe",
    L"sniff_hit.exe",
    L"windbg.exe",
    L"joeboxcontrol.exe",
    L"joeboxserver.exe",
    L"ResourceHacker.exe",
    L"x32dbg.exe",
    L"x64dbg.exe",
    L"Fiddler.exe",
    L"httpdebugger.exe",
    L"cheatengine-i386.exe",
    L"cheatengine-x86_64.exe",
    L"cheatengine-x86_64-SSE4-AVX2.exe",
    L"frida-helper-32.exe",
    L"frida-helper-64.exe"

};

/**
 * @brief A list of common Hypervisor specific drivers
 *
 */
static const PCHAR HV_DRIVER[] = {
    "hyperkd",

    //
    // Drivers from VBox
    //

    "VBoxGuest",
    "VBoxMouse",
    "VBoxSF",

    //
    // Drivers from VMWare Tools
    //

    "vmrawdsk",
    "giappdef",
    "glxgi",
    "vm3dmp",
    "vm3dmp_loader",
    "vm3dmp-debug",
    "vm3dmp-stats",
    "vmxnet3",
    "vmxnet",
    "vnetWFP",
    "vnetflt",
    "vsepflt",
    "pvscsi",
    "vmmemctl",
    "vmhgfs",
    "vmusbmouse",
    "vmmouse",
    "vmaudio",
    "vmci",
    "vsock",

    //
    // Hyper-V Drivers
    //

    "VMBusHID",
    "vmbus",
    "vmgid",
    "IndirectKmd",
    "HyperVideo",
    "hyperkbd",

};

/**
 * @brief A list of common Hypervisor specific files
 *
 */
static const PWCH HV_FILES[] = {
        //
        // Hyperdbg Files
        //
        L"hyperhv",
        L"hyperkd"
        L"hyperlog",
        L"libhyperdbg",



        //
        // VMWare Files
        //
        L"vmmouse.sys",
        L"Vmmouse.sys",
        L"vmusbmouse.sys",
        L"Vmusbmouse.sys",
        L"vm3dgl.dll",
        L"vmdum.dll",
        L"VmGuestLibJava.dll",
        L"vm3dver.dll",
        L"vmtray.dll",
        L"VMToolsHook.dll",
        L"vmGuestLib.dll",
        L"vmhgfs.dll",
        L"vmhgfs.sys",
        L"vm3dum64_loader.dll",
        L"vm3dum64_10.dll",
        L"vmnet.sys",
        L"vmusb.sys",
        L"vm3dmp.sys",
        L"vmci.sys",
        L"vmmemctl.sys",
        L"vmx86.sys",
        L"vmrawdsk.sys",
        L"vmkdb.sys",
        L"vmnetuserif.sys",
        L"vmnetadapter.sys",
        L"VMware Tools",
        L"VMWare",



        //
        // VirtualBox Files
        //
        L"VBoxMouse.sys",
        L"VBoxGuest.sys",
        L"VBoxSF.sys",
        L"VBoxVideo.sys",
        L"vboxoglpackspu.dll",
        L"vboxoglpassthroughspu.dll",
        L"vboxservice.exe",
        L"vboxoglcrutil.dll",
        L"vboxdisp.dll",
        L"vboxhook.dll",
        L"vboxmrxnp.dll",
        L"vboxogl.dll",
        L"vboxtray.exe",
        L"VBoxControl.exe",
        L"vboxoglerrorspu.dll",
        L"vboxoglfeedbackspu.dll",
        L"vboxoglarrayspu.dll",
        L"vboxmrxnp.dll",
        L"virtualbox guest additions",

        //
        // KVM files
        //
        L"balloon.sys",
        L"netkvm.sys",
        L"pvpanic.sys",
        L"viofs.sys",
        L"viogpudo.sys",
        L"vioinput.sys",
        L"viorng.sys",
        L"vioscsi.sys",
        L"vioser.sys",
        L"viostor.sys",

        //
        // VPC files
        //
        L"vmsrvc.sys",
        L"vmusrvc.sys",
        L"vmsrvc.exe",
        L"vmusrvc.exe",
        L"vpc-s3.sys",
        L"Virtio-Win",
        
        L"qemu-ga",
        L"SPICE Guest Tools",
};

/**
 * @brief A list of common Hypervisor specific directories
 *
 */
static const PWCH HV_DIRS[] = {
    L"hyperhv",
    L"VMware Tools",
    L"Virtio-Win",
    L"qemu-ga",
    L"SPICE Guest Tools",
    L"VMWare",
    L"virtualbox guest additions",
};

/**
 * @brief A list of common Hypervisor firmware entries
 * 
 * @details The list contains both a normal and uppercase versions of the entries, for better compatibility
 *
 */
static const PWCH HV_FIRM_NAMES[] = {
    "VMWARE",
    "VMware",
    "VS2005R2",
    "VirtualBox",
    "VIRTUALBOX",
    "Oracle",
    "ORACLE",
    "Innotek",
    "INNOTEK",
};

/**
 * @brief A list of Registry keys that might contain indetifiable hypervisor vendor data
 * 
 * @details The keys are spread out in the registry with different paths, where only the final key name is matched
 *
 */
static const PWCH TRANSPARENT_DETECTABLE_REGISTRY_KEYS[] = {
    L"Identifier",
    L"SystemBiosVersion",
    L"VideoBiosVersion",
    L"ProductID",
    L"SystemManufacturer",
    L"AcpiData",
    L"SMBiosData",
    L"SystemProductName",
    L"DeviceDesc",
    L"FriendlyName",
    L"DisplayName",
    L"ProviderName",
    L"Device Description",

};

/**
 * @brief A list of Registry keys speciffic indetifiable hypervisor vendors
 * 
 *
 */
static const PWCH HV_REGKEYS[] = {
    L"VMware Tools",
    L"qemu-ga",
    L"SPICE Guest Tools",
    L"VMware",
    L"VMWARE",
    L"VEN_15A",
    L"VMware, Inc.",
    L"INNOTEK GMBH",
    L"VIRTUALBOX",
    L"SUN MICROSYSTEMS",
    L"VBOXVER",
    L"VIRTUAL MACHINE",
    L"GOOGLE COMPUTE ENGINE",
    L"VBOXAPIC",
};
//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

VOID
TransparentCPUID(INT32 CpuInfo[], PGUEST_REGS Regs);

BOOLEAN
TransparentSetTrapFlagAfterSyscall(VIRTUAL_MACHINE_STATE *           VCpu,
                                   UINT32                            ProcessId,
                                   UINT32                            ThreadId,
                                   UINT64                            Context,
                                   TRANSPARENT_MODE_CONTEXT_PARAMS * Params);

BOOLEAN
TransparentCheckAndHandleAfterSyscallTrapFlags(VIRTUAL_MACHINE_STATE * VCpu,
                                               UINT32                  ProcessId,
                                               UINT32                  ThreadId);

VOID
TransparentCallbackHandleAfterSyscall(VIRTUAL_MACHINE_STATE *           VCpu,
                                      UINT32                            ProcessId,
                                      UINT32                            ThreadId,
                                      UINT64                            Context,
                                      TRANSPARENT_MODE_CONTEXT_PARAMS * Params);

VOID
TransparentHandleSystemCallHook(VIRTUAL_MACHINE_STATE* VCpu);

VOID
TransparentHandleNtQuerySystemInformationSyscall(VIRTUAL_MACHINE_STATE* VCpu);

VOID
TransparentHandleNtQueryAttributesFileSyscall(VIRTUAL_MACHINE_STATE* VCpu);

VOID
TransparentHandleNtOpenDirectoryObjectSyscall(VIRTUAL_MACHINE_STATE* VCpu);

VOID
TransparentHandleNtQueryInformationProcessSyscall(VIRTUAL_MACHINE_STATE* VCpu);

VOID
TransparentHandleNtOpenFileSyscall(VIRTUAL_MACHINE_STATE* VCpu);

VOID
TransparentHandleNtOpenKeySyscall(VIRTUAL_MACHINE_STATE* VCpu);

VOID
TransparentHandleNtQueryValueKeySyscall(VIRTUAL_MACHINE_STATE* VCpu);

UINT32
TransparentGetRand();