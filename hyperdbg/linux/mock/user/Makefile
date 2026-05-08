CC      = gcc
PWD    := $(shell pwd)
CFLAGS  = -Wall -Wextra -std=c11 -O2
CFLAGS += -I$(PWD)/../../../include
CFLAGS += -I$(PWD)/../../../include/platform/user/header
CFLAGS += -I$(PWD)/../../../include/platform/user/code
TARGET  = mock
SRCS    = mock.c \
          platform-intrinsics.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: clean platform-intrinsics.c $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c pch.h
	$(CC) $(CFLAGS) -c -o $@ $<

platform-intrinsics.c:
	cp $(PWD)/../../../include/platform/user/code/platform-intrinsics.c $(PWD)/platform-intrinsics.c

clean:
	rm -f $(OBJS) $(TARGET)
	rm -f $(PWD)/platform-intrinsics.c