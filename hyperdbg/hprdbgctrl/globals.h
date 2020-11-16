/**
 * @file globals.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Global Variables for user-mode interface
 * @details
 * @version 0.1
 * @date 2020-07-13
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//		 Remote and Local Connection            //
//////////////////////////////////////////////////

/**
 * @brief Shows whether the user is allowed to use 'load' command
 * to load modules locally in VMI (virtual machine introspection) mode
 *
 */
BOOLEAN g_IsConnectedToHyperDbgLocally = FALSE;

/**
 * @brief Shows whether the current debugger is the host and
 * connected to a remote debuggee (guest)
 *
 */
BOOLEAN g_IsConnectedToRemoteDebuggee = FALSE;

/**
 * @brief  Shows whether the current system is a guest (debuggee)
 * and a remote debugger is connected to this system
 *
 */
BOOLEAN g_IsConnectedToRemoteDebugger = FALSE;

/**
 * @brief We use this variable because this causes to not show
 * the debugger signature immediately and wait for the remote
 * deubgee to send its message then we can show the signature
 *
 */
BOOLEAN g_IsRemoteDebuggerMessageReceived = TRUE;

/**
 * @brief The socket object of host debugger (not debuggee)
 * it is because in HyperDbg, debuggee is server and debugger
 * is a client
 *
 */
SOCKET g_ClientConnectSocket = {0};

/**
 * @brief The socket object of guest debuggee (not debugger)
 * it is because in HyperDbg, debugger is client and debuggee
 * is a server
 *
 */
SOCKET g_SeverSocket = {0};

/**
 * @brief Server in debuggee needs an extra socket
 *
 */
SOCKET g_ServerListenSocket = {0};

/**
 * @brief In debugger (not debuggee), we save the port of server
 * debuggee in this variable to use it later e.g, in signature
 *
 */
string g_ServerPort = "";

/**
 * @brief In debugger (not debuggee), we save the port of server
 * debuggee in this variable to use it later e.g, in signature
 *
 */
string g_ServerIp = "";

/**
 * @brief In debugger (not debuggee), we save the ip of server
 * debuggee in this variable to use it later e.g, in signature
 *
 */
HANDLE g_RemoteDebuggeeListeningThread = NULL;

//////////////////////////////////////////////////
//				 Global Variables               //
//////////////////////////////////////////////////

/**
 * @brief this variable is used to indicate that modules
 * are loaded so we make sure to later use a trace of
 * loading in 'unload' command
 *
 */
BOOLEAN g_IsDebuggerModulesLoaded = FALSE;

/**
 * @brief State of debugging threads
 *
 */
DEBUGGING_STATE g_DebuggingState = {0};

/**
 * @brief This variable holds the trace and generate numbers
 * for new tags of events
 *
 */
UINT64 g_EventTag = DebuggerEventTagStartSeed;

/**
 * @brief This variable holds the trace and generate numbers
 * for unique tag of the output resources
 *
 */
UINT64 g_OutputSourceTag = DebuggerOutputSourceTagStartSeed;

/**
 * @brief it shows whether the debugger started using
 * events or not or in other words, is g_EventTrace
 * initialized with a variable or it is empty
 *
 */
BOOLEAN g_EventTraceInitialized = FALSE;

/**
 * @brief Holds a list of events in kernel and the state of events
 * and the commands to show the state of each command (disabled/enabled)
 *
 * @details this list is not have any relation with the things that HyperDbg
 * holds for each event in the kernel
 *
 */
LIST_ENTRY g_EventTrace = {0};

/**
 * @brief it shows whether the debugger started using
 * output sources or not or in other words, is g_OutputSources
 * initialized with a variable or it is empty
 *
 */
BOOLEAN g_OutputSourcesInitialized = FALSE;

/**
 * @brief Holds a list of output sources created by output command
 *
 * @details user-mode events and output sources are two separate things
 * in HyperDbg
 *
 */
LIST_ENTRY g_OutputSources = {0};

/**
 * @brief Holds the location driver to install it
 *
 */
TCHAR g_DriverLocation[MAX_PATH] = {0};

/**
 * @brief Holds the location test-hyperdbg.exe
 *
 */
TCHAR g_TestLocation[MAX_PATH] = {0};

/**
 * @brief The handler for ShowMessages function
 * this is because the user might choose not to use
 * printf and instead use his/her handler for showing
 * messages
 *
 */
Callback g_MessageHandler = 0;

/**
 * @brief Shows whether the vmxoff process start or not
 *
 */
BOOLEAN g_IsVmxOffProcessStart;

/**
 * @brief Holds the global handle of device which is used
 * to send the request to the kernel by IOCTL, this
 * handle is not used for IRP Pending of message tracing
 *
 */
HANDLE g_DeviceHandle;

/**
 * @brief Shows whether the '.logopen' command is executed
 * and the log file is open or not
 *
 */
BOOLEAN g_LogOpened = FALSE;

/**
 * @brief The object of log file ('.logopen' command)
 *
 */
ofstream g_LogOpenFile;

/**
 * @brief Shows whether the target is executing a script
 * form '.script' command or executing script by an
 * argument
 *
 */
BOOLEAN g_ExecutingScript = FALSE;

/**
 * @brief Shows whether the pause command or CTRL+C
 * or CTRL+Break is executed or not
 *
 */
BOOLEAN g_BreakPrintingOutput = FALSE;

/**
 * @brief Shows whether the user executed and mesaured '!measure'
 * command or not, it is because we want to use these measurements
 * later in '!hide' command
 *
 */
BOOLEAN g_TransparentResultsMeasured = FALSE;

/**
 * @brief The average calculated from the measurements of cpuid
 * '!measure' command
 */
UINT64 g_CpuidAverage = 0;

/**
 * @brief The standard deviation calculated from the measurements of cpuid
 * '!measure' command
 */
UINT64 g_CpuidStandardDeviation = 0;

/**
 * @brief The median calculated from the measurements of cpuid
 * '!measure' command
 */
UINT64 g_CpuidMedian = 0;

/**
 * @brief The average calculated from the measurements of rdtsc/p
 * '!measure' command
 */
UINT64 g_RdtscAverage = 0;

/**
 * @brief The standard deviation calculated from the measurements
 *  of rdtsc/p '!measure' command
 */
UINT64 g_RdtscStandardDeviation = 0;

/**
 * @brief The median calculated from the measurements of rdtsc/p
 * '!measure' command
 */
UINT64 g_RdtscMedian = 0;

//////////////////////////////////////////////////
//			     	 Settings			        //
//////////////////////////////////////////////////

/**
 * @brief Whether auto-unpause mode is enabled or not enabled
 * @details it is enabled by default
 *
 */
BOOLEAN g_AutoUnpause = TRUE;

/**
 * @brief Whether auto-flush mode is enabled or not enabled
 * @details it is disabled by default
 *
 */
BOOLEAN g_AutoFlush = FALSE;

/**
 * @brief Shows the syntax used in !u !u2 u u2 commands
 * @details INTEL = 1, ATT = 2, MASM = 3
 *
 */
UINT32 g_DisassemblerSyntax = 1;
