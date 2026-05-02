KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
obj-m := HyperDbg.o
HyperDbg-objs := mock.o \
                 PlatformMem.o \
                 PlatformIntrinsicsVmx.o

ccflags-y += -I$(PWD)/../../../include 
ccflags-y += -I$(PWD)/../../../include/platform/kernel/header
ccflags-y += -I$(PWD)/../../../include/platform/kernel/code

all: clean PlatformMem.c PlatformIntrinsicsVmx.c
	$(MAKE) -C $(KDIR) M=$(PWD) modules

PlatformMem.c:
	cp ../../../include/platform/kernel/code/PlatformMem.c $(PWD)/PlatformMem.c
PlatformIntrinsicsVmx.c:
	cp ../../../include/platform/kernel/code/PlatformIntrinsicsVmx.c $(PWD)/PlatformIntrinsicsVmx.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(PWD)/PlatformMem.c
	rm -f $(PWD)/PlatformIntrinsicsVmx.c