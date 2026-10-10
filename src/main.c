#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#include "compiler.h"
#include "emit.h"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "error: cannot open %s\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)n + 1);
    if (!buf || fread(buf, 1, (size_t)n, f) != (size_t)n) {
        fprintf(stderr, "error: cannot read %s\n", path);
        exit(1);
    }
    buf[n] = 0;
    fclose(f);
    return buf;
}

static FILE *open_out(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "error: cannot write %s\n", path);
        exit(1);
    }
    return f;
}

static int run_emulator(const char *vice, const char *prg) {
#ifdef _WIN32
    size_t vice_len = strlen(vice), prg_len = strlen(prg);
    char *command = malloc(2 * (vice_len + prg_len) + 8);
    if (!command) {
        fprintf(stderr, "error: out of memory while starting VICE\n");
        return 1;
    }
    char *p = command;
    const char *args[] = {vice, prg};
    for (size_t arg = 0; arg < 2; arg++) {
        const char *s = args[arg];
        *p++ = '"';
        while (*s) {
            size_t slashes = 0;
            while (*s == '\\') {
                slashes++;
                s++;
            }
            if (*s == '"') {
                for (size_t i = 0; i < slashes * 2 + 1; i++) *p++ = '\\';
                *p++ = *s++;
            } else if (*s) {
                for (size_t i = 0; i < slashes; i++) *p++ = '\\';
                *p++ = *s++;
            } else {
                for (size_t i = 0; i < slashes * 2; i++) *p++ = '\\';
            }
        }
        *p++ = '"';
        if (arg == 0) *p++ = ' ';
    }
    *p = '\0';

    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof(startup);
    if (!CreateProcessA(NULL, command, NULL, NULL, FALSE, 0, NULL, NULL,
                        &startup, &process)) {
        fprintf(stderr, "error: could not start VICE (%s), Windows error %lu\n",
                vice, (unsigned long)GetLastError());
        free(command);
        return 1;
    }
    free(command);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exit_code = 0;
    GetExitCodeProcess(process.hProcess, &exit_code);
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    return (int)exit_code;
#else
    pid_t child = fork();
    if (child < 0) {
        perror("error: could not start VICE");
        return 1;
    }
    if (child == 0) {
        execlp(vice, vice, prg, (char *)NULL);
        perror("error: could not start VICE");
        _exit(127);
    }
    int status;
    if (waitpid(child, &status, 0) < 0) {
        perror("error: waiting for VICE failed");
        return 1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
#endif
}

int main(int argc, char **argv) {
    const char *in = NULL, *prg = "out.prg", *asm_path = NULL;
    const char *vice = "x64sc";
    int run = 0;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-o") && i + 1 < argc) prg = argv[++i];
        else if (!strcmp(argv[i], "-S") && i + 1 < argc) asm_path = argv[++i];
        else if (!strcmp(argv[i], "--run")) run = 1;
        else if (!strcmp(argv[i], "--vice") && i + 1 < argc) vice = argv[++i];
        else in = argv[i];
    }
    if (!in) {
        fprintf(stderr, "usage: c64cc input.c [-o out.prg] [-S out.asm] [--run] [--vice path]\n");
        return 2;
    }

    char *src = read_file(in);
    compile(src);
    free(src);

    FILE *f = open_out(prg);
    emit_write_prg(f);
    fclose(f);

    if (asm_path) {
        f = open_out(asm_path);
        emit_write_asm(f);
        fclose(f);
    }
    return run ? run_emulator(vice, prg) : 0;
}
