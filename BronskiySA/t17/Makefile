CC = cc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
TARGET = line_editor

all: $(TARGET)

$(TARGET): line_editor.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ line_editor.c $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
