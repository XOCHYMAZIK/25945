#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

enum { WIDTH = 40, CTRL_D = 4, CTRL_W = 23 };

static void erase_chars(size_t count) {
    for (size_t i = 0; i < count; ++i)
        fputs("\b \b", stdout);
}

static size_t word_start(const char *line, size_t end) {
    while (end > 0 && line[end - 1] != ' ')
        --end;
    return end;
}

static int edit(cc_t erase, cc_t kill) {
    char line[WIDTH];
    size_t length = 0;
    unsigned char key;

    while (!ferror(stdout)) {
        ssize_t size = read(STDIN_FILENO, &key, 1);
        if (size == 0)
            break;
        if (size < 0) {
            if (errno == EINTR)
                continue;
            perror("read");
            return 1;
        }

        if (key == CTRL_D) {
            if (length == 0)
                break;
        } else if (key == erase || key == kill || key == CTRL_W) {
            size_t keep = length;
            if (key == erase && keep > 0)
                --keep;
            if (key == kill)
                keep = 0;
            if (key == CTRL_W) {
                while (keep > 0 && line[keep - 1] == ' ')
                    --keep;
                keep = word_start(line, keep);
            }
            erase_chars(length - keep);
            length = keep;
        } else if (isprint(key)) {
            if (length == WIDTH) {
                size_t start = key == ' ' ? length : word_start(line, length);
                if (start == 0)
                    start = length;
                size_t carry = length - start;
                erase_chars(carry);
                putchar('\n');
                memmove(line, line + start, carry);
                fwrite(line, 1, carry, stdout);
                length = carry;
            }
            line[length++] = (char)key;
            putchar(key);
        } else {
            putchar('\a');
        }
    }

    return ferror(stdout) != 0;
}

int main(void) {
    struct termios original;
    if (tcgetattr(STDIN_FILENO, &original) == -1) {
        perror("tcgetattr");
        return 1;
    }

    struct termios mode = original;
    mode.c_lflag &= ~(ICANON | ECHO | ECHONL | ISIG | IEXTEN);
    mode.c_iflag &= ~IXON;
    mode.c_cc[VMIN] = 1;
    mode.c_cc[VTIME] = 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &mode) == -1) {
        perror("tcsetattr");
        return 1;
    }

    int failed = edit(original.c_cc[VERASE], original.c_cc[VKILL]);
    if (tcsetattr(STDIN_FILENO, TCSANOW, &original) == -1) {
        perror("tcsetattr");
        return 1;
    }
    return failed;
}
