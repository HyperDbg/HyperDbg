/**
 * @file Install.cpp
 * @author Sina Karvandi (sina@rayanfam.com)
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
    SC_HANDLE schService;
    DWORD     err;

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
    schService =
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

    if (schService == NULL)
    {
        err = GetLastError();

        if (err == ERROR_SERVICE_EXISTS)
        {
            //
            // Ignore this error.
            //
            return TRUE;
        }
        else if (err == ERROR_SERVICE_MARKED_FOR_DELETE)
        {
            //
            // Previous instance of the service is not fully deleted so sleep
            // and try again.
            //
            ShowMessages("Previous instance of the service is not fully deleted. Try "
                         "again...\n");
            return FALSE;
        }
        else
        {
            ShowMessages("CreateService failed!  Error = %d \n", err);

            //
            // Indicate an error.
            //
            return FALSE;
        }
    }

    //
    // Close the service object.
    //
    if (schService)
    {
        CloseServiceHandle(schService);
    }

    //
    // Indicate success.
    //
    return TRUE;
}

/**
 * @brief Manage Driver
 *
 * @param LPCTSTR
 * @param LPCTSTR
 * @param USHORT
 * @return BOOLEAN
 */
BOOLEAN
ManageDriver(LPCTSTR DriverName, LPCTSTR ServiceName, USHORT Function)
{
    SC_HANDLE schSCManager;
    BOOLEAN   rCode = TRUE;

    //
    // Insure (somewhat) that the driver and service names are valid.
    //
    if (!DriverName || !ServiceName)
    {
        ShowMessages("Invalid Driver or Service provided to ManageDriver() \n");

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
        ShowMessages("Open SC Manager failed! Error = %d \n", GetLastError());

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
            rCode = StartDriver(schSCManager, DriverName);
        }
        else
        {
            //
            // Indicate an error.
            //
            rCode = FALSE;
        }

        break;

    case DRIVER_FUNC_STOP:

        //
        // Stop the driver.
        //
        StopDriver(schSCManager, DriverName);

        //
        // Ignore all errors.
        //
        rCode = TRUE;

        break;

    case DRIVER_FUNC_REMOVE:

        //
        // Remove the driver service.
        //
        RemoveDriver(schSCManager, DriverName);

        //
        // Ignore all errors.
        //
        rCode = TRUE;

        break;

    default:

        ShowMessages("Unknown ManageDriver() function. \n");

        rCode = FALSE;

        break;
    }

    //
    // Close handle to service control manager.
    //
    if (schSCManager)
    {
        CloseServiceHandle(schSCManager);
    }

    return rCode;
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
    SC_HANDLE schService;
    BOOLEAN   rCode;

    //
    // Open the handle to the existing service
    //
    schService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);

    if (schService == NULL)
    {
        ShowMessages("OpenService failed!  Error = %d \n", GetLastError());

        //
        // Indicate error
        //
        return FALSE;
    }

    //
    // Mark the service for deletion from the service control manager database
    //
    if (DeleteService(schService))
    {
        //
        // Indicate success
        //
        rCode = TRUE;
    }
    else
    {
        ShowMessages("DeleteService failed!  Error = %d \n", GetLastError());

        //
        // Indicate failure.  Fall through to properly close the service handle
        //
        rCode = FALSE;
    }

    //
    // Close the service object
    //
    if (schService)
    {
        CloseServiceHandle(schService);
    }

    return rCode;
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
    SC_HANDLE      schService;
    DWORD          err;
    SERVICE_STATUS serviceStatus;
    UINT64         Status = TRUE;

    //
    // Open the handle to the existing service.
    //
    schService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);

    if (schService == NULL)
    {
        ShowMessages("OpenService failed!  Error = %d \n", GetLastError());

        //
        // Indicate failure
        //
        return FALSE;
    }

    //
    // Start the execution of the service (i.e. start the driver).
    //
    if (!StartService(schService, // service identifier
                      0,          // number of arguments
                      NULL        // pointer to arguments
                      ))
    {
        err = GetLastError();

        if (err == ERROR_SERVICE_ALREADY_RUNNING)
        {
            //
            // Ignore this error
            //
        }
        else if (err == 577)
        {
            ShowMessages(
                "Err 577, it's because you driver signature enforcement is enabled. "
                "You should disable driver signature enforcement by attaching Windbg "
                "or from the boot menu\n");

            //
            // Indicate failure.  Fall through to properly close the service handle
            //
            Status = FALSE;
        }
        else
        {
            ShowMessages("StartService failure! Error = %d \n", err);

            //
            // Indicate failure.  Fall through to properly close the service handle
            //
            Status = FALSE;
        }
    }

    //
    // Close the service object
    //
    if (schService)
    {
        CloseServiceHandle(schService);
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
    BOOLEAN        rCode = TRUE;
    SC_HANDLE      schService;
    SERVICE_STATUS serviceStatus;

    //
    // Open the handle to the existing service.
    //
    schService = OpenService(SchSCManager, DriverName, SERVICE_ALL_ACCESS);

    if (schService == NULL)
    {
        ShowMessages("OpenService failed!  Error = %d \n", GetLastError());

        return FALSE;
    }

    //
    // Request that the service stop.
    //
    if (ControlService(schService, SERVICE_CONTROL_STOP, &serviceStatus))
    {
        //
        // Indicate success.
        //
        rCode = TRUE;
    }
    else
    {
        ShowMessages("ControlService failed!  Error = %d \n", GetLastError());

        //
        // Indicate failure.  Fall through to properly close the service handle.
        //
        rCode = FALSE;
    }

    //
    // Close the service object.
    //
    if (schService)
    {
        CloseServiceHandle(schService);
    }

    return rCode;
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
    HANDLE  fileHandle;
    DWORD   driverLocLen = 0;
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
  driverLocLen = GetCurrentDirectory(BufferLength, DriverLocation);

  if (driverLocLen == 0) {

    ShowMessages("GetCurrentDirectory failed!  Error = %d \n", GetLastError());

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
            StringCbCat(DriverLocation, BufferLength, "\\" DRIVER_NAME ".sys")))
    {
        return FALSE;
    }

    //
    // Insure driver file is in the specified directory.
    //
    if ((fileHandle = CreateFile(DriverLocation, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) ==
        INVALID_HANDLE_VALUE)
    {
        ShowMessages("%s.sys is not loaded.\n", DRIVER_NAME);

        //
        // Indicate failure.
        //
        return FALSE;
    }

    //
    // Close open file handle.
    //
    if (fileHandle)
    {
        CloseHandle(fileHandle);
    }

    //
    // Indicate success.
    //
    return TRUE;
}
