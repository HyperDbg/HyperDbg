// hprdbgctrl.cpp : Defines the exported functions for the DLL.
//
#include "pch.h"
#include <Windows.h>
#include <winioctl.h>
#include <conio.h>
#include <iostream>  
#include "Definitions.h"
#include "Configuration.h"
#include "framework.h"
#include "hprdbgctrl.h"

// Global Variables
using namespace std;
HANDLE Handle;
BOOLEAN IsVmxOffProcessStart; // Show whether the vmxoff process start or not
Callback Handler = 0;


// Exports
extern "C"
{
	__declspec (dllexport) int __cdecl  HyperdbgLoad();
	__declspec (dllexport) int __cdecl  HyperdbgUnload();
	__declspec (dllexport) void __stdcall HyperdbgSetTextMessageCallback(Callback handler);

}


void __stdcall HyperdbgSetTextMessageCallback(Callback handler) {
	Handler = handler;
}

void ShowMessages(const char* Fmt, ...) {

	va_list ArgList;
	char TempMessage[PacketChunkSize];

	va_start(ArgList, Fmt);

	int sprintfresult = vsprintf_s(TempMessage, PacketChunkSize - 1, Fmt, ArgList);
	va_end(ArgList);

	if (sprintfresult != -1)
	{
		if (Handler != NULL)
		{
			Handler(TempMessage);
		}
		else
		{
			printf(TempMessage);
			printf("\n");
		}
	}
	else
	{
		MessageBoxA(0, "Error occured in send date to managed code !", "error", 0);
	}


}



string GetCpuid()
{
	char SysType[13]; // Array consisting of 13 single bytes/characters
	string CpuID; // The string that will be used to add all the characters toStarting coding in assembly language 

	_asm
	{
		//Execute CPUID with EAX = 0 to get the CPU producer
		xor eax, eax
		cpuid

		//MOV EBX to EAX and get the characters one by one by using shift out right bitwise operation.
		mov eax, ebx
		mov SysType[0], al
		mov SysType[1], ah
		shr eax, 16
		mov SysType[2], al
		mov SysType[3], ah

		//Get the second part the same way but these values are stored in EDX
		mov eax, edx
		mov SysType[4], al
		mov SysType[5], ah
		shr EAX, 16
		mov SysType[6], al
		mov SysType[7], ah

		//Get the third part
		mov eax, ecx
		mov SysType[8], al
		mov SysType[9], ah
		SHR EAX, 16
		mov SysType[10], al
		mov SysType[11], ah
		mov SysType[12], 00
	}

	CpuID.assign(SysType, 12);

	return CpuID;
}


bool VmxSupportDetection()
{

	bool VMX;

	VMX = false;

	__asm {

		xor eax, eax
		inc    eax
		cpuid
		bt     ecx, 0x5
		jc     VMXSupport

		VMXNotSupport :
		jmp     NopInstr

			VMXSupport :
		mov    VMX, 0x1

			NopInstr :
			nop
	}

	return VMX;
}


#if !UseDbgPrintInsteadOfUsermodeMessageTracking 

void ReadIrpBasedBuffer(HANDLE  Device) {

	BOOL    Status;
	ULONG   ReturnedLength;
	REGISTER_EVENT RegisterEvent;
	UINT32 OperationCode;

	ShowMessages(" =============================== Kernel-Mode Logs (Driver) ===============================");
	RegisterEvent.hEvent = NULL;
	RegisterEvent.Type = IRP_BASED;
	// allocate buffer for transfering messages
	char* OutputBuffer = (char*)malloc(UsermodeBufferSize);

	try
	{

		while (TRUE) {
			if (!IsVmxOffProcessStart)
			{
				ZeroMemory(OutputBuffer, UsermodeBufferSize);

				Sleep(200);							// we're not trying to eat all of the CPU ;)

				Status = DeviceIoControl(
					Device,							// Handle to device
					IOCTL_REGISTER_EVENT,			// IO Control code
					&RegisterEvent,					// Input Buffer to driver.
					SIZEOF_REGISTER_EVENT * 2,		// Length of input buffer in bytes. (x 2 is bcuz as the driver is x64 and has 64 bit values)
					OutputBuffer,					// Output Buffer from driver.
					UsermodeBufferSize,				// Length of output buffer in bytes.
					&ReturnedLength,				// Bytes placed in buffer.
					NULL							// synchronous call
				);

				if (!Status) {
					ShowMessages("Ioctl failed with code %d", GetLastError());
					break;
				}
				ShowMessages("========================= Kernel Mode (Buffer) =========================");

				OperationCode = 0;
				memcpy(&OperationCode, OutputBuffer, sizeof(UINT32));

				ShowMessages("Returned Length : 0x%x ", ReturnedLength);
				ShowMessages("Operation Code : 0x%x ", OperationCode);

				switch (OperationCode)
				{
				case OPERATION_LOG_NON_IMMEDIATE_MESSAGE:
					ShowMessages("A buffer of messages (OPERATION_LOG_NON_IMMEDIATE_MESSAGE) :");
					ShowMessages("%s", OutputBuffer + sizeof(UINT32));
					break;
				case OPERATION_LOG_INFO_MESSAGE:
					ShowMessages("Information log (OPERATION_LOG_INFO_MESSAGE) :");
					ShowMessages("%s", OutputBuffer + sizeof(UINT32));
					break;
				case OPERATION_LOG_ERROR_MESSAGE:
					ShowMessages("Error log (OPERATION_LOG_ERROR_MESSAGE) :");
					ShowMessages("%s", OutputBuffer + sizeof(UINT32));
					break;
				case OPERATION_LOG_WARNING_MESSAGE:
					ShowMessages("Warning log (OPERATION_LOG_WARNING_MESSAGE) :");
					ShowMessages("%s", OutputBuffer + sizeof(UINT32));
					break;

				default:
					break;
				}


				ShowMessages("========================================================================");

			}
			else
			{
				// the thread should not work anymore
				return;
			}
		}
	}
	catch (const std::exception&)
	{
		ShowMessages(" Exception !");
	}
}

