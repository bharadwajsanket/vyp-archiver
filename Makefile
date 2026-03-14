CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude
TARGET  = vypack
SRCDIR  = src
SRCS    = $(SRCDIR)/main.c $(SRCDIR)/archive.c $(SRCDIR)/index.c $(SRCDIR)/cli.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -lzstd

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
