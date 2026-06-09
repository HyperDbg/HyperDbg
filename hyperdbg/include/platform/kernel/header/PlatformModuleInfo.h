// PlatformModuleInfo.h

#if defined(__linux__)

#    ifndef MODULE_INFO_H
#        define MODULE_INFO_H

#        include <linux/module.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alish");
MODULE_DESCRIPTION("Linux Kernel module Mock");
MODULE_VERSION("0.1");

#    endif // _MODULE_INFO_H_

#endif // defined(__linux__)
