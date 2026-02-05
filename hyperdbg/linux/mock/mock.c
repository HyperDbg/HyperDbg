/**
 * @file mock_module.c
 * @author Alireza moradi (alish014)
 * @brief Mock Linux kernel module for testing cross-platform memory APIs.
 * @details This file demonstrates the usage of the unified Platform* memory
 *          API (Allocate, Write, Set, Free) within a Linux kernel module environment.
 * @version 0.1
 * @date 2026-02-05
 *
 * @copyright This project is released under the GNU Public License v3.
 */

#include <linux/module.h>
#include <linux/init.h>
#include "../header/platform_mem.h"

/**
 * @brief Global pointer to the allocated test buffer.
 * @details Stores the address returned by PlatformAllocateMemory so it can be
 *          freed when the module unloads.
 */
static PLAT_PTR g_AllocatedBuffer = NULL;

/**
 * @brief Size of the buffer to allocate during tests.
 * @note Default size is 4096 bytes (1 page).
 */
static PLAT_SIZE g_BufferSize = 4096;

/**
 * @brief Module initialization entry point.
 *
 * @details Performs the following test sequence:
 *          1. Allocates memory using the cross-platform wrapper.
 *          2. Copies a test string into the allocated memory.
 *          3. Prints the content to the kernel log.
 *          4. Zeroes out the memory contents.
 *
 * @return int
 *         - 0: Initialization successful.
 *         - -ENOMEM: Memory allocation failed.
 */
static int __init mock_init(void)
{
    char source_data[] = "This is a test string from Linux kernel module";

    printk(KERN_INFO "Mock module loading\n");

    // 1. Allocation
    g_AllocatedBuffer = PlatformAllocateMemory(g_BufferSize);
    if (!g_AllocatedBuffer)
        return -ENOMEM;

    // 2. Copying (Write Memory)
    // We pass NULL for 'Process' as the current implementation targets local kernel memory.
    PlatformWriteMemory(NULL, g_AllocatedBuffer, source_data, sizeof(source_data));

    printk(KERN_INFO "Copied data: %s\n", (char*)g_AllocatedBuffer);

    // 3. Zeroing (Set Memory)
    PlatformSetMemory(g_AllocatedBuffer, 0, g_BufferSize);

    printk(KERN_INFO "Buffer zeroed\n");

    return 0;
}

/**
 * @brief Module cleanup/exit entry point.
 *
 * @details Frees the global buffer allocated during initialization to prevent
 *          memory leaks.
 *
 * @return void
 */
static void __exit mock_exit(void)
{
    printk(KERN_INFO "Mock module unloading\n");

    // 4. Freeing
    if (g_AllocatedBuffer) {
        PlatformFreeMemory(g_AllocatedBuffer);
        g_AllocatedBuffer = NULL; // Good practice to nullify after free
    }
}

// Register entry and exit points
module_init(mock_init);
module_exit(mock_exit);

// Module Metadata
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alireza Moradi");
MODULE_DESCRIPTION("Cross-platform memory API test module");
