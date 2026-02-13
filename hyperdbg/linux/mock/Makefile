KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

obj-m := Hyperdbg.o

Hyperdbg-objs := mock.o \
                 ../../include/platform/kernel/code/PlatformMem.o

ccflags-y += -I$(PWD)/../../include 
ccflags-y += -I$(PWD)/../../include/platform/kernel/header
ccflags-y += -I$(PWD)/../../include/platform/kernel/code


all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm ../../include/platform/kernel/code/PlatformMem.o
	rm ../../include/platform/kernel/code/.PlatformMem.o.cmd
