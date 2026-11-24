#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
    int no_wait = argc == 3 && strcmp(argv[1], "--no-wait") == 0;
    if (argc != 2 && !no_wait) {
        fprintf(stderr, "Usage: %s [--no-wait] file\n", argv[0]);
        return 1;
    }

    char *path = argv[argc - 1];
    /* A single '-' means stdin to cat, even after '--'. */
    char *command[] = {"cat", "--", strcmp(path, "-") == 0 ? "./-" : path, NULL};
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        execvp(command[0], command);
        perror("execvp cat");
        _exit(127);
    }

    printf("[PARENT] Child PID: %ld; parent is working.\n", (long)child);
    fflush(stdout);
    if (no_wait) {
        puts("[PARENT] Finished without waiting.");
        return 0;
    }

    int status;
    while (wait(&status) == -1) {
        if (errno == EINTR)
            continue;
        perror("wait");
        return 1;
    }

    int code = WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
    printf("[PARENT] Child status: %d\n", code);
    puts("[PARENT] Child has finished. This is the last line.");
    return code;
}
