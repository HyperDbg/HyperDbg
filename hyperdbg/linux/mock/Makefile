obj-m += mymodule.o
mymodule-objs :=  mock.o \
              ../../include/platform/kernel/code/Mem.o 

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
EXTRA_CFLAGS += -I$(PWD)/../../include/platform/kernel/header

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
