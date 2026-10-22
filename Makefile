CC = gcc
CFLAGS = -Wall -Wextra -I/opt/X11/include
LDFLAGS = -L/opt/X11/lib -lX11

TARGET = terminator
SOURCES = main.c gui.c input.c shell.c scroll.c variables.c io_redirect.c hotkeys.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
