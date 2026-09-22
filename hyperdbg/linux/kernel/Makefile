# SPDX-License-Identifier: GPL-3.0
#
# Outer wrapper that drives the kernel build for HyperDbg.ko.
#
# The actual object list + flags live in the root Kbuild ($(ROOT)/Kbuild).
# We invoke the running kernel's build system with M= pointing at the repo
# root, so every source (hyperhv/, hyperkd/, include/, script-eval/, ...) is
# inside the module tree — no copying or symlinking of sources is needed.
#
#   make            # build HyperDbg.ko (lands at $(ROOT)/HyperDbg.ko)
#   make clean      # remove kbuild artifacts scattered under $(ROOT)
#   make load       # insmod the freshly built module (needs root)
#   make unload     # rmmod it (needs root)
#
# Port workflow (compile ONE file at a time while stubbing out the WDK):
#
#   make one FILE=include/platform/kernel/code/PlatformDpc.c
#
# 'one' compiles a SINGLE translation unit to its .o and stops — it does NOT
# link the module, so you see only that file's own compile/WDK errors instead
# of a wall of undefined-symbol link errors from the not-yet-ported files. The
# file does NOT need to be listed in $(ROOT)/Kbuild for this to work (kbuild's
# single-object target still applies the Kbuild ccflags-y include paths). Once
# a file compiles clean, move its line into the ACTIVE block of $(ROOT)/Kbuild
# and run a full `make` to fold it into HyperDbg.ko. FILE may be given as the
# .c source or the .o object; either is accepted, relative to the repo root.
#
# Override the kernel tree with:  make KDIR=/path/to/linux-headers

KDIR ?= /lib/modules/$(shell uname -r)/build
ROOT := $(abspath $(CURDIR)/../..)

.PHONY: all clean load unload modules_install one

all:
	$(MAKE) -C $(KDIR) M=$(ROOT) modules

# Compile a single TU in isolation (no module link). See the header above.
one:
ifndef FILE
	$(error Usage: make one FILE=<path/to/File.c relative to repo root>)
endif
	$(MAKE) -C $(KDIR) M=$(ROOT) $(FILE:.c=.o)

clean:
	$(MAKE) -C $(KDIR) M=$(ROOT) clean

modules_install:
	$(MAKE) -C $(KDIR) M=$(ROOT) modules_install

load:
	sudo insmod $(ROOT)/HyperDbg.ko

unload:
	sudo rmmod HyperDbg
