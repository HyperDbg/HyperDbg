CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -O2 -I../../../include
TARGET  = mock
SRCS    = mock.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c pch.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)