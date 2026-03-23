CC      = gcc
CFLAGS  = -std=c99 -Wall -Wextra -Wpedantic -O2 -Iinclude -I/opt/homebrew/include
LDFLAGS = -L/opt/homebrew/lib -lzstd
TARGET  = vypack
SRCDIR  = src
SRCS    = $(SRCDIR)/main.c $(SRCDIR)/archive.c $(SRCDIR)/index.c $(SRCDIR)/cli.c
OBJS    = $(SRCS:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)
