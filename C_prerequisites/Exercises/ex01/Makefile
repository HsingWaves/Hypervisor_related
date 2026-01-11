CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -Og
TARGET = ex01
SRCS = main.c singly_linked_list.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET).exe

$(TARGET).exe: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	ifeq ($(OS),Windows_NT)
		del $(OBJS) $(TARGET).exe
	else
		rm -f $(OBJS) $(TARGET).exe
	endif

.PHONY: all clean
