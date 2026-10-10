#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void lex_init(Lexer *lx, const char *src) {
    lx->p = src;
    lx->line = 1;
}

static void die(int line, const char *msg) {
    fprintf(stderr, "error: line %d: %s\n", line, msg);
    exit(1);
}

static void skip_space_and_comments(Lexer *lx) {
    for (;;) {
        char c = *lx->p;
        if (c == '\n') {
            lx->line++;
            lx->p++;
        } else if (isspace((unsigned char)c)) {
            lx->p++;
        } else if (c == '/' && lx->p[1] == '/') {
            while (*lx->p && *lx->p != '\n') lx->p++;
        } else if (c == '/' && lx->p[1] == '*') {
            int start = lx->line;
            lx->p += 2;
            while (*lx->p && !(lx->p[0] == '*' && lx->p[1] == '/')) {
                if (*lx->p == '\n') lx->line++;
                lx->p++;
            }
            if (!*lx->p) die(start, "unterminated comment");
            lx->p += 2;
        } else {
            return;
        }
    }
}

Token lex_next(Lexer *lx) {
    Token t = {T_EOF, "", 0, 0};
    skip_space_and_comments(lx);
    t.line = lx->line;
    char c = *lx->p;

    if (!c) return t;

    if (isalpha((unsigned char)c) || c == '_') {
        size_t n = 0;
        while (isalnum((unsigned char)*lx->p) || *lx->p == '_') {
            if (n >= sizeof t.text - 1) die(t.line, "identifier too long");
            t.text[n++] = *lx->p++;
        }
        t.text[n] = 0;
        t.kind = T_IDENT;
        if (!strcmp(t.text, "void")) t.kind = T_VOID;
        else if (!strcmp(t.text, "while")) t.kind = T_WHILE;
        else if (!strcmp(t.text, "return")) t.kind = T_RETURN;
        return t;
    }

    if (c == '"') {
        size_t n = 0;
        lx->p++;
        while (*lx->p != '"') {
            if (!*lx->p || *lx->p == '\n') die(t.line, "unterminated string");
            if (n >= sizeof t.text - 1) die(t.line, "string too long");
            t.text[n++] = *lx->p++;
        }
        lx->p++;
        t.text[n] = 0;
        t.kind = T_STR;
        return t;
    }

    if (isdigit((unsigned char)c)) {
        char *end;
        t.num = strtol(lx->p, &end, 0);
        lx->p = end;
        t.kind = T_NUM;
        return t;
    }

    lx->p++;
    switch (c) {
    case '(': t.kind = T_LPAREN; break;
    case ')': t.kind = T_RPAREN; break;
    case '{': t.kind = T_LBRACE; break;
    case '}': t.kind = T_RBRACE; break;
    case ';': t.kind = T_SEMI; break;
    case ',': t.kind = T_COMMA; break;
    default: die(t.line, "unexpected character");
    }
    return t;
}
