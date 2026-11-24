CC = cc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
TARGET = list_metadata

all: $(TARGET)

$(TARGET): list_metadata.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ list_metadata.c $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
