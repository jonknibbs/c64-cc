#ifndef LEXER_H
#define LEXER_H

typedef enum {
    T_EOF, T_IDENT, T_NUM, T_STR,
    T_VOID, T_WHILE, T_RETURN,
    T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_SEMI, T_COMMA
} TokKind;

typedef struct {
    TokKind kind;
    char text[64];
    long num;
    int line;
} Token;

typedef struct {
    const char *p;
    int line;
} Lexer;

void lex_init(Lexer *lx, const char *src);
Token lex_next(Lexer *lx);

#endif
