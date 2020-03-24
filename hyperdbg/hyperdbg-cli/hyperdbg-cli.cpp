#include <Windows.h>
#include <conio.h>
#include <iostream>  
#include "Definitions.h"
#include "Configuration.h"

using namespace std;
BOOLEAN IsVmxOffProcessStart; // Show whether the vmxoff process start or not

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

void PrintAppearance() {

	printf("\n"


		"    _   _                             _                  _____                      ____                 _       _     \n"
		"   | | | |_   _ _ __   ___ _ ____   _(_)___  ___  _ __  |  ___| __ ___  _ __ ___   / ___|  ___ _ __ __ _| |_ ___| |__  \n"
		"   | |_| | | | | '_ \\ / _ \\ '__\\ \\ / / / __|/ _ \\| '__| | |_ | '__/ _ \\| '_ ` _ \\  \\___ \\ / __| '__/ _` | __/ __| '_ \\ \n"
		"   |  _  | |_| | |_) |  __/ |   \\ V /| \\__ \\ (_) | |    |  _|| | | (_) | | | | | |  ___) | (__| | | (_| | || (__| | | |\n"
		"   |_| |_|\\__, | .__/ \\___|_|    \\_/ |_|___/\\___/|_|    |_|  |_|  \\___/|_| |_| |_| |____/ \\___|_|  \\__,_|\\__\\___|_| |_|\n"
		"          |___/|_|                                                                                                     \n"
		"\n\n");
}

#if !UseDbgPrintInsteadOfUsermodeMessageTracking 

void ReadIrpBasedBuffer(HANDLE  Device) {

	BOOL    Status;
	ULONG   ReturnedLength;
	REGISTER_EVENT RegisterEvent;
	UINT32 OperationCode;

	printf(" =============================== Kernel-Mode Logs (Driver) ===============================\n");
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
					printf("Ioctl failed with code %d\n", GetLastError());
					break;
				}
				printf("\n========================= Kernel Mode (Buffer) =========================\n");

				OperationCode = 0;
				memcpy(&OperationCode, OutputBuffer, sizeof(UINT32));

				printf("Returned Length : 0x%x \n", ReturnedLength);
				printf("Operation Code : 0x%x \n", OperationCode);

				switch (OperationCode)
				{
				case OPERATION_LOG_NON_IMMEDIATE_MESSAGE:
					printf("A buffer of messages (OPERATION_LOG_NON_IMMEDIATE_MESSAGE) :\n");
					printf("%s", OutputBuffer + sizeof(UINT32));
					break;
				case OPERATION_LOG_INFO_MESSAGE:
					printf("Information log (OPERATION_LOG_INFO_MESSAGE) :\n");
					printf("%s", OutputBuffer + sizeof(UINT32));
					break;
				case OPERATION_LOG_ERROR_MESSAGE:
					printf("Error log (OPERATION_LOG_ERROR_MESSAGE) :\n");
					printf("%s", OutputBuffer + sizeof(UINT32));
					break;
				case OPERATION_LOG_WARNING_MESSAGE:
					printf("Warning log (OPERATION_LOG_WARNING_MESSAGE) :\n");
					printf("%s", OutputBuffer + sizeof(UINT32));
					break;

				default:
					break;
				}


				printf("\n========================================================================\n");

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
		printf("\n Exception !\n");
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

int main()
{
	string CpuID;
	DWORD ErrorNum;
	HANDLE Handle;
	BOOL    Status;

	// Print Hypervisor From Scratch Message
	PrintAppearance();

	CpuID = GetCpuid();

	printf("[*] The CPU Vendor is : %s \n", CpuID.c_str());

	if (CpuID == "GenuineIntel")
	{
		printf("[*] The Processor virtualization technology is VT-x. \n");
	}
	else
	{
		printf("[*] This program is not designed to run in a non-VT-x environemnt !\n");
		return 1;
	}

	if (VmxSupportDetection())
	{
		printf("[*] VMX Operation is supported by your processor .\n");
	}
	else
	{
		printf("[*] VMX Operation is not supported by your processor .\n");
		return 1;
	}

	Handle = CreateFileA("\\\\.\\MyHypervisorDevice",
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
		printf("[*] CreateFile failed : %d\n", ErrorNum);
		return 1;

	}

#if !UseDbgPrintInsteadOfUsermodeMessageTracking 

	HANDLE Thread = CreateThread(NULL, 0, ThreadFunc, Handle, 0, NULL);
	if (Thread) {
		printf("[*] Thread Created successfully !!!");
	}
#endif

	printf("\n[*] Press any key to terminate the VMX operation...\n");

	_getch();

	printf("[*] Terminating VMX !\n");

	// Indicate that the finish process start or not
	IsVmxOffProcessStart = TRUE;

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
		printf("Ioctl failed with code %d\n", GetLastError());
	}
	// Send IRP_MJ_CLOSE to driver to terminate Vmxs
	CloseHandle(Handle);

	printf("\nError : 0x%x\n", GetLastError());

	printf("[*] You're not on hypervisor anymore !");

	exit(0);

	return 0;
}

