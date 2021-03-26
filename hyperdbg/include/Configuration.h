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
#pragma once

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
 * @details Should be FALSE, I realized that if we enable this flag, we end up
 * in a situation that DbgPrint halts the system because it is executing in
 * Dispatch-level in a DPC routine, I left it to FALSE for future attention
 */
#define ShowMessagesOnDebugger FALSE

/**
 * @brief Use immediate messaging (means that it sends each message when they
 * recieved and do not accumulate them) it works only if you set
 * UseDbgPrintInsteadOfUsermodeMessageTracking to FALSE
 */
#define UseImmediateMessaging TRUE

/**
 * @brief Use immediate messaging (means that it sends each message when they
 * recieved and do not accumulate them) its the default value on events,
 * a user can change this behavior by selecting 'imm yes' or 'imm no' in the
 * case of events
 */
#define UseImmediateMessagingByDefaultOnEvents TRUE

/**
 * @brief Shows whether to show or not show the drivers debugging infomation
 * and also enters debugger in debugging section to break the debugger in the
 * case of errors
 */
#define DebugMode FALSE
