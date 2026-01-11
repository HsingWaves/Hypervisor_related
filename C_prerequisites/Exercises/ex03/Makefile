CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -g
TARGET = ex03

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET).exe main.c

clean:
	rm -f $(TARGET).exe

.PHONY: all clean
