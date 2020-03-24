


// Default buffer size
#define MaximumPacketsCapacity 1000 // number of packets
#define PacketChunkSize		1000 // NOTE : REMEMBER TO CHANGE IT IN USER-MODE APP TOO
#define UsermodeBufferSize  sizeof(UINT32) + PacketChunkSize + 1 /* Becausee of Opeation code at the start of the buffer + 1 for null-termminating */
#define LogBufferSize MaximumPacketsCapacity * (PacketChunkSize + sizeof(BUFFER_HEADER))

#define SIZEOF_REGISTER_EVENT  sizeof(REGISTER_EVENT)

#define DbgPrintLimitation  512



#define IOCTL_REGISTER_EVENT \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS )

#define IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL \
   CTL_CODE( FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS )



typedef enum {
	IRP_BASED,
	EVENT_BASED
} NOTIFY_TYPE;

typedef struct _REGISTER_EVENT
{
	NOTIFY_TYPE Type;
	HANDLE  hEvent;

} REGISTER_EVENT, * PREGISTER_EVENT;



//////////////////////////////////////////////////
//				Operation Codes					//
//////////////////////////////////////////////////

// Message area >= 0x4
#define OPERATION_LOG_INFO_MESSAGE							0x1
#define OPERATION_LOG_WARNING_MESSAGE						0x2
#define OPERATION_LOG_ERROR_MESSAGE							0x3
#define OPERATION_LOG_NON_IMMEDIATE_MESSAGE					0x4