DWORD WINAPI ThreadFunc(void* Data) {
	// Do stuff.  This will be the first function called on the new thread.
	// When this function returns, the thread goes away.  See MSDN for more details.
	// Test Irp Based Notifications
	ReadIrpBasedBuffer(Data);

	return 0;
}
#endif



HPRDBGCTRL_API int HyperdbgLoad()
{

	string CpuID;
	DWORD ErrorNum;
	BOOL    Status;


	CpuID = GetCpuid();

	ShowMessages("The CPU Vendor is : %s", CpuID.c_str());

	if (CpuID == "GenuineIntel")
	{
		ShowMessages("The Processor virtualization technology is VT-x.");
	}
	else
	{
		ShowMessages("This program is not designed to run in a non-VT-x environemnt !");
		return 1;
	}

	if (VmxSupportDetection())
	{
		ShowMessages("VMX Operation is supported by your processor .");
	}
	else
	{
		ShowMessages("VMX Operation is not supported by your processor .");
		return 1;
	}

	Handle = CreateFileA("\\\\.\\HyperdbgHypervisor",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ |
		FILE_SHARE_WRITE,
		NULL, /// lpSecurityAttirbutes
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL |
		FILE_FLAG_OVERLAPPED,
		NULL); /// lpTemplateFile 

	if (Handle == INVALID_HANDLE_VALUE)
	{
		ErrorNum = GetLastError();
		ShowMessages("CreateFile failed : %d", ErrorNum);
		return 1;

	}

#if !UseDbgPrintInsteadOfUsermodeMessageTracking 

	HANDLE Thread = CreateThread(NULL, 0, ThreadFunc, Handle, 0, NULL);
	if (Thread) {
		ShowMessages("Thread Created successfully !!!");
	}
#endif

	return 0;
}

HPRDBGCTRL_API int HyperdbgUnload()
{
	BOOL    Status;

	if (!Handle)
	{
		ShowMessages("Handle not found, probably the driver is not initialized.");
		return 1;
	}

	ShowMessages("Terminating VMX !");


	// Send IOCTL to mark complete all IRP Pending 
	Status = DeviceIoControl(
		Handle,															// Handle to device
		IOCTL_TERMINATE_VMX,											// IO Control code
		NULL,															// Input Buffer to driver.
		0,																// Length of input buffer in bytes. (x 2 is bcuz as the driver is x64 and has 64 bit values)
		NULL,															// Output Buffer from driver.
		0,																// Length of output buffer in bytes.
		NULL,															// Bytes placed in buffer.
		NULL															// synchronous call
	);
	// wait to make sure we don't use an invalid handle in another Ioctl
	if (!Status) {
		ShowMessages("Ioctl failed with code %d", GetLastError());
	}


	// Send IOCTL to mark complete all IRP Pending 
	Status = DeviceIoControl(
		Handle,															// Handle to device
		IOCTL_RETURN_IRP_PENDING_PACKETS_AND_DISALLOW_IOCTL,			// IO Control code
		NULL,															// Input Buffer to driver.
		0,																// Length of input buffer in bytes. (x 2 is bcuz as the driver is x64 and has 64 bit values)
		NULL,															// Output Buffer from driver.
		0,																// Length of output buffer in bytes.
		NULL,															// Bytes placed in buffer.
		NULL															// synchronous call
	);

	// wait to make sure we don't use an invalid handle in another Ioctl
	if (!Status) {
		ShowMessages("Ioctl failed with code %d", GetLastError());
	}

	// Indicate that the finish process start or not
	IsVmxOffProcessStart = TRUE;

	Sleep(1000); // Wait so next thread can return from IRP Pending
	// Send IRP_MJ_CLOSE to driver to terminate Vmxs
	if (!CloseHandle(Handle))
	{
		ShowMessages("Error : 0x%x", GetLastError());
	};

	ShowMessages("You're not on hypervisor anymore !");

}