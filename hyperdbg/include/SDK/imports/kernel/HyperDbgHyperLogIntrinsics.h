/**
 * @file HyperDbgHyperLogIntrinsics.h
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Headers relating exported functions from hyperlog project
 * @version 0.1
 * @date 2023-01-15
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#pragma once

//////////////////////////////////////////////////
//					Enums						//
//////////////////////////////////////////////////

/**
 * @brief Types of log messages
 *
 */
typedef enum _LOG_TYPE
{
    LOG_INFO,
    LOG_WARNING,
    LOG_ERROR

} LOG_TYPE;

//////////////////////////////////////////////////
//					Logging						//
//////////////////////////////////////////////////

/**
 * @brief Define log variables
 *
 */
#if UseDbgPrintInsteadOfUsermodeMessageTracking
/* Use DbgPrint */
#    define Logformat, ...)                           \
        DbgPrint("[+] Information (%s:%d) | " format "\n", \
                 __func__,                                 \
                 __LINE__,                                 \
                 __VA_ARGS__)

#    define LogWarning(format, ...)                    \
        DbgPrint("[-] Warning (%s:%d) | " format "\n", \
                 __func__,                             \
                 __LINE__,                             \
                 __VA_ARGS__)

#    define LogError(format, ...)                    \
        DbgPrint("[!] Error (%s:%d) | " format "\n", \
                 __func__,                           \
                 __LINE__,                           \
                 __VA_ARGS__);                       \
        DbgBreakPoint()

/**
 * @brief Log without any prefix
 *
 */
#    define Log(format, ...) \
        DbgPrint(format, __VA_ARGS__)

#else

/**
 * @brief Log, general
 *
 */
#    define LogInfo(format, ...)                                                          \
        LogCallbackPrepareAndSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE,               \
                                                UseImmediateMessaging,                    \
                                                ShowSystemTimeOnDebugMessages,            \
                                                FALSE,                                    \
                                                "[+] Information (%s:%d) | " format "\n", \
                                                __func__,                                 \
                                                __LINE__,                                 \
                                                __VA_ARGS__)

/**
 * @brief Log in the case of priority message
 *
 */
#    define LogInfoPriority(format, ...)                                                  \
        LogCallbackPrepareAndSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE,               \
                                                TRUE,                                     \
                                                ShowSystemTimeOnDebugMessages,            \
                                                TRUE,                                     \
                                                "[+] Information (%s:%d) | " format "\n", \
                                                __func__,                                 \
                                                __LINE__,                                 \
                                                __VA_ARGS__)

/**
 * @brief Log in the case of warning
 *
 */
#    define LogWarning(format, ...)                                                   \
        LogCallbackPrepareAndSendMessageToQueue(OPERATION_LOG_WARNING_MESSAGE,        \
                                                UseImmediateMessaging,                \
                                                ShowSystemTimeOnDebugMessages,        \
                                                TRUE,                                 \
                                                "[-] Warning (%s:%d) | " format "\n", \
                                                __func__,                             \
                                                __LINE__,                             \
                                                __VA_ARGS__)

/**
 * @brief Log in the case of error
 *
 */
#    define LogError(format, ...)                                                   \
        LogCallbackPrepareAndSendMessageToQueue(OPERATION_LOG_ERROR_MESSAGE,        \
                                                UseImmediateMessaging,              \
                                                ShowSystemTimeOnDebugMessages,      \
                                                TRUE,                               \
                                                "[!] Error (%s:%d) | " format "\n", \
                                                __func__,                           \
                                                __LINE__,                           \
                                                __VA_ARGS__);                       \
        if (DebugMode)                                                              \
        DbgBreakPoint()

/**
 * @brief Log without any prefix
 *
 */
#    define Log(format, ...)                                                \
        LogCallbackPrepareAndSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE, \
                                                TRUE,                       \
                                                FALSE,                      \
                                                FALSE,                      \
                                                format,                     \
                                                __VA_ARGS__)

/**
 * @brief Log without any prefix and bypass the stack
 * problem (getting two temporary stacks in preparing phase)
 *
 */
#    define LogSimpleWithTag(tag, isimmdte, buffer, len) \
        LogCallbackSendMessageToQueue(tag,               \
                                      isimmdte,          \
                                      buffer,            \
                                      len,               \
                                      FALSE)

#endif // UseDbgPrintInsteadOfUsermodeMessageTracking

/**
 * @brief Log, initialize boot information and debug information
 *
 */
#define LogDebugInfo(format, ...)                                                     \
    if (DebugMode)                                                                    \
    LogCallbackPrepareAndSendMessageToQueue(OPERATION_LOG_INFO_MESSAGE,               \
                                            UseImmediateMessaging,                    \
                                            ShowSystemTimeOnDebugMessages,            \
                                            FALSE,                                    \
                                            "[+] Information (%s:%d) | " format "\n", \
                                            __func__,                                 \
                                            __LINE__,                                 \
                                            __VA_ARGS__)
