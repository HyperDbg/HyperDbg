/**
 * @file SyscallFootprints.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Hide the debugger from SYSCALL anti-debugging and anti-hypervisor methods (headers)
 * @details
 * @version 0.14
 * @date 2024-06-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//				      Enums		    			//
//////////////////////////////////////////////////

/**
 * @brief System information class
 *
 */
typedef enum _SYSTEM_INFORMATION_CLASS
{
    SystemProcessInformation         = 0x05,
    SystemExtendedProcessInformation = 0x39,
    SystemFullProcessInformation     = 0x94,
    SystemModuleInformation          = 0x0B,
    SystemKernelDebuggerInformation  = 0x23,
    SystemCodeIntegrityInformation   = 0x67,
    SystemFirmwareTableInformation   = 0x4C,
} SYSTEM_INFORMATION_CLASS,
    *PSYSTEM_INFORMATION_CLASS;

//////////////////////////////////////////////////
//				   Structures					//
//////////////////////////////////////////////////

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
typedef struct _SYSTEM_PROCESS_INFORMATION
{
    ULONG          NextEntryOffset;
    ULONG          NumberOfThreads;
    BYTE           Reserved1[48];
    UNICODE_STRING ImageName;
    KPRIORITY      BasePriority;
    HANDLE         UniqueProcessId;
    PVOID          Reserved2;
    ULONG          HandleCount;
    ULONG          SessionId;
    PVOID          Reserved3;
    SIZE_T         PeakVirtualSize;
    SIZE_T         VirtualSize;
    ULONG          Reserved4;
    SIZE_T         PeakWorkingSetSize;
    SIZE_T         WorkingSetSize;
    PVOID          Reserved5;
    SIZE_T         QuotaPagedPoolUsage;
    PVOID          Reserved6;
    SIZE_T         QuotaNonPagedPoolUsage;
    SIZE_T         PagefileUsage;
    SIZE_T         PeakPagefileUsage;
    SIZE_T         PrivatePageCount;
    LARGE_INTEGER  Reserved7[6];
} SYSTEM_PROCESS_INFORMATION, *PSYSTEM_PROCESS_INFORMATION;

/**
 * @brief SSDT structure
 *
 */
typedef struct _SSDTStruct
{
    LONG * pServiceTable;
    PVOID  pCounterTable;
#ifdef _WIN64
    UINT64 NumberOfServices;
#else
    ULONG NumberOfServices;
#endif
    PCHAR pArgumentTable;
} SSDTStruct, *PSSDTStruct;

/**
 * @brief Module entry
 *
 */
typedef struct _SYSTEM_MODULE_ENTRY
{
    HANDLE Section;
    PVOID  MappedBase;
    PVOID  ImageBase;
    ULONG  ImageSize;
    ULONG  Flags;
    UINT16 LoadOrderIndex;
    UINT16 InitOrderIndex;
    UINT16 LoadCount;
    UINT16 OffsetToFileName;
    UCHAR  FullPathName[256];
} SYSTEM_MODULE_ENTRY, *PSYSTEM_MODULE_ENTRY;

/**
 * @brief System Information for modules
 *
 */
typedef struct _SYSTEM_MODULE_INFORMATION
{
    ULONG               Count;
    SYSTEM_MODULE_ENTRY Module[1];

} SYSTEM_MODULE_INFORMATION, *PSYSTEM_MODULE_INFORMATION;

typedef NTSTATUS(NTAPI * ZWQUERYSYSTEMINFORMATION)(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID                   SystemInformation,
    IN ULONG                    SystemInformationLength,
    OUT PULONG ReturnLength     OPTIONAL);

NTSTATUS(*NtCreateFileOrig)
(
    PHANDLE            FileHandle,
    ACCESS_MASK        DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK   IoStatusBlock,
    PLARGE_INTEGER     AllocationSize,
    ULONG              FileAttributes,
    ULONG              ShareAccess,
    ULONG              CreateDisposition,
    ULONG              CreateOptions,
    PVOID              EaBuffer,
    ULONG              EaLength);

//////////////////////////////////////////////////
//				     Globals        			//
//////////////////////////////////////////////////

/**
 * @brief A variable holding the randomly chosen index for the genuine vendor list.
 *        This is used for transparent vendor spoofing
 */
static WORD TRANSPARENT_GENUINE_VENDOR_STRING_INDEX = 0;

/**
 * @brief System call numbers information
 */
SYSTEM_CALL_NUMBERS_INFORMATION g_SystemCallNumbersInformation;

//////////////////////////////////////////////////
//				   Constants        			//
//////////////////////////////////////////////////

#if DISABLE_HYPERDBG_HYPEREVADE == FALSE

/**
 * @brief A list of windows processes, for which to ignore systemcall requests
 * when the transparency mode is enabled
 *
 */
static const PCHAR TRANSPARENT_WIN_PROCESS_IGNORE[] = {
    "winlogon.exe",
    "wininit.exe",
    "csrss.exe",
    "sihost.exe",
    "explorer.exe",
};

/**
 * @brief A brief list of genuine system vendors device ID's used for manufacturer spoofing
 *
 */
static const PWCHAR TRANSPARENT_LEGIT_DEVICE_ID_VENDOR_STRINGS_WCHAR[] = {
    L"VEN_8086", // Intel
    L"VEN_10DE", // NVIDIA
    L"VEN_1002", // AMD
    L"VEN_10EC", // Realtek

};

/**
 * @brief A list of genuine system vendors used for manufacturer spoofing
 *
 */
