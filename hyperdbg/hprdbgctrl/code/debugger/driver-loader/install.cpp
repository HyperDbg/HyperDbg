/**
 * @file Install.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Install functions
 * @details
 * @version 0.1
 * @date 2020-04-11
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

BOOLEAN
InstallDriver(SC_HANDLE SchSCManager, LPCTSTR DriverName, LPCTSTR ServiceExe);

BOOLEAN
RemoveDriver(SC_HANDLE SchSCManager, LPCTSTR DriverName);

BOOLEAN
StartDriver(SC_HANDLE SchSCManager, LPCTSTR DriverName);

BOOLEAN
StopDriver(SC_HANDLE SchSCManager, LPCTSTR DriverName);

/**
 * @brief Install driver
 *
 * @param SC_HANDLE
 * @param LPCTSTR
 * @param LPCTSTR
 * @return BOOLEAN
 */
BOOLEAN
InstallDriver(SC_HANDLE SchSCManager, LPCTSTR DriverName, LPCTSTR ServiceExe)
{
    SC_HANDLE SchService;
    DWORD     LastError;

    //
    // NOTE: This creates an entry for a standalone driver. If this
    //       is modified for use with a driver that requires a Tag,
    //       Group, and/or Dependencies, it may be necessary to
    //       query the registry for existing driver information
    //       (in order to determine a unique Tag, etc.).
    //

    //
    // Create a new a service object.
    //
    SchService =
        CreateService(SchSCManager,          // handle of service control manager database
                      DriverName,            // address of name of service to start
                      DriverName,            // address of display name
                      SERVICE_ALL_ACCESS,    // type of access to service
                      SERVICE_KERNEL_DRIVER, // type of service
                      SERVICE_DEMAND_START,  // when to start service
                      SERVICE_ERROR_NORMAL,  // severity if service fails to start
                      ServiceExe,            // address of name of binary file
                      NULL,                  // service does not belong to a group
                      NULL,                  // no tag requested
                      NULL,                  // no dependency names
                      NULL,                  // use LocalSystem account
                      NULL                   // no password for service account
        );

    if (SchService == NULL)
    {
        LastError = GetLastError();

        if (LastError == ERROR_SERVICE_EXISTS)
        {
            //
            // Ignore this error.
            //
            return TRUE;
        }
        else if (LastError == ERROR_SERVICE_MARKED_FOR_DELETE)
        {
            //
            // Previous instance of the service is not fully deleted so sleep
            // and try again.
            //
            ShowMessages("err, previous instance of the service is not fully deleted. Try "
                         "again...\n");
            return FALSE;
        }
        else
        {
            ShowMessages("err, CreateService failed (%x)\n", LastError);

            //
            // Indicate an error.
            //
            return FALSE;
        }
    }

    //
    // Close the service object.
    //
    if (SchService)
    {
        CloseServiceHandle(SchService);
    }

    //
    // Indicate success.
    //
    return TRUE;
}

/**
 * @brief Manage Driver
 *
 * @param DriverName
 * @param ServiceName
 * @param Function
 * @return BOOLEAN
 */
BOOLEAN
ManageDriver(LPCTSTR DriverName, LPCTSTR ServiceName, UINT16 Function)
{
    SC_HANDLE schSCManager;
    BOOLEAN   Res = TRUE;

    //
    // Insure (somewhat) that the driver and service names are valid.
    //
    if (!DriverName || !ServiceName)
    {
        ShowMessages("invalid Driver or Service provided to ManageDriver() \n");

        return FALSE;
    }

    //
    // Connect to the Service Control Manager and open the Services database.
    //
    schSCManager = OpenSCManager(NULL,                 // local machine
                                 NULL,                 // local database
                                 SC_MANAGER_ALL_ACCESS // access required
    );

    if (!schSCManager)
    {
        ShowMessages("err, OpenSCManager failed (%x)\n", GetLastError());

        return FALSE;
    }

    //
    // Do the requested function.
    //
    switch (Function)
    {
    case DRIVER_FUNC_INSTALL:

        //
        // Install the driver service.
        //

        if (InstallDriver(schSCManager, DriverName, ServiceName))
        {
            //
            // Start the driver service (i.e. start the driver).
            //
            Res = StartDriver(schSCManager, DriverName);
        }
        else
        {
            //
            // Indicate an error.
            //
            Res = FALSE;
        }

        break;

    case DRIVER_FUNC_STOP:

        //
        // Stop the driver.
        //
        Res = StopDriver(schSCManager, DriverName);

        break;

    case DRIVER_FUNC_REMOVE:

        //
        // Remove the driver service.
        //
        Res = RemoveDriver(schSCManager, DriverName);

        break;

    default:

        ShowMessages("unknown ManageDriver() function. \n");

        Res = FALSE;

        break;
    }

    //
    // Close handle to service control manager.
    //
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
    }

    return Res;
}

/**
 * @brief Remove Driver
 *
 * @param SC_HANDLE
 * @param LPCTSTR
 * @return BOOLEAN
 */
BOOLEAN
RemoveDriver(SC_HANDLE SchSCManager, LPCTSTR DriverName)
{
    SC_HANDLE SchService;
    BOOLEAN   Res;

    //
    // Open the handle to the existing service
    //
    SchService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);

    if (SchService == NULL)
    {
        ShowMessages("err, OpenService failed (%x)\n", GetLastError());

        //
        // Indicate error
        //
        return FALSE;
    }

    //
    // Mark the service for deletion from the service control manager database
    //
    if (DeleteService(SchService))
    {
        //
        // Indicate success
        //
        Res = TRUE;
    }
    else
    {
        ShowMessages("err, DeleteService failed (%x)\n", GetLastError());

        //
        // Indicate failure.  Fall through to properly close the service handle
        //
        Res = FALSE;
    }

    //
    // Close the service object
    //
    if (SchService)
    {
        CloseServiceHandle(SchService);
    }

    return Res;
}

