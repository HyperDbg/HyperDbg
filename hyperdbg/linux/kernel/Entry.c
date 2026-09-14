/**
 * @file Entry.c
 * @brief Temporary module_init/module_exit entry point for HyperDbg.ko.
 *
 * @details This is scaffolding, NOT the real driver. The real entry point is
 *          hyperkd/code/driver/Driver.c (DriverEntry + IoCreateDevice/IOCTL),
 *          which cannot compile until the WDK surface is routed through the
 *          platform layer. Until then, this TU gives the skeleton a loadable
 *          module and smoke-tests that the active platform wrappers link.
 *
 *          Delete this file (and its line in Kbuild) once hyperkd's driver
 *          entry compiles on Linux.
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

static PVOID  g_SmokeBuffer = NULL;
static SIZE_T g_SmokeSize   = 4096;

static int __init
HyperDbgInit(void)
{
    char probe[] = "HyperDbg platform layer online";

    pr_info("HyperDbg: loading (skeleton entry)\n");

    //
    // Exercise the currently-active platform wrappers so a broken link shows up
    // at load time rather than silently.
    //
    g_SmokeBuffer = PlatformAllocateMemory(g_SmokeSize);
    if (!g_SmokeBuffer)
        return -ENOMEM;

    PlatformWriteMemory(g_SmokeBuffer, probe, sizeof(probe));
    pr_info("HyperDbg: platform buffer says: %s\n", (char *)g_SmokeBuffer);
    PlatformSetMemory(g_SmokeBuffer, 0, g_SmokeSize);

    return 0;
}

static void __exit
HyperDbgExit(void)
{
    if (g_SmokeBuffer)
    {
        PlatformFreeMemory(g_SmokeBuffer);
        g_SmokeBuffer = NULL;
    }
    pr_info("HyperDbg: unloading (skeleton entry)\n");
}

module_init(HyperDbgInit);
module_exit(HyperDbgExit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("HyperDbg contributors");
MODULE_DESCRIPTION("HyperDbg kernel module (Linux port, skeleton)");
