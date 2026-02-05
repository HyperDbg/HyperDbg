# Define the kernel build directory
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# Define the root of your project
HYPERDBG_ROOT ?= /home/alish/Documents/HyperDbg/hyperdbg

# Name of the final kernel module
obj-m += mymodule.o

# List of object files required to build 'mymodule.o'
# Kbuild will automatically compile the corresponding .c files
mymodule-objs := mock.o \
                 ../../include/platform/kernel/code/mem.o

# Add include directories
ccflags-y += -I$(HYPERDBG_ROOT)/include \
             -I$(HYPERDBG_ROOT)/include/platform/header

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
