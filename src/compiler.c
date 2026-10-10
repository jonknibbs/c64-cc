#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emit.h"
#include "lexer.h"

static Lexer lx;
static Token tok;
static int label_counter;

static void die(const char *msg) {
    fprintf(stderr, "error: line %d: %s\n", tok.line, msg);
    exit(1);
}

static void advance(void) {
    tok = lex_next(&lx);
}

static void expect(TokKind kind, const char *what) {
    if (tok.kind != kind) {
        char buf[96];
        snprintf(buf, sizeof buf, "expected %s", what);
        die(buf);
    }
    advance();
}

/* Expressions are integer constants only for now. */
static long parse_const(void) {
    if (tok.kind != T_NUM) die("expected a constant");
    long v = tok.num;
    advance();
    return v;
}

static void new_label(char *buf, size_t n) {
    snprintf(buf, n, ".L%d", label_counter++);
}

static void parse_statement(void);

static void parse_block(void) {
    expect(T_LBRACE, "'{'");
    while (tok.kind != T_RBRACE) {
        if (tok.kind == T_EOF) die("unexpected end of file, missing '}'");
        parse_statement();
    }
    advance();
}

static void parse_while(void) {
    advance();
    expect(T_LPAREN, "'('");
    long cond = parse_const();
    expect(T_RPAREN, "')'");

    char top[32], end[32];
    new_label(top, sizeof top);
    new_label(end, sizeof end);

    emit_label(top);
    if (!cond) emit_op_label(OP_JMP, end);
    parse_statement();
    emit_op_label(OP_JMP, top);
    emit_label(end);
}

static void parse_call(void) {
    char name[64];
    snprintf(name, sizeof name, "%s", tok.text);
    advance();
    expect(T_LPAREN, "'('");

    if (!strcmp(name, "poke")) {
        long addr = parse_const();
        expect(T_COMMA, "','");
        long val = parse_const();
        if (addr < 0 || addr > 0xFFFF) die("poke address out of range (0..65535)");
        if (val < 0 || val > 0xFF) die("poke value out of range (0..255)");
        emit_op(OP_LDA, M_IMM, (int)val);
        emit_op(OP_STA, M_ABS, (int)addr);
    } else if (!strcmp(name, "print")) {
        long addr = parse_const();
        expect(T_COMMA, "','");
        if (tok.kind != T_STR) die("expected a string");
        char str[64];
        snprintf(str, sizeof str, "%s", tok.text);
        advance();
        if (addr < 0 || addr + (long)strlen(str) > 0x10000) die("print address out of range");
        for (size_t i = 0; str[i]; i++) {
            int c = (unsigned char)str[i];
            /* Letters map to screen codes 1..26; 32..63 match ASCII. */
            if (c >= 'a' && c <= 'z') c -= 'a' - 1;
            else if (c >= 'A' && c <= 'Z') c -= 'A' - 1;
            else if (c < 32 || c > 63) die("unsupported character in string");
            emit_op(OP_LDA, M_IMM, c);
            emit_op(OP_STA, M_ABS, (int)(addr + (long)i));
        }
    } else {
        char label[72];
        snprintf(label, sizeof label, "f_%s", name);
        emit_op_label(OP_JSR, label);
    }
    expect(T_RPAREN, "')'");
    expect(T_SEMI, "';'");
}

static void parse_statement(void) {
    switch (tok.kind) {
    case T_LBRACE: parse_block(); break;
    case T_WHILE: parse_while(); break;
    case T_RETURN:
        advance();
        expect(T_SEMI, "';'");
        emit_op(OP_RTS, M_IMP, 0);
        break;
    case T_IDENT: parse_call(); break;
    case T_SEMI: advance(); break;
    default: die("unexpected token in statement");
    }
}

static void parse_function(void) {
    expect(T_VOID, "'void'");
    if (tok.kind != T_IDENT) die("expected function name");
    char label[72];
    snprintf(label, sizeof label, "f_%s", tok.text);
    advance();
    expect(T_LPAREN, "'('");
    expect(T_RPAREN, "')'");

    emit_label(label);
    parse_block();
    emit_op(OP_RTS, M_IMP, 0);
}

void compile(const char *source) {
    emit_reset();
    label_counter = 0;
    lex_init(&lx, source);
    advance();

    /* Startup: call main, then park the CPU. */
    emit_op_label(OP_JSR, "f_main");
    emit_label("halt");
    emit_op_label(OP_JMP, "halt");

    while (tok.kind != T_EOF) parse_function();
}
