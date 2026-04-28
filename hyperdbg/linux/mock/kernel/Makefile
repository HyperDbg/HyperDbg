KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

obj-m := HyperDbg.o
HyperDbg-objs := mock.o \
                 PlatformMem.o

ccflags-y += -I$(PWD)/../../../include 
ccflags-y += -I$(PWD)/../../../include/platform/kernel/header
ccflags-y += -I$(PWD)/../../../include/platform/kernel/code

all: PlatformMem.c
	$(MAKE) -C $(KDIR) M=$(PWD) modules

PlatformMem.c:
	cp ../../../include/platform/kernel/code/PlatformMem.c $(PWD)/PlatformMem.c

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f $(PWD)/PlatformMem.c
