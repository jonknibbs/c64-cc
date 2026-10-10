#ifndef LEXER_H
#define LEXER_H

typedef enum {
    T_EOF, T_IDENT, T_NUM, T_STR,
    T_VOID, T_CHAR, T_INT, T_UNSIGNED, T_IF, T_ELSE, T_WHILE,
    T_RETURN, T_FOR, T_BREAK, T_CONTINUE,
    T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE, T_LBRACKET, T_RBRACKET,
    T_SEMI, T_COMMA,
    T_ASSIGN, T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_AMP, T_PIPE, T_CARET, T_TILDE, T_BANG,
    T_EQ, T_NE, T_LT, T_LE, T_GT, T_GE, T_SHL, T_SHR,
    T_AND_AND, T_OR_OR, T_INC, T_DEC
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
