/**
 * @file platform-serial.c
 * @author Max Raulea (max.raulea@hyperdbg.org)
 * @brief User mode cross-platform implementation of the kernel-debugger serial transport
 * @details See platform-serial.h. The Windows branch wraps the Win32 serial primitives
 *          (CreateFile / Comm* / overlapped ReadFile/WriteFile) and owns the per-direction
 *          OVERLAPPED state internally so the protocol layer never sees it. The Linux
 *          branch uses blocking file descriptors: termios for tty devices, and a
 *          UNIX domain socket for VMware's serial-port socket on a Linux host.
 *
 * @version 0.20
 * @date 2026-06-08
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

#if defined(__linux__)
#    include "../header/platform-serial.h"
#endif // defined(__linux__)

#if defined(_WIN32)

//
// Not implemented here
//

#elif defined(__linux__)

#    include <errno.h>
#    include <fcntl.h>
#    include <stdint.h>
#    include <string.h>
#    include <termios.h>
#    include <unistd.h>
#    include <sys/socket.h>
#    include <sys/stat.h>
#    include <sys/un.h>

//
// The transport handle wraps a file descriptor. fd 0 is a valid descriptor but
// a NULL HANDLE means failure, so the handle stores fd + 1
//
#    define SERIAL_FD_TO_HANDLE(Fd) ((HANDLE)(intptr_t)((Fd) + 1))
#    define SERIAL_HANDLE_TO_FD(H)  ((int)(intptr_t)(H) - 1)

/**
 * @brief Open the serial transport
 * @details PortName is either a tty device (/dev/ttyS*, /dev/ttyUSB*, a pty)
 *          or a UNIX domain socket, which is how VMware exposes a VM's serial
 *          port ("named pipe") on a Linux host
 *
 * @param PortName path of the device or socket
 * @param Role debugger or debuggee side (both use blocking I/O on Linux)
 *
 * @return HANDLE transport handle, or NULL on failure (errno is set)
 */
HANDLE
PlatformSerialOpen(const char * PortName, PLATFORM_SERIAL_IO_ROLE Role)
{
    struct stat St;
    int         Fd;

    (void)Role;

    if (stat(PortName, &St) != 0)
        return NULL;

    if (S_ISSOCK(St.st_mode))
    {
        struct sockaddr_un Addr = {0};

        if (strlen(PortName) >= sizeof(Addr.sun_path))
        {
            errno = ENAMETOOLONG;
            return NULL;
        }

        Fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (Fd < 0)
            return NULL;

        Addr.sun_family = AF_UNIX;
        strcpy(Addr.sun_path, PortName);

        if (connect(Fd, (struct sockaddr *)&Addr, sizeof(Addr)) != 0)
        {
            int SavedErrno = errno;
            close(Fd);
            errno = SavedErrno;
            return NULL;
        }
    }
    else
    {
        Fd = open(PortName, O_RDWR | O_NOCTTY | O_CLOEXEC);
        if (Fd < 0)
            return NULL;
    }

    return SERIAL_FD_TO_HANDLE(Fd);
}

/**
 * @brief Convert a baud rate to its termios speed constant
 *
 * @param BaudRate
 *
 * @return speed_t the speed, or 0 if termios has no constant for it
 */
static speed_t
PlatformSerialBaudToSpeed(DWORD BaudRate)
{
    switch (BaudRate)
    {
    case 110:
        return B110;
    case 300:
        return B300;
    case 600:
        return B600;
    case 1200:
        return B1200;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
    case 57600:
        return B57600;
    case 115200:
        return B115200;
    default:
        return 0;
    }
}

/**
 * @brief Configure baud rate and raw 8-N-1 framing
 * @details A socket has no line settings, so non-tty handles are accepted as-is
 *
 * @param Handle
 * @param BaudRate
 *
 * @return BOOLEAN
 */
BOOLEAN
PlatformSerialConfigure(HANDLE Handle, DWORD BaudRate)
{
    int            Fd = SERIAL_HANDLE_TO_FD(Handle);
    struct termios Tio;
    speed_t        Speed;

    if (!isatty(Fd))
        return TRUE;

    Speed = PlatformSerialBaudToSpeed(BaudRate);
    if (Speed == 0)
    {
        errno = EINVAL;
        return FALSE;
    }

    if (tcgetattr(Fd, &Tio) != 0)
        return FALSE;

    cfmakeraw(&Tio); // 8 data bits, no parity, no echo, no line processing
    Tio.c_cflag &= ~(CSTOPB | CRTSCTS);
    Tio.c_cflag |= CLOCAL | CREAD;
    Tio.c_cc[VMIN]  = 1;
    Tio.c_cc[VTIME] = 0;

    if (cfsetispeed(&Tio, Speed) != 0 || cfsetospeed(&Tio, Speed) != 0)
        return FALSE;

    if (tcsetattr(Fd, TCSANOW, &Tio) != 0)
        return FALSE;

    tcflush(Fd, TCIOFLUSH);

    return TRUE;
}

/**
 * @brief Read a single byte (blocking)
 *
 * @param Handle
 * @param OutByte
 * @param BytesRead
 * @param Role
 *
 * @return BOOLEAN TRUE if a byte was read
 */
BOOLEAN
PlatformSerialReadByte(HANDLE                  Handle,
                       CHAR *                  OutByte,
                       DWORD *                 BytesRead,
                       PLATFORM_SERIAL_IO_ROLE Role)
{
    ssize_t Ret;

    (void)Role;

    do
    {
        Ret = read(SERIAL_HANDLE_TO_FD(Handle), OutByte, 1);
    } while (Ret < 0 && errno == EINTR);

    if (BytesRead)
        *BytesRead = Ret > 0 ? (DWORD)Ret : 0;

    return Ret == 1;
}

/**
 * @brief Write a buffer (blocking, until everything is sent)
 *
 * @param Handle
 * @param Buffer
 * @param Length
 * @param Synchronous unused, all writes are blocking on Linux
 *
 * @return BOOLEAN
 */
BOOLEAN
PlatformSerialWrite(HANDLE Handle, const void * Buffer, UINT32 Length, BOOLEAN Synchronous)
{
    const char * Ptr = (const char *)Buffer;
    ssize_t      Ret;

    (void)Synchronous;

    while (Length > 0)
    {
        Ret = write(SERIAL_HANDLE_TO_FD(Handle), Ptr, Length);

        if (Ret < 0)
        {
            if (errno == EINTR)
                continue;

            return FALSE;
        }

        Ptr += Ret;
        Length -= (UINT32)Ret;
    }

    return TRUE;
}

/**
 * @brief Close the transport handle
 *
 * @param Handle
 *
 * @return BOOLEAN
 */
BOOLEAN
PlatformSerialClose(HANDLE Handle)
{
    if (Handle == NULL)
        return FALSE;

    return close(SERIAL_HANDLE_TO_FD(Handle)) == 0;
}

#else
#    error "Unsupported platform"
#endif
