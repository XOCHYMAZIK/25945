CC = cc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
TARGET = run_command

all: $(TARGET)

$(TARGET): run_command.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ run_command.c $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
