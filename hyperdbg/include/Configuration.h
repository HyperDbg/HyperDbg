/**
 * @file Configuration.h
 * @author Sina Karvandi (sina@rayanfam.com)
 * @brief Global configuration that applies on compile time
 * @details you can disable or enable the following features and compile the
 * project Next time you used the project binary files these settings applied.
 * @version 0.1
 * @date 2020-04-10
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */

/**
 * @brief Configures whether to show the current system time in the output of
 * debug messages or not (only available on usermode tracing messages)
 *
 */
#define ShowSystemTimeOnDebugMessages TRUE

/**
 * @brief Use WPP Tracing instead of all logging functions
 *
 */
#define UseWPPTracing FALSE

/**
 * @brief Configures whether to use DbgPrint or use the custom usermode message
 * tracking
 *
 */
#define UseDbgPrintInsteadOfUsermodeMessageTracking FALSE

/**
 * @brief Show debug messages in both usermode app and debugger,
 * it works only if you set UseDbgPrintInsteadOfUsermodeMessageTracking to FALSE
 *
 */
#define ShowMessagesOnDebugger TRUE

/**
 * @brief Use immediate messaging (means that it sends each message when they
 * recieved and do not accumulate them) it works only if you set
 * UseDbgPrintInsteadOfUsermodeMessageTracking to FALSE
 */
#define UseImmediateMessaging FALSE
