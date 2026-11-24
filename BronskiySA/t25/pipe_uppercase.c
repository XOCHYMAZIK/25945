#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static int send_text(int fd, const char *text) {
    FILE *output = fdopen(fd, "w");
    if (output == NULL) {
        perror("fdopen");
        close(fd);
        return 1;
    }
    int failed = fprintf(output, "%s\n", text) < 0;
    if (fclose(output) == EOF)
        failed = 1;
    return failed;
}

static int print_uppercase(int fd) {
    unsigned char buffer[128];
    for (;;) {
        ssize_t size = read(fd, buffer, sizeof buffer);
        if (size == 0)
            return fflush(stdout) == EOF;
        if (size < 0) {
            if (errno == EINTR)
                continue;
            perror("read");
            return 1;
        }
        for (ssize_t i = 0; i < size; ++i)
            buffer[i] = (unsigned char)toupper(buffer[i]);
        if (fwrite(buffer, 1, (size_t)size, stdout) != (size_t)size)
            return 1;
    }
}

int main(int argc, char **argv) {
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [text]\n", argv[0]);
        return 1;
    }
    const char *text = argc == 2 ? argv[1] : "Hello from the Child!";

    int channel[2];
    if (pipe(channel) == -1) {
        perror("pipe");
        return 1;
    }
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        close(channel[0]);
        close(channel[1]);
        return 1;
    }
    if (child == 0) {
        close(channel[0]);
        _exit(send_text(channel[1], text));
    }

    close(channel[1]);
    int failed = print_uppercase(channel[0]);
    close(channel[0]);

    int status;
    while (waitpid(child, &status, 0) == -1) {
        if (errno == EINTR)
            continue;
        perror("waitpid");
        return 1;
    }
    return failed || !WIFEXITED(status) || WEXITSTATUS(status) != 0;
}
