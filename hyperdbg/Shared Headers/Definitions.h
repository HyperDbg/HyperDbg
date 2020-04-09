
//////////////////////////////////////////////////
//				Message Tracing					//
//////////////////////////////////////////////////

// Default buffer size
#define MaximumPacketsCapacity 1000 // number of packets
#define PacketChunkSize		1000 // NOTE : REMEMBER TO CHANGE IT IN USER-MODE APP TOO
#define UsermodeBufferSize  sizeof(UINT32) + PacketChunkSize + 1 /* Becausee of Opeation code at the start of the buffer + 1 for null-termminating */
#define LogBufferSize MaximumPacketsCapacity * (PacketChunkSize + sizeof(BUFFER_HEADER))

#define SIZEOF_REGISTER_EVENT  sizeof(REGISTER_EVENT)

#define DbgPrintLimitation  512


//////////////////////////////////////////////////
//					Events						//
//////////////////////////////////////////////////

typedef enum _NOTIFY_TYPE
{
	IRP_BASED,
	EVENT_BASED
}NOTIFY_TYPE;

typedef struct _REGISTER_EVENT
{
	NOTIFY_TYPE Type;
	HANDLE  hEvent;

} REGISTER_EVENT, * PREGISTER_EVENT;


//////////////////////////////////////////////////
//					Installer 					//
//////////////////////////////////////////////////

#define DRIVER_NAME       "hprdbghv"


//////////////////////////////////////////////////
//				Operation Codes					//
//////////////////////////////////////////////////

// Message area >= 0x4
#define OPERATION_LOG_INFO_MESSAGE							0x1
#define OPERATION_LOG_WARNING_MESSAGE						0x2
#define OPERATION_LOG_ERROR_MESSAGE							0x3
#define OPERATION_LOG_NON_IMMEDIATE_MESSAGE					0x4

//////////////////////////////////////////////////
//		    	Callback Definitions			//
//////////////////////////////////////////////////

typedef int(__stdcall* Callback)(const char* Text);



//////////////////////////////////////////////////
//				Debugger Actions				//
//////////////////////////////////////////////////

typedef enum _DEBUGGER_ACTION_ENUM
{
	BREAK_TO_DEBUGGER,
	LOG_TO_DEBUGGER

}DEBUGGER_ACTION_ENUM;

// Structure 
typedef struct _DEBUGGER_ACTION
{
	DEBUGGER_ACTION_ENUM Action;
	BOOLEAN ImmediatelySendTheResults;

} DEBUGGER_ACTION, * PDEBUGGER_ACTION;


//////////////////////////////////////////////////
//				Debugger Codes					//
//////////////////////////////////////////////////

/* ------------------------- DEBUGGER_EPT_SYSCALL_HOOK_EFER ------------------------- */

// Commands
#define DEBUGGER_EPT_SYSCALL_HOOK_EFER				0x803

// Method
typedef enum _SYSCALL_HOOK_METHOD
{
	SYSCALL_HOOK_EFER

}SYSCALL_HOOK_METHOD;

// Type
typedef enum _SYSCALL_HOOK_TYPE
{
	ALL_SYSCALLS,
	SPECIFIC_PROCESS_SYSCALLS

}SYSCALL_HOOK_TYPE;

// Structure 
typedef struct _DEBUGGER_EPT_SYSCALL_HOOK_EFER_STRUCT
{
	UINT64 Tag;
	DEBUGGER_ACTION Action;
	SYSCALL_HOOK_METHOD Method;
	SYSCALL_HOOK_TYPE Type;
	UINT32 ProcessId;

} DEBUGGER_EPT_SYSCALL_HOOK_EFER_STRUCT, * PDEBUGGER_EPT_SYSCALL_HOOK_EFER_STRUCT;

/* ---------------------------------------------------------------------------------- */



//////////////////////////////////////////////////
//					IOCTLs						//
//////////////////////////////////////////////////

#define IOCTL_REGISTER_EVENT \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_TERMINATE_VMX \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_DEBUGGER_EPT_SYSCALL_HOOK_EFER \
   CTL_CODE( FILE_DEVICE_UNKNOWN, DEBUGGER_EPT_SYSCALL_HOOK_EFER, METHOD_BUFFERED, FILE_ANY_ACCESS )
