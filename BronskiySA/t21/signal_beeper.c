#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGINT);
    sigaddset(&signals, SIGQUIT);
    if (sigprocmask(SIG_BLOCK, &signals, NULL) == -1) {
        perror("sigprocmask");
        return 1;
    }

    puts("Ctrl-C: beep. Ctrl-\\: show count and quit.");
    fflush(stdout);
    unsigned long count = 0;

    for (;;) {
        int signo;
        int error = sigwait(&signals, &signo);
        if (error != 0) {
            fprintf(stderr, "sigwait: %s\n", strerror(error));
            return 1;
        }
        if (signo == SIGQUIT)
            break;

        if (putchar('\a') == EOF || fflush(stdout) == EOF) {
            perror("stdout");
            return 1;
        }
        ++count;
    }

    printf("Beeps: %lu\n", count);
    return 0;
}
