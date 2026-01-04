KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

obj-m += mymodule.o
mymodule-objs :=  mock.o \
              ../../include/platform/kernel/code/Mem.o 

HYPERDBG_ROOT := /home/alish/Documents/HyperDbg/hyperdbg

ccflags-y += -I$(HYPERDBG_ROOT)/include

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
