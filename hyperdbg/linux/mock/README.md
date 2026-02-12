# HyperDbg Linux Kernel Module

This directory contains a Linux kernel module that demonstrates and tests the  
cross-platform memory management APIs (`PlatformMem.c`) from the HyperDbg project.

The module provides a simple test harness (`mock.c`) that exercises memory  
allocation, copy, and deallocation functions in kernel space
  
---  
## Prerequisites
Before building the module, ensure you have the necessary development tools  
and kernel headers installed:

```bash  
# Install build essentials and kernel module utilities  
sudo apt-get update  
sudo apt-get install build-essential kmod  
  
# Install kernel headers for your running kernel  
sudo apt-get install linux-headers-`uname -r`  
```  

---
# project structure

```
hyperdbg/
 ├── include/
 │    └── platform/
 │         └── kernel/
 │              ├── header/
 │              └── code/
 └── linux/
      └── mock/
           ├── Makefile
           ├── mock.c
           └── HyperDbg.ko

```



---  
## Build the Module
```bash  
cd hyperdbg/linux/mock```  
then build module:  
```bash  
make  
```  
If successful, this will generate:

```bash  
HyperDbg.ko  
```  
---  
# Load the Module

to insert the module into the kernel:

```bash  
sudo insmod HyperDbg.ko
```
 
---  
# Verify Module Loaded Successfully

you can use `lsmod` :
```bash  
lsmod | grep HyperDbg
```

or you can inspect kernel module:
```bash  
sudo dmesg | tail -10
```  
---  
# Remove the Module
```bash  
sudo rmmod HyperDbg
```  

---

# finally you can clean project:
```bash  
make clean
```

