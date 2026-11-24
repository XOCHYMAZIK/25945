#define _POSIX_C_SOURCE 200809L
#include <grp.h>
#include <inttypes.h>
#include <libgen.h>
#include <pwd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

static int list_path(char *path) {
    struct stat info;
    if (lstat(path, &info) == -1) {
        perror(path);
        return 1;
    }

    char mode[11] = {0};
    mode[0] = S_ISDIR(info.st_mode) ? 'd' : S_ISREG(info.st_mode) ? '-' : '?';
    for (int group = 0; group < 3; ++group) {
        mode_t rights = (info.st_mode >> (6 - 3 * group)) & 7;
        char *letters = mode + 1 + 3 * group;
        letters[0] = rights & 4 ? 'r' : '-';
        letters[1] = rights & 2 ? 'w' : '-';
        letters[2] = rights & 1 ? 'x' : '-';
    }

    char uid[32], gid[32], size[32] = "", date[24] = "?";
    snprintf(uid, sizeof uid, "%ju", (uintmax_t)info.st_uid);
    snprintf(gid, sizeof gid, "%ju", (uintmax_t)info.st_gid);
    struct passwd *owner = getpwuid(info.st_uid);
    struct group *group = getgrgid(info.st_gid);
    if (S_ISREG(info.st_mode))
        snprintf(size, sizeof size, "%jd", (intmax_t)info.st_size);
    struct tm *modified = localtime(&info.st_mtime);
    if (modified != NULL)
        strftime(date, sizeof date, "%Y-%m-%d %H:%M", modified);

    printf("%s %3ju %-12s %-12s %10s %-16s %s\n",
           mode, (uintmax_t)info.st_nlink, owner ? owner->pw_name : uid,
           group ? group->gr_name : gid, size, date, basename(path));
    return 0;
}

int main(int argc, char **argv) {
    char current[] = ".";
    if (argc == 1)
        return list_path(current);

    int failed = 0;
    for (int i = 1; i < argc; ++i)
        failed |= list_path(argv[i]);
    return failed;
}
