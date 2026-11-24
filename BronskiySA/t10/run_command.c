#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s command [args...]\n", argv[0]);
        return 1;
    }

    char **command = &argv[1];
    pid_t child = fork();
    if (child == -1) {
        perror("fork");
        return 1;
    }
    if (child == 0) {
        execvp(command[0], command);
        int code = errno == ENOENT ? 127 : 126;
        perror(command[0]);
        _exit(code);
    }

    siginfo_t result;
    while (waitid(P_PID, (id_t)child, &result, WEXITED) == -1) {
        if (errno == EINTR)
            continue;
        perror("waitid");
        return 1;
    }

    const char *label = result.si_code == CLD_EXITED
                        ? "Exit code" : "Terminated by signal";
    printf("%s: %d\n", label, result.si_status);
    return 0;
}
