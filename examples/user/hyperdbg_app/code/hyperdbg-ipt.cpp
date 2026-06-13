#include "pch.h"

static int
ShowMessages(const char * Text)
{
	printf("%s", Text);
	return 0;
}

static int
LoadVmm()
{
    hyperdbg_u_set_text_message_callback((PVOID)ShowMessages);

    if (!hyperdbg_u_detect_vmx_support())
    {
        printf("[-] VT-x (VMX) is not supported / enabled on this processor\n");
        return 1;
    }

    printf("[*] loading HyperDbg VMM...\n");
    if (hyperdbg_u_install_kd_driver() == 1 || hyperdbg_u_load_vmm() == 1)
    {
        printf("[-] cannot load the HyperDbg VMM\n");
        return 1;
    }

    printf("[+] HyperDbg VMM is running\n");

    return 0;
}

int
main(int argc, char ** argv)
{
    if (LoadVmm() != 0)
    {
        return 1;
    }

    hyperdbg_u_run_command((CHAR*)"lm");

    printf("[*] unloading HyperDbg VMM...\n");

    //
    // Unload the driver
    //
    hyperdbg_u_unload_vmm();
    hyperdbg_u_unload_kd();
    hyperdbg_u_stop_kd_driver();
    hyperdbg_u_uninstall_kd_driver();

    printf("[+] done\n");

    return 0;
}