static const PWCHAR TRANSPARENT_LEGIT_VENDOR_STRINGS_WCHAR[] = {
    L"ASUS",
    L"ASUSTeK Computer INC.",
    L"ASUSTek",
    L"ASUSTEK COMPUTER INC.",

    // MSI
    L"Micro-Star International Co., Ltd.",
    L"MSI",
    L"MICRO-STAR INTERNATIONAL CO., LTD",

    // Gigabyte
    L"Gigabyte Technology Co., Ltd.",
    L"GIGABYTE",
    L"Gigabyte Technology",

    // ASRock
    L"ASRock",
    L"ASRock Incorporation",
    L"ASRock Inc.",

    // Dell
    L"Dell Inc.",
    L"DELL",
    L"Dell Computer Corporation",

    // HP
    L"Hewlett-Packard",
    L"Hewlett Packard",
    L"HP",
    L"HP Inc.",

    // Lenovo
    L"LENOVO",
    L"Lenovo Group Limited",

    // Intel
    L"Intel Corporation",
    L"Intel",

    // EVGA
    L"EVGA Corporation",
    L"EVGA",
};

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
    L"frida-helper-64.exe",

};

/**
 * @brief A list of common Hypervisor specific drivers
 *
 */
static const PCHAR HV_DRIVER[] = {
    "hyperkd",
    "hyperhv",

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
    // HyperDbg Files
    //
    L"hyperhv",
    L"hyperkd"
    L"hyperlog",
    L"libhyperdbg",

    //
    // VMware Files
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
    L"VMware",
    L"VMWARE",
    L"virtualbox guest additions",
    L"VirtualBox Guest Additions",
};

/**
 * @brief A list of common Hypervisor specific registry keys
 *
 */
static const PWCH HV_REGKEYS[] = {

    //
    // PCI device vendor id's
    //
    // NOTE: These need to stay at the top of the list
    //
    L"VEN_80EE",
    L"VEN_15AD",
    L"VEN_5333",

    //
    // Common names
    //
    L"Virtual",
    L"VIRTUAL",
    L"virtual",
    L"Hypervisor",
    L"hypervisor",
    L"HYPERVISOR",

    //
    // VMWare
    //
    L"VMware Tools",
    L"VMware, Inc.",
    L"vmusbmouse",
    L"VMware",
    L"VMWARE",
    L"VMWare",
    L"vmdebug",
    L"vmmouse",
    L"VMTools",
    L"VMMEMCTL",
    L"vmware tools",
    L"VMW0001",
    L"VMW0002",
    L"VMW0003",

    L"sandbox",
    L"Sandboxie",

    //
    // VirtualBox
    //
    L"VirtualBox Guest Additions",
    L"VBOX__",
    L"VBoxGuest",
    L"VBoxMouse",
    L"VBoxService",
    L"VBoxSF",
    L"VBoxVideo",
    L"VIRTUALBOX",
    L"SUN MICROSYSTEMS",
    L"VBOXVER",
    L"VBOXAPIC",
    L"INNOTEK GMBH",

    //
    // QEMU
    //
    L"qemu-ga",
    L"SPICE Guest Tools",

    //
    // VPC
    //
    L"vpcbus",
    L"vpc-s3",
    L"vpcuhub",
    L"msvmmouf",
    L"Wine",
    L"xen",
    L"VIRTUAL MACHINE",
    L"GOOGLE COMPUTE ENGINE",
    L"sandbox",
    L"Sandboxie",

    //
    // KVM
    //
    L"vioscsi",
    L"viostor",
    L"VirtIO-FS Service",
    L"VirtioSerial",
    L"BALLOON",
    L"BalloonService",
    L"netkvm",

};

//
// @brief A list of registry keys which might contain hypervisor vendor information in their data
//
// NOTE: This is not a complete list, there are a lot of generic keys that also can have the identifiable data
//
static const PWCH TRANSPARENT_DETECTABLE_REGISTRY_KEYS[] = {
    L"AcpiData",
    L"SMBiosData",
    L"Identifier",
    L"SystemBiosVersion",
    L"VideoBiosVersion",
    L"ProductID",
    L"SystemManufacturer",
    L"SystemProductName",
    L"DeviceDesc",
    L"FriendlyName",
    L"DisplayName",
    L"ProviderName",
    L"Device Description",
    L"BIOSVendor",
    L"DriverDesc",
    L"InfSection",
    L"Service",
    L"0",
    L"1",
    L"00000000",
};

/**
 * @brief A list of common Hypervisor firmware entries
 *
 * @details The list contains both a normal and uppercase versions of the entries, for better compatibility
 *
 */
static const PCHAR HV_FIRM_NAMES[] = {
    "440BX Desktop Reference Platform",
    "VMWARE, INC.",
    "VMware, Inc.",
    "VMWARE",
    "VMware",
    "VMW",
    "VS2005R2",
    "VirtualBox",
    "VIRTUALBOX",
    "Oracle",
    "ORACLE",
    "Innotek",
    "INNOTEK",
    "Virtual",

};

#endif

//////////////////////////////////////////////////
//				   Functions					//
//////////////////////////////////////////////////

VOID
TransparentHandleNtQuerySystemInformationSyscall(GUEST_REGS * Regs);

VOID
TransparentHandleNtQueryAttributesFileSyscall(GUEST_REGS * Regs);

VOID
TransparentHandleNtSystemDebugControlSyscall(GUEST_REGS * Regs);

VOID
TransparentHandleNtOpenDirectoryObjectSyscall(GUEST_REGS * Regs);

VOID
TransparentHandleNtQueryInformationProcessSyscall(GUEST_REGS * Regs);

VOID
TransparentHandleNtOpenFileSyscall(GUEST_REGS * Regs);

VOID
TransparentHandleNtOpenKeySyscall(GUEST_REGS * Regs);

VOID
TransparentHandleNtQueryValueKeySyscall(GUEST_REGS * Regs);

VOID
TransparentHandleNtEnumerateKeySyscall(GUEST_REGS * Regs);
