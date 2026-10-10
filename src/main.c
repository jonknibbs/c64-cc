#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

typedef struct {
    char *data;
    size_t length;
    size_t capacity;
} StringBuffer;

typedef struct {
    char name[64];
    char value[32];
} NumericMacro;

static const char *include_dirs[32];
static size_t include_dir_count;
static const char *once_paths[64];
static size_t once_path_count;
static NumericMacro macros[128];
static size_t macro_count;
static int in_block_comment;

static void append_text(StringBuffer *buffer, const char *text, size_t length) {
    if (buffer->length + length + 1 > buffer->capacity) {
        size_t capacity = buffer->capacity ? buffer->capacity : 1024;
        while (capacity < buffer->length + length + 1) capacity *= 2;
        char *data = realloc(buffer->data, capacity);
        if (!data) {
            fprintf(stderr, "error: out of memory while reading source\n");
            exit(1);
        }
        buffer->data = data;
        buffer->capacity = capacity;
    }
    memcpy(buffer->data + buffer->length, text, length);
    buffer->length += length;
    buffer->data[buffer->length] = '\0';
}

static char *read_raw_file(const char *path) {
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

static int path_exists(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static void join_path(char *result, size_t result_size, const char *directory, const char *file) {
    size_t length = strlen(directory);
    if (length && (directory[length - 1] == '/' || directory[length - 1] == '\\'))
        snprintf(result, result_size, "%s%s", directory, file);
    else
        snprintf(result, result_size, "%s/%s", directory, file);
}

static int resolve_include(char *result, size_t result_size, const char *source_path,
                           const char *include_name) {
    const char *slash = strrchr(source_path, '/');
    const char *backslash = strrchr(source_path, '\\');
    if (!slash || (backslash && backslash > slash)) slash = backslash;
    if (slash) {
        char directory[1024];
        size_t length = (size_t)(slash - source_path);
        if (length >= sizeof directory) return 0;
        memcpy(directory, source_path, length);
        directory[length] = '\0';
        join_path(result, result_size, directory, include_name);
        if (path_exists(result)) return 1;
    }
    for (size_t i = 0; i < include_dir_count; i++) {
        join_path(result, result_size, include_dirs[i], include_name);
        if (path_exists(result)) return 1;
    }
    return 0;
}

static int include_was_loaded(const char *path) {
    for (size_t i = 0; i < once_path_count; i++)
        if (!strcmp(once_paths[i], path)) return 1;
    return 0;
}

static int directive_is(const char *line, size_t length, const char *directive) {
    size_t directive_length = strlen(directive);
    return length >= directive_length && !strncmp(line, directive, directive_length) &&
           (length == directive_length || isspace((unsigned char)line[directive_length]));
}

static void define_numeric_macro(const char *line, size_t length, const char *path) {
    const char *p = line + 7;
    const char *end = line + length;
    while (p < end && isspace((unsigned char)*p)) p++;
    if (p == end || !(isalpha((unsigned char)*p) || *p == '_')) {
        fprintf(stderr, "error: malformed #define in %s\n", path);
        exit(1);
    }
    char name[64];
    size_t name_length = 0;
    while (p < end && (isalnum((unsigned char)*p) || *p == '_')) {
        if (name_length + 1 >= sizeof name) {
            fprintf(stderr, "error: macro name too long in %s\n", path);
            exit(1);
        }
        name[name_length++] = *p++;
    }
    name[name_length] = '\0';
    if (p < end && *p == '(') {
        fprintf(stderr, "error: function-like macros are not supported in %s\n", path);
        exit(1);
    }
    while (p < end && isspace((unsigned char)*p)) p++;
    const char *value_start = p;
    while (p < end && !isspace((unsigned char)*p)) p++;
    size_t value_length = (size_t)(p - value_start);
    if (!value_length || value_length >= sizeof macros[0].value) {
        fprintf(stderr, "error: #define %s needs a numeric value in %s\n", name, path);
        exit(1);
    }
    char value[32];
    memcpy(value, value_start, value_length);
    value[value_length] = '\0';
    char *number_end;
    long numeric_value = strtol(value, &number_end, 0);
    if (*number_end || numeric_value < 0 || numeric_value > 0xFFFF) {
        fprintf(stderr, "error: #define %s must use a 16-bit nonnegative integer in %s\n", name, path);
        exit(1);
    }
    size_t index;
    for (index = 0; index < macro_count; index++)
        if (!strcmp(macros[index].name, name)) break;
    if (index == macro_count) {
        if (macro_count >= sizeof macros / sizeof macros[0]) {
            fprintf(stderr, "error: too many numeric macros\n");
            exit(1);
        }
        macro_count++;
    }
    snprintf(macros[index].name, sizeof macros[index].name, "%s", name);
    snprintf(macros[index].value, sizeof macros[index].value, "%s", value);
}

static const char *macro_value(const char *name, size_t length) {
    for (size_t i = 0; i < macro_count; i++)
        if (strlen(macros[i].name) == length && !strncmp(macros[i].name, name, length))
            return macros[i].value;
    return NULL;
}

static void append_expanded_line(StringBuffer *output, const char *line, size_t length) {
    size_t i = 0;
    while (i < length) {
        if (in_block_comment) {
            size_t start = i;
            while (i + 1 < length && !(line[i] == '*' && line[i + 1] == '/')) i++;
            if (i + 1 < length) {
                i += 2;
                in_block_comment = 0;
            } else {
                i = length;
            }
            append_text(output, line + start, i - start);
        } else if (i + 1 < length && line[i] == '/' && line[i + 1] == '*') {
            in_block_comment = 1;
            append_text(output, line + i, 2);
            i += 2;
        } else if (i + 1 < length && line[i] == '/' && line[i + 1] == '/') {
            append_text(output, line + i, length - i);
            break;
        } else if (line[i] == '"' || line[i] == '\'') {
            char quote = line[i];
            size_t start = i++;
            while (i < length) {
                if (line[i] == '\\' && i + 1 < length) i += 2;
                else if (line[i++] == quote) break;
            }
            append_text(output, line + start, i - start);
        } else if (isalpha((unsigned char)line[i]) || line[i] == '_') {
            size_t start = i++;
            while (i < length && (isalnum((unsigned char)line[i]) || line[i] == '_')) i++;
            const char *replacement = macro_value(line + start, i - start);
            if (replacement) append_text(output, replacement, strlen(replacement));
            else append_text(output, line + start, i - start);
        } else {
            append_text(output, line + i, 1);
            i++;
        }
    }
}

static void expand_source(const char *path, StringBuffer *output, int depth) {
    if (depth > 16) {
        fprintf(stderr, "error: include nesting exceeds 16 files at %s\n", path);
        exit(1);
    }
    if (include_was_loaded(path)) return;
    char *source = read_raw_file(path);
    const char *cursor = source;
    while (*cursor) {
        const char *end = strchr(cursor, '\n');
        size_t line_length = end ? (size_t)(end - cursor) : strlen(cursor);
        const char *p = cursor;
        while (p < cursor + line_length && isspace((unsigned char)*p)) p++;
        size_t directive_length = (size_t)((cursor + line_length) - p);
        if (directive_is(p, directive_length, "#pragma once")) {
            if (once_path_count >= sizeof once_paths / sizeof once_paths[0]) {
                fprintf(stderr, "error: too many #pragma once headers\n");
                free(source);
                exit(1);
            }
            size_t path_length = strlen(path) + 1;
            char *saved_path = malloc(path_length);
            if (!saved_path) {
                fprintf(stderr, "error: out of memory while tracking included headers\n");
                free(source);
                exit(1);
            }
            memcpy(saved_path, path, path_length);
            once_paths[once_path_count++] = saved_path;
        } else if (directive_is(p, directive_length, "#define")) {
            define_numeric_macro(p, directive_length, path);
        } else if (directive_is(p, directive_length, "#include")) {
            p += 8;
            while (p < cursor + line_length && isspace((unsigned char)*p)) p++;
            if (p == cursor + line_length || *p != '"') {
                fprintf(stderr, "error: only quoted #include headers are supported in %s\n", path);
                free(source);
                exit(1);
            }
            const char *name_start = ++p;
            while (p < cursor + line_length && *p != '"') p++;
            if (p == cursor + line_length) {
                fprintf(stderr, "error: malformed #include in %s\n", path);
                free(source);
                exit(1);
            }
            size_t name_length = (size_t)(p - name_start);
            char include_name[512], resolved[1024];
            if (!name_length || name_length >= sizeof include_name) {
                fprintf(stderr, "error: invalid include path in %s\n", path);
                free(source);
                exit(1);
            }
            memcpy(include_name, name_start, name_length);
            include_name[name_length] = '\0';
            if (!resolve_include(resolved, sizeof resolved, path, include_name)) {
                fprintf(stderr, "error: cannot find included header '%s' from %s\n", include_name, path);
                free(source);
                exit(1);
            }
            expand_source(resolved, output, depth + 1);
            append_text(output, "\n", 1);
        } else {
            append_expanded_line(output, cursor, line_length);
            append_text(output, "\n", end ? 1 : 0);
        }
        cursor += line_length + (end ? 1 : 0);
    }
    free(source);
}

static char *read_source(const char *path) {
    StringBuffer source = {0};
    once_path_count = 0;
    macro_count = 0;
    in_block_comment = 0;
    expand_source(path, &source, 0);
    if (!source.data) {
        source.data = calloc(1, 1);
        if (!source.data) {
            fprintf(stderr, "error: out of memory while reading source\n");
            exit(1);
        }
    }
    return source.data;
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
    HANDLE vice_file = CreateFileA(vice, FILE_READ_ATTRIBUTES,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                   NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (vice_file == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "error: could not open VICE executable (%s), Windows error %lu\n",
                vice, (unsigned long)GetLastError());
        return 1;
    }
    char real_vice[32768];
    DWORD real_vice_len = GetFinalPathNameByHandleA(vice_file, real_vice,
                                                     sizeof(real_vice), FILE_NAME_NORMALIZED);
    CloseHandle(vice_file);
    if (real_vice_len == 0 || real_vice_len >= sizeof(real_vice)) {
        fprintf(stderr, "error: could not resolve VICE executable path\n");
        return 1;
    }
    if (strncmp(real_vice, "\\\\?\\", 4) == 0) {
        memmove(real_vice, real_vice + 4, real_vice_len - 3);
        real_vice_len -= 4;
    }

    size_t vice_len = strlen(real_vice), prg_len = strlen(prg);
    char *command = malloc(2 * (vice_len + prg_len) + 24);
    if (!command) {
        fprintf(stderr, "error: out of memory while starting VICE\n");
        return 1;
    }
    char *p = command;
    const char *args[] = {real_vice, "-autostart", prg};
    for (size_t arg = 0; arg < 3; arg++) {
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
        if (arg != 2) *p++ = ' ';
    }
    *p = '\0';

    STARTUPINFOA startup = {0};
    PROCESS_INFORMATION process = {0};
    startup.cb = sizeof(startup);
    if (!CreateProcessA(real_vice, command, NULL, NULL, FALSE, 0, NULL, NULL,
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
        execlp(vice, vice, "-autostart", prg, (char *)NULL);
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
        if (!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
            printf("usage: c64cc input.c [-I dir] [-o out.prg] [-S out.asm] [--run] [--vice path]\n"
                   "       c64cc --version\n");
            return 0;
        } else if (!strcmp(argv[i], "--version")) {
            puts("c64cc 0.2.0");
            return 0;
        } else if (!strcmp(argv[i], "-I") || !strcmp(argv[i], "-o") || !strcmp(argv[i], "-S") ||
                   !strcmp(argv[i], "--vice")) {
            if (i + 1 >= argc) {
                fprintf(stderr, "error: %s requires a path\n", argv[i]);
                return 2;
            }
            if (!strcmp(argv[i], "-I")) {
                if (include_dir_count >= sizeof include_dirs / sizeof include_dirs[0]) {
                    fprintf(stderr, "error: too many include directories\n");
                    return 2;
                }
                include_dirs[include_dir_count++] = argv[++i];
            } else if (!strcmp(argv[i], "-o")) prg = argv[++i];
            else if (!strcmp(argv[i], "-S")) asm_path = argv[++i];
            else vice = argv[++i];
        }
        else if (!strcmp(argv[i], "--run")) run = 1;
        else if (argv[i][0] == '-') {
            fprintf(stderr, "error: unknown option: %s\n", argv[i]);
            return 2;
        } else if (in) {
            fprintf(stderr, "error: only one input file is supported\n");
            return 2;
        } else in = argv[i];
    }
    if (!in) {
        fprintf(stderr, "usage: c64cc input.c [-I dir] [-o out.prg] [-S out.asm] [--run] [--vice path]\n");
        return 2;
    }

    char *src = read_source(in);
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
