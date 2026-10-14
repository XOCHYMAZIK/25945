#define _POSIX_C_SOURCE 200809L
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
extern char **environ;
struct option { int key; char *value; };
static void usage(void) {
    puts("Usage: options [-ispucdv] [-U limit] [-C bytes] [-V NAME=value]\nOptions execute from right to left; u/U use RLIMIT_NOFILE.");
}
static int limit_value(int resource, const char *value) {
    struct rlimit lim;
    if (getrlimit(resource, &lim) < 0) { perror("getrlimit"); return 1; }
    if (!value) {
        if (lim.rlim_cur == RLIM_INFINITY) puts("unlimited");
        else printf("%" PRIuMAX "\n", (uintmax_t)lim.rlim_cur);
        return 0;
    }
    char *end; errno = 0;
    uintmax_t n = strtoumax(value, &end, 10);
    if (*value < '0' || *value > '9' || errno || *end || (uintmax_t)(rlim_t)n != n || (rlim_t)n == RLIM_INFINITY) {
        fprintf(stderr, "Invalid limit: %s\n", value); return 1;
    }
    lim.rlim_cur = (rlim_t)n;
    if (setrlimit(resource, &lim) < 0) { perror("setrlimit"); return 1; }
    return 0;
}
int main(int argc, char **argv) {
    if (argc == 1) { usage(); return 0; }
    size_t capacity = 0;
    for (int i = 1; i < argc; ++i) capacity += strlen(argv[i]);
    struct option *ops = calloc(capacity, sizeof(*ops));
    if (!ops) { perror("calloc"); return 1; }
    size_t count = 0; int key, result = 0;
    opterr = 0;
    while ((key = getopt(argc, argv, ":ispuU:cC:dvV:")) != -1) {
        if (key == '?' || key == ':') {
            fprintf(stderr, "Invalid option or missing argument: -%c\n", optopt);
            free(ops); return 1;
        }
        ops[count++] = (struct option){ key, optarg };
    }
    if (optind != argc) { fprintf(stderr, "Unexpected positional argument\n"); free(ops); return 1; }
    while (count) {
        struct option o = ops[--count];
        switch (o.key) {
        case 'i': printf("UID=%lu EUID=%lu GID=%lu EGID=%lu\n", (unsigned long)getuid(), (unsigned long)geteuid(), (unsigned long)getgid(), (unsigned long)getegid()); break;
        case 's': if (setpgid(0, 0) < 0) { perror("setpgid"); result = 1; } break;
        case 'p': printf("PID=%ld PPID=%ld PGID=%ld\n", (long)getpid(), (long)getppid(), (long)getpgrp()); break;
        case 'u': result |= limit_value(RLIMIT_NOFILE, NULL); break;
        case 'U': result |= limit_value(RLIMIT_NOFILE, o.value); break;
        case 'c': result |= limit_value(RLIMIT_CORE, NULL); break;
        case 'C': result |= limit_value(RLIMIT_CORE, o.value); break;
        case 'd': {
            size_t size = 256; char *buf = NULL;
            for (;;) {
                char *next = realloc(buf, size);
                if (!next) { free(buf); perror("realloc"); result = 1; break; }
                buf = next;
                if (getcwd(buf, size)) { puts(buf); free(buf); break; }
                if (errno != ERANGE || size > SIZE_MAX / 2) { perror("getcwd"); free(buf); result = 1; break; }
                size *= 2;
            }
            break;
        }
        case 'v': for (char **p = environ; *p; ++p) puts(*p); break;
        case 'V': {
            char *eq = strchr(o.value, '=');
            if (!eq || eq == o.value) { fprintf(stderr, "Expected NAME=value\n"); result = 1; break; }
            size_t len = (size_t)(eq - o.value);
            char *name = malloc(len + 1);
            if (!name) { perror("malloc"); result = 1; break; }
            memcpy(name, o.value, len); name[len] = '\0';
            if (setenv(name, eq + 1, 1) < 0) { perror("setenv"); result = 1; }
            free(name); break;
        }
        }
    }
    free(ops); return result;
}
