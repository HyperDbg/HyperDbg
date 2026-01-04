#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include "module_info.h"
#include "../../include/platform/kernel/header/Mem.h"


static void* g_AllocatedBuffer;
static size_t g_BufferSize = 4096;

static int  mock_init(void)
{
    char source_data[] = "This is a test string from Linux kernel module";

    printk(KERN_INFO "Mock module loading\n");

    g_AllocatedBuffer = MemAllocKernel(g_BufferSize);
    if (!g_AllocatedBuffer)
        return -ENOMEM;

    MemCopy(g_AllocatedBuffer, source_data, sizeof(source_data));
    printk(KERN_INFO "Copied data: %s\n", (char*)g_AllocatedBuffer);

    MemSet(g_AllocatedBuffer, 0, g_BufferSize);
    printk(KERN_INFO "Buffer zeroed\n");

    return 0;
}

static void  mock_exit(void)
{
    printk(KERN_INFO "Mock module unloading\n");
    MemFree(g_AllocatedBuffer);
}

module_init(mock_init);
module_exit(mock_exit);

