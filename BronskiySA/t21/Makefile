CC = cc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
TARGET = signal_beeper

all: $(TARGET)

$(TARGET): signal_beeper.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ signal_beeper.c $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
