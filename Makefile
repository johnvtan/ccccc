TARGET := COMPILERBABY
CC := gcc
CFLAGS := -Wall -Wextra
DEBUGFLAGS := -g -O0

SRCS := $(wildcard *.c)


all:
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

debug:
	$(CC) $(CFLAGS) $(DEBUGFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -rf $(TARGET) 