/**
 * @brief Start Driver
 *
 * @param SC_HANDLE
 * @param LPCTSTR
 * @return BOOLEAN
 */
BOOLEAN
StartDriver(SC_HANDLE SchSCManager, LPCTSTR DriverName)
{
    SC_HANDLE      SchService;
    DWORD          LastError;
    SERVICE_STATUS serviceStatus;
    UINT64         Status = TRUE;

    //
    // Open the handle to the existing service.
    //
    SchService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);

    if (SchService == NULL)
    {
        ShowMessages("err, OpenService failed (%x)\n", GetLastError());

        //
        // Indicate failure
        //
        return FALSE;
    }

    //
    // Start the execution of the service (i.e. start the driver).
    //
    if (!StartService(SchService, // service identifier
                      0,          // number of arguments
                      NULL        // pointer to arguments
                      ))
    {
        LastError = GetLastError();

        if (LastError == ERROR_SERVICE_ALREADY_RUNNING)
        {
            //
            // Ignore this error
            //
        }
        else if (LastError == ERROR_PATH_NOT_FOUND)
        {
            //
            // Driver not found, or anti-virus limits the access to it
            //
            ShowMessages("err, path to the driver not found, or the access to the driver file is limited\n");

            ShowMessages("most of the time, it's because anti-virus software is not finished scanning the drivers, so, if you try to load the driver again (re-enter the previous command), the problem will be solved\n");

            //
            // Indicate failure
            //
            Status = FALSE;
        }
        else if (LastError == ERROR_INVALID_IMAGE_HASH)
        {
            ShowMessages(
                "err, failed loading driver\n"
                "it's because either the driver signature enforcement is enabled or HVCI prevents the driver from loading\n"
                "you should disable the driver signature enforcement by attaching WinDbg or from the boot menu\n"
                "if the driver signature enforcement is disabled, HVCI might prevent the driver from loading\n"
                "HyperDbg is not compatible with Virtualization Based Security (VBS)\n"
                "please follow the instructions from: https://docs.hyperdbg.org/getting-started/build-and-install \n");

            //
            // Indicate failure.  Fall through to properly close the service handle
            //
            Status = FALSE;
        }
        else
        {
            ShowMessages("err, StartService failure (%x)\n", LastError);

            //
            // Indicate failure.  Fall through to properly close the service handle
            //
            Status = FALSE;
        }
    }

    //
    // Close the service object
    //
    if (SchService)
    {
        CloseServiceHandle(SchService);
    }

    return Status;
}

/**
 * @brief Stop driver
 *
 * @param SC_HANDLE
 * @param LPCTSTR
 * @return BOOLEAN
 */
BOOLEAN
StopDriver(SC_HANDLE SchSCManager, LPCTSTR DriverName)
{
    BOOLEAN        Res = TRUE;
    SC_HANDLE      SchService;
    SERVICE_STATUS serviceStatus;

    //
    // Open the handle to the existing service.
    //
    SchService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);

    if (SchService == NULL)
    {
        ShowMessages("err, OpenService failed (%x)\n", GetLastError());

        return FALSE;
    }

    //
    // Request that the service stop.
    //
    if (ControlService(SchService, SERVICE_CONTROL_STOP, &serviceStatus))
    {
        //
        // Indicate success.
        //
        Res = TRUE;
    }
    else
    {
        ShowMessages("err, ControlService failed (%x)\n", GetLastError());

        //
        // Indicate failure.  Fall through to properly close the service handle.
        //
        Res = FALSE;
    }

    //
    // Close the service object.
    //
    if (SchService)
    {
        CloseServiceHandle(SchService);
    }

    return Res;
}

/**
 * @brief Setup driver name
 *
 * @param ULONG
 * @return BOOLEAN
 */
BOOLEAN
SetupDriverName(_Inout_updates_bytes_all_(BufferLength) PCHAR DriverLocation,
                ULONG                                         BufferLength)
{
    HANDLE  FileHandle;
    DWORD   DriverLocLen = 0;
    HMODULE ProcHandle   = GetModuleHandle(NULL);
    char *  Pos;

    //
    // Get the current directory.
    //

    /*
  //
  // We use the location of running exe instead of
  // finding driver based on current directory
  //
  DriverLocLen = GetCurrentDirectory(BufferLength, DriverLocation);

  if (DriverLocLen == 0) {

    ShowMessages("err, GetCurrentDirectory failed (%x)\n", GetLastError());

    return FALSE;
  }
  */

    GetModuleFileName(ProcHandle, DriverLocation, BufferLength);

    Pos = strrchr(DriverLocation, '\\');
    if (Pos != NULL)
    {
        //
        // this will put the null terminator here. you can also copy to
        // another string if you want, we can also use PathCchRemoveFileSpec
        //
        *Pos = '\0';
    }

    //
    // Setup path name to driver file.
    //
    if (FAILED(
            StringCbCat(DriverLocation, BufferLength, "\\" VMM_DRIVER_NAME ".sys")))
    {
        return FALSE;
    }

    //
    // Insure driver file is in the specified directory.
    //
    if ((FileHandle = CreateFile(DriverLocation, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) ==
        INVALID_HANDLE_VALUE)
    {
        ShowMessages("%s.sys is not loaded.\n", VMM_DRIVER_NAME);

        //
        // Indicate failure.
        //
        return FALSE;
    }

    //
    // Close open file handle.
    //
    if (FileHandle)
    {
        CloseHandle(FileHandle);
    }

    //
    // Indicate success.
    //
    return TRUE;
}
