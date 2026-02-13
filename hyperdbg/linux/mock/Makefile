KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

obj-m := Hyperdbg.o

Hyperdbg-objs := mock.o \
                 ../../include/platform/kernel/code/PlatformMem.o

ccflags-y += -I$(PWD)/../../include \
             -I$(PWD)/../../include/platform/kernel/header

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

