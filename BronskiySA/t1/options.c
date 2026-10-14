/* Включаем объявления функций POSIX, например getopt() и setenv(). */
#define _POSIX_C_SOURCE 200809L
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <errno.h>
#include <string.h>
/* Окружение процесса: массив строк "ИМЯ=значение", заканчивающийся NULL. */
extern char **environ;
/* Одна сохранённая опция: её буква и указатель на аргумент (если он есть). */
struct option { int key; char *value; };
/* Краткая инструкция, если программа запущена без опций. */
static void usage(void) {
    puts("Usage: options [-ispucdv] [-U limit] [-C bytes] [-V NAME=value]\nOptions execute from right to left; u/U use RLIMIT_NOFILE.");
}
/* Общая функция для лимитов: value == NULL — показать, иначе изменить. */
static int limit_value(int resource, const char *value) {
    /* rlim_cur — мягкий лимит; rlim_max — жёсткий предел. */
    struct rlimit lim;
    /* Получаем оба значения выбранного ограничения процесса. */
    if (getrlimit(resource, &lim) < 0) { perror("getrlimit"); return 1; }
    /* Без нового значения только печатаем текущий мягкий лимит. */
    if (!value) {
        if (lim.rlim_cur == RLIM_INFINITY) puts("unlimited");
        else printf("%" PRIuMAX "\n", (uintmax_t)lim.rlim_cur);
        return 0;
    }
    /* Преобразуем строку в число; end укажет на остаток строки, errno — на ошибку. */
    char *end; errno = 0;
    uintmax_t n = strtoumax(value, &end, 10);
    /* Отбрасываем неверный ввод, переполнение и значение вне диапазона rlim_t. */
    if (*value < '0' || *value > '9' || errno || *end || (uintmax_t)(rlim_t)n != n || (rlim_t)n == RLIM_INFINITY) {
        fprintf(stderr, "Invalid limit: %s\n", value); return 1;
    }
    /* Меняем только мягкий лимит. Жёсткий предел остаётся прежним. */
    lim.rlim_cur = (rlim_t)n;
    if (setrlimit(resource, &lim) < 0) { perror("setrlimit"); return 1; }
    return 0;
}
/* argc — число аргументов, argv — их строки; argv[0] — имя программы. */
int main(int argc, char **argv) {
    if (argc == 1) { usage(); return 0; }
    /* Сумма длин аргументов даёт достаточно места даже для группировки -ipd. */
    size_t capacity = 0;
    for (int i = 1; i < argc; ++i) capacity += strlen(argv[i]);
    /* calloc выделяет и обнуляет массив для сохранения опций. */
    struct option *ops = calloc(capacity, sizeof(*ops));
    if (!ops) { perror("calloc"); return 1; }
    /* count — число опций; result запоминает, была ли ошибка выполнения. */
    size_t count = 0; int key, result = 0;
    /* Отключаем сообщения getopt: ниже выводим собственную диагностику. */
    opterr = 0;
    /* Разбираем слева направо. Двоеточие после U, C, V требует аргумент.
       Первое двоеточие позволяет различать неизвестную опцию и пропущенный аргумент. */
    while ((key = getopt(argc, argv, ":ispuU:cC:dvV:")) != -1) {
        /* ? — неизвестная опция; : — не передан обязательный аргумент. */
        if (key == '?' || key == ':') {
            fprintf(stderr, "Invalid option or missing argument: -%c\n", optopt);
            free(ops); return 1;
        }
        /* Пока только сохраняем опцию. optarg указывает на её аргумент в argv. */
        ops[count++] = (struct option){ key, optarg };
    }
    /* После getopt не должно остаться обычных аргументов, например имени файла. */
    if (optind != argc) { fprintf(stderr, "Unexpected positional argument\n"); free(ops); return 1; }
    /* По условию выполняем опции СПРАВА НАЛЕВО, поэтому идём с конца массива. */
    while (count) {
        /* Сначала уменьшаем count, затем берём элемент с полученным индексом. */
        struct option o = ops[--count];
        /* По букве выбираем соответствующее действие. */
        switch (o.key) {
        /* Реальные и эффективные идентификаторы пользователя и группы. */
        case 'i': printf("UID=%lu EUID=%lu GID=%lu EGID=%lu\n", (unsigned long)getuid(), (unsigned long)geteuid(), (unsigned long)getgid(), (unsigned long)getegid()); break;
        /* Два нуля означают: текущий процесс, новая группа с PGID = его PID. */
        case 's': if (setpgid(0, 0) < 0) { perror("setpgid"); result = 1; } break;
        /* PID — процесс, PPID — родитель, PGID — группа процессов. */
        case 'p': printf("PID=%ld PPID=%ld PGID=%ld\n", (long)getpid(), (long)getppid(), (long)getpgrp()); break;
        /* u/U: показать или изменить мягкий лимит числа открытых файлов.
           |= сохраняет факт ошибки: последующий успех его не сбросит. */
        case 'u': result |= limit_value(RLIMIT_NOFILE, NULL); break;
        case 'U': result |= limit_value(RLIMIT_NOFILE, o.value); break;
        /* c/C: показать или изменить максимальный размер core-файла в байтах. */
        case 'c': result |= limit_value(RLIMIT_CORE, NULL); break;
        case 'C': result |= limit_value(RLIMIT_CORE, o.value); break;
        /* Получаем текущий каталог; если буфер мал, увеличиваем его вдвое. */
        case 'd': {
            size_t size = 256; char *buf = NULL;
            for (;;) {
                /* Временный указатель сохраняет старый буфер при ошибке realloc. */
                char *next = realloc(buf, size);
                if (!next) { free(buf); perror("realloc"); result = 1; break; }
                buf = next;
                if (getcwd(buf, size)) { puts(buf); free(buf); break; }
                /* ERANGE означает нехватку места; остальные ошибки прекращают попытки. */
                if (errno != ERANGE || size > SIZE_MAX / 2) { perror("getcwd"); free(buf); result = 1; break; }
                size *= 2;
            }
            break;
        }
        /* Обходим окружение до завершающего NULL и выводим каждую строку. */
        case 'v': for (char **p = environ; *p; ++p) puts(*p); break;
        /* Например, -VTEST=hello: отделяем имя TEST от значения hello. */
        case 'V': {
            /* Ищем первый знак =. Имя слева от него не должно быть пустым. */
            char *eq = strchr(o.value, '=');
            if (!eq || eq == o.value) { fprintf(stderr, "Expected NAME=value\n"); result = 1; break; }
            size_t len = (size_t)(eq - o.value);
            /* +1 нужен для завершающего нулевого символа строки C. */
            char *name = malloc(len + 1);
            if (!name) { perror("malloc"); result = 1; break; }
            memcpy(name, o.value, len); name[len] = '\0';
            /* eq + 1 — начало значения; 1 разрешает заменять существующую переменную.
               Меняется окружение только этой программы, а не родительского терминала. */
            if (setenv(name, eq + 1, 1) < 0) { perror("setenv"); result = 1; }
            free(name); break;
        }
        }
    }
    /* Освобождаем массив. 0 — успех, 1 — хотя бы одно действие завершилось ошибкой. */
    free(ops); return result;
}
