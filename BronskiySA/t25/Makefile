CC = cc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
TARGET = pipe_uppercase

all: $(TARGET)

$(TARGET): pipe_uppercase.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ pipe_uppercase.c $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
