
// Configures whether to show the current system time in the output of debug messages or not
// (only available on usermode tracing messages)
#define ShowSystemTimeOnDebugMessages		TRUE

// Use WPP Tracing instead of all logging functions
#define UseWPPTracing		FALSE


// Configures whether to use DbgPrint or use the custom usermode message tracking 
#define UseDbgPrintInsteadOfUsermodeMessageTracking		FALSE

// Show debug messages in both usermode app and debugger, it works only if you set UseDbgPrintInsteadOfUsermodeMessageTracking to FALSE
#define ShowMessagesOnDebugger		TRUE

// Use immediate messaging (means that it sends each message when they recieved and do not accumulate them)
// it works only if you set UseDbgPrintInsteadOfUsermodeMessageTracking to FALSE
#define UseImmediateMessaging		FALSE