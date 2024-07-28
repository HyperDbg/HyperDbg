/**
 * @file hyperdbg-app.cpp
 * @author Sina Karvandi (sina@hyperdbg.org)
 * @brief Controller of the reversing machine's module
 * @details
 *
 * @version 0.2
 * @date 2023-02-01
 *
 * @copyright This project is released under the GNU Public License v3.
 *
 */
#include "pch.h"

PVOID g_SharedMessageBuffer = NULL;

/**
 * @brief Show messages
 *
 * @param Text
 * @return int
 */
int
hyperdbg_show_messages(const char * Text)
{
    printf("%s", Text);
    return 0;
}

/**
 * @brief Show messages (shared buffer)
 *
 * @param Text
 * @return int
 */
int
hyperdbg_show_messages_shared_buffer()
{
    printf("%s", (char *)g_SharedMessageBuffer);
    return 0;
}

/**
 * @brief Load the driver
 *
 * @return int return zero if it was successful or non-zero if there
 * was error
 */
int
hyperdbg_load()
{
    char CpuId[13] = {0};

    //
    // Read the vendor string
    //
    hyperdbg_u_read_vendor_string(CpuId);

    printf("current processor vendor is : %s\n", CpuId);

    if (strcmp(CpuId, "GenuineIntel") == 0)
    {
        printf("virtualization technology is vt-x\n");
    }
    else
    {
        printf("this program is not designed to run in a non-VT-x "
               "environment !\n");
        return 1;
    }

    //
    // Detect if the processor supports vmx operation
    //
    if (hyperdbg_u_detect_vmx_support())
    {
        printf("vmx operation is supported by your processor\n");
    }
    else
    {
        printf("vmx operation is not supported by your processor\n");
        return 1;
    }

    //
    // Set callback function for showing messages
    //
    hyperdbg_u_set_text_message_callback(hyperdbg_show_messages);

    //
    // Test intepreter with shared buffer
    //
    // g_SharedMessageBuffer = hyperdbg_u_set_text_message_callback_using_shared_buffer(hyperdbg_show_messages_shared_buffer);

    //
    // Test interpreter
    //
    hyperdbg_u_connect_remote_debugger_using_named_pipe("\\\\.\\pipe\\HyperDbgPipe", TRUE);
    Sleep(10000);
    hyperdbg_u_interpreter((CHAR *)"r");
    hyperdbg_u_interpreter((CHAR *)".start path c:\\Windows\\system32\\calc.exe");
    hyperdbg_u_continue_debuggee();
    hyperdbg_u_continue_debuggee();
    hyperdbg_u_continue_debuggee();
    hyperdbg_u_continue_debuggee();
    hyperdbg_u_continue_debuggee();
    hyperdbg_u_continue_debuggee();
    hyperdbg_u_continue_debuggee();

    return 0;
}

/**
 * @brief main function
 *
 * @return int
 */
int
main()
{
    if (hyperdbg_load() == 0)
    {
        //
        // HyperDbg driver loaded successfully
        //
        // hyperdbg_unload();
    }
    else
    {
        printf("err, in loading HyperDbg\n");
    }
}
