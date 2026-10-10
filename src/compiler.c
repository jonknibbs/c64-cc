#include "compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "emit.h"
#include "lexer.h"

#define MAX_SYMBOLS 512
#define MAX_SLOTS 1024
#define MAX_FUNCTIONS 128
#define MAX_PARAMS 8

typedef struct {
    char name[64];
    char function[64];
    char label[64];
    char high_label[64];
    size_t array_length;
    int is_array;
    int is_pointer;
} Symbol;

typedef struct {
    char label[64];
    int value;
} Slot;

typedef struct {
    char name[64];
    char param_names[MAX_PARAMS][64];
    char param_labels[MAX_PARAMS][64];
    size_t param_count;
    int returns_value;
    int has_signature;
    int defined;
} Function;

typedef struct {
    char break_label[32];
    char continue_label[32];
} LoopContext;

static Lexer lx;
static Token tok;
static Symbol symbols[MAX_SYMBOLS];
static Slot slots[MAX_SLOTS];
static Function functions[MAX_FUNCTIONS];
static LoopContext loops[32];
static size_t symbol_count, slot_count, function_count;
static size_t loop_depth;
static int label_counter, temp_counter, symbol_counter;
static int array_counter;
static int pointer_counter;
static char current_function[64];

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

static long parse_const(void) {
    if (tok.kind != T_NUM) die("expected a constant");
    long value = tok.num;
    advance();
    return value;
}

static void new_label(char *buf, size_t n) {
    snprintf(buf, n, ".L%d", label_counter++);
}

static void add_slot(const char *label, int value) {
    if (slot_count >= MAX_SLOTS) die("too many variables or temporaries");
    snprintf(slots[slot_count].label, sizeof slots[slot_count].label, "%s", label);
    slots[slot_count++].value = value & 0xFF;
}

static void new_temp(char *buf, size_t n) {
    snprintf(buf, n, "t_%d", temp_counter++);
    add_slot(buf, 0);
}

static int find_symbol(const char *name) {
    for (size_t i = symbol_count; i > 0; i--) {
        Symbol *symbol = &symbols[i - 1];
        if (!strcmp(symbol->name, name) &&
            (!symbol->function[0] || !strcmp(symbol->function, current_function)))
            return (int)(i - 1);
    }
    return -1;
}

static int find_function(const char *name) {
    for (size_t i = 0; i < function_count; i++)
        if (!strcmp(functions[i].name, name)) return (int)i;
    return -1;
}

static int get_function(const char *name) {
    int index = find_function(name);
    if (index >= 0) return index;
    if (function_count >= MAX_FUNCTIONS) die("too many functions");
    index = (int)function_count++;
    memset(&functions[index], 0, sizeof functions[index]);
    snprintf(functions[index].name, sizeof functions[index].name, "%s", name);
    return index;
}

static void add_symbol_array(const char *name, int initial_value, size_t length, int is_array) {
    if (find_symbol(name) >= 0) die("duplicate variable declaration");
    if (symbol_count >= MAX_SYMBOLS) die("too many variables");
    Symbol *symbol = &symbols[symbol_count++];
    memset(symbol, 0, sizeof *symbol);
    snprintf(symbol->name, sizeof symbol->name, "%s", name);
    snprintf(symbol->function, sizeof symbol->function, "%s", current_function);
    if (current_function[0])
        snprintf(symbol->label, sizeof symbol->label, "v_%d_%s", symbol_counter++, name);
    else
        snprintf(symbol->label, sizeof symbol->label, "g_%s", name);
    symbol->array_length = length;
    symbol->is_array = is_array;
    add_slot(symbol->label, initial_value);
    int array_id = array_counter++;
    for (size_t i = 1; i < length; i++) {
        char element_label[64];
        snprintf(element_label, sizeof element_label, "a_%d_%zu", array_id, i);
        add_slot(element_label, 0);
    }
}

static void add_pointer_symbol(const char *name, unsigned int address) {
    if (find_symbol(name) >= 0) die("duplicate variable declaration");
    if (symbol_count >= MAX_SYMBOLS) die("too many variables");
    Symbol *symbol = &symbols[symbol_count++];
    memset(symbol, 0, sizeof *symbol);
    snprintf(symbol->name, sizeof symbol->name, "%s", name);
    snprintf(symbol->function, sizeof symbol->function, "%s", current_function);
    int id = pointer_counter++;
    snprintf(symbol->label, sizeof symbol->label, "ptr_%d_lo", id);
    snprintf(symbol->high_label, sizeof symbol->high_label, "ptr_%d_hi", id);
    symbol->is_pointer = 1;
    add_slot(symbol->label, address & 0xFF);
    add_slot(symbol->high_label, address >> 8);
}

static void add_parameter_symbol(const char *name, const char *label) {
    if (find_symbol(name) >= 0) die("duplicate parameter or local variable");
    if (symbol_count >= MAX_SYMBOLS) die("too many variables");
    Symbol *symbol = &symbols[symbol_count++];
    snprintf(symbol->name, sizeof symbol->name, "%s", name);
    snprintf(symbol->function, sizeof symbol->function, "%s", current_function);
    snprintf(symbol->label, sizeof symbol->label, "%s", label);
}

static int parse_byte_type(void) {
    if (tok.kind == T_UNSIGNED) {
        advance();
        if (tok.kind != T_CHAR && tok.kind != T_INT)
            die("expected char or int after unsigned");
        advance();
        return 1;
    }
    if (tok.kind == T_CHAR || tok.kind == T_INT) {
        advance();
        return 1;
    }
    return 0;
}

static void parse_expression(void);
static void parse_statement(void);
static void parse_local_declaration(void);
static void parse_function_call(const char *name, int want_value);

static void emit_pointer_setup(const Symbol *symbol) {
    emit_op_label(OP_LDA, symbol->label);
    emit_op(OP_STA, M_ABS, 0xFB);
    emit_op_label(OP_LDA, symbol->high_label);
    emit_op(OP_STA, M_ABS, 0xFC);
}

static void parse_block(void) {
    expect(T_LBRACE, "'{'");
    while (tok.kind != T_RBRACE) {
        if (tok.kind == T_EOF) die("unexpected end of file, missing '}'");
        if (tok.kind == T_CHAR || tok.kind == T_INT || tok.kind == T_UNSIGNED)
            parse_local_declaration();
        else
            parse_statement();
    }
    advance();
}

static void parse_primary(void) {
    if (tok.kind == T_NUM) {
        if (tok.num < 0 || tok.num > 255) die("byte constant out of range (0..255)");
        emit_op(OP_LDA, M_IMM, (int)tok.num);
        advance();
        return;
    }
    if (tok.kind == T_LPAREN) {
        advance();
        parse_expression();
        expect(T_RPAREN, "')'");
        return;
    }
    if (tok.kind == T_IDENT) {
        char name[64];
        snprintf(name, sizeof name, "%s", tok.text);
        advance();
        if (!strcmp(name, "peek") && tok.kind == T_LPAREN) {
            advance();
            long address = parse_const();
            if (address < 0 || address > 0xFFFF) die("peek address out of range (0..65535)");
            expect(T_RPAREN, "')'");
            emit_op(OP_LDA, M_ABS, (int)address);
            return;
        }
        if (!strcmp(name, "key") && tok.kind == T_LPAREN) {
            advance();
            expect(T_RPAREN, "')'");
            emit_op(OP_JSR, M_ABS, 0xFFE4);
            return;
        }
        if (tok.kind == T_LPAREN) {
            if (!strcmp(name, "poke") || !strcmp(name, "print"))
                die("poke and print are statements, not expressions");
            advance();
            parse_function_call(name, 1);
            return;
        }
        int index = find_symbol(name);
        if (index < 0) die("unknown variable");
        if (symbols[index].is_pointer) die("pointer value requires dereference");
        if (tok.kind == T_LBRACKET) {
            if (!symbols[index].is_array) die("subscripted value is not an array");
            advance();
            parse_expression();
            expect(T_RBRACKET, "']'");
            emit_op(OP_TAX, M_IMP, 0);
            emit_op_indexed_label(OP_LDA, symbols[index].label);
            return;
        }
        if (symbols[index].is_array) die("array must be indexed");
        emit_op_label(OP_LDA, symbols[index].label);
        return;
    }
    die("expected an expression");
}

static void parse_unary(void) {
    if (tok.kind == T_STAR) {
        advance();
        if (tok.kind != T_IDENT) die("expected pointer name after '*'");
        int index = find_symbol(tok.text);
        if (index < 0 || !symbols[index].is_pointer)
            die("dereference requires a pointer variable");
        advance();
        emit_pointer_setup(&symbols[index]);
        emit_op(OP_LDY, M_IMM, 0);
        emit_op_indirect_y(OP_LDA, 0xFB);
    } else if (tok.kind == T_PLUS) {
        advance();
        parse_unary();
    } else if (tok.kind == T_MINUS) {
        advance();
        parse_unary();
        emit_op(OP_EOR, M_IMM, 0xFF);
        emit_op(OP_CLC, M_IMP, 0);
        emit_op(OP_ADC, M_IMM, 1);
    } else if (tok.kind == T_TILDE) {
        advance();
        parse_unary();
        emit_op(OP_EOR, M_IMM, 0xFF);
    } else if (tok.kind == T_BANG) {
        char yes[32], done[32];
        advance();
        parse_unary();
        emit_op(OP_CMP, M_IMM, 0);
        new_label(yes, sizeof yes);
        new_label(done, sizeof done);
        emit_op_label(OP_BEQ, yes);
        emit_op(OP_LDA, M_IMM, 0);
        emit_op_label(OP_JMP, done);
        emit_label(yes);
        emit_op(OP_LDA, M_IMM, 1);
        emit_label(done);
    } else {
        parse_primary();
    }
}

typedef void (*ParseLevel)(void);

static void parse_binary_level(ParseLevel lower, const TokKind *kinds,
                               const Op *ops, size_t count) {
    lower();
    for (;;) {
        size_t selected = count;
        for (size_t i = 0; i < count; i++) {
            if (tok.kind == kinds[i]) { selected = i; break; }
        }
        if (selected == count) return;

        char left[64], right[64];
        new_temp(left, sizeof left);
        emit_op_label(OP_STA, left);
        advance();
        lower();
        new_temp(right, sizeof right);
        emit_op_label(OP_STA, right);
        emit_op_label(OP_LDA, left);
        if (ops[selected] == OP_ADC) emit_op(OP_CLC, M_IMP, 0);
        if (ops[selected] == OP_SBC) emit_op(OP_SEC, M_IMP, 0);
        emit_op_label(ops[selected], right);
    }
}

static void parse_multiply(void) {
    parse_unary();
    while (tok.kind == T_STAR || tok.kind == T_SLASH || tok.kind == T_PERCENT) {
        TokKind operation = tok.kind;
        char left[64], right[64], result[64], loop[32], done[32];
        new_temp(left, sizeof left);
        emit_op_label(OP_STA, left);
        advance();
        parse_unary();
        new_temp(right, sizeof right);
        emit_op_label(OP_STA, right);
        new_temp(result, sizeof result);
        new_label(loop, sizeof loop);
        new_label(done, sizeof done);

        if (operation == T_STAR) {
            emit_op(OP_LDA, M_IMM, 0);
            emit_op_label(OP_STA, result);
            emit_label(loop);
            emit_op_label(OP_LDA, right);
            emit_op_label(OP_BEQ, done);
            emit_op_label(OP_LDA, result);
            emit_op(OP_CLC, M_IMP, 0);
            emit_op_label(OP_ADC, left);
            emit_op_label(OP_STA, result);
            emit_op_label(OP_LDA, right);
            emit_op(OP_SEC, M_IMP, 0);
            emit_op(OP_SBC, M_IMM, 1);
            emit_op_label(OP_STA, right);
            emit_op_label(OP_JMP, loop);
            emit_label(done);
            emit_op_label(OP_LDA, result);
        } else {
            emit_op(OP_LDA, M_IMM, 0);
            emit_op_label(OP_STA, result);
            emit_op_label(OP_LDA, right);
            emit_op_label(OP_BEQ, done);
            emit_label(loop);
            emit_op_label(OP_LDA, left);
            emit_op_label(OP_CMP, right);
            emit_op_label(OP_BCC, done);
            emit_op_label(OP_LDA, left);
            emit_op(OP_SEC, M_IMP, 0);
            emit_op_label(OP_SBC, right);
            emit_op_label(OP_STA, left);
            emit_op_label(OP_LDA, result);
            emit_op(OP_CLC, M_IMP, 0);
            emit_op(OP_ADC, M_IMM, 1);
            emit_op_label(OP_STA, result);
            emit_op_label(OP_JMP, loop);
            emit_label(done);
            if (operation == T_PERCENT) emit_op_label(OP_LDA, left);
            else emit_op_label(OP_LDA, result);
        }
    }
}

static void parse_add(void) {
    static const TokKind kinds[] = {T_PLUS, T_MINUS};
    static const Op ops[] = {OP_ADC, OP_SBC};
    parse_binary_level(parse_multiply, kinds, ops, 2);
}

static void parse_shift(void) {
    parse_add();
    while (tok.kind == T_SHL || tok.kind == T_SHR) {
        TokKind operation = tok.kind;
        char value[64], amount[64], result[64], loop[32], zero[32], exhausted[32], done[32];
        new_temp(value, sizeof value);
        emit_op_label(OP_STA, value);
        advance();
        parse_add();
        new_temp(amount, sizeof amount);
        emit_op_label(OP_STA, amount);
        new_temp(result, sizeof result);
        new_label(loop, sizeof loop);
        new_label(zero, sizeof zero);
        new_label(exhausted, sizeof exhausted);
        new_label(done, sizeof done);

        emit_op_label(OP_LDA, amount);
        emit_op(OP_CMP, M_IMM, 8);
        emit_op_label(OP_BCS, zero);
        emit_op_label(OP_LDA, value);
        emit_op_label(OP_STA, result);
        emit_label(loop);
        emit_op_label(OP_LDA, amount);
        emit_op_label(OP_BEQ, exhausted);
        emit_op_label(OP_LDA, result);
        emit_op(operation == T_SHL ? OP_ASL : OP_LSR, M_IMP, 0);
        emit_op_label(OP_STA, result);
        emit_op_label(OP_LDA, amount);
        emit_op(OP_SEC, M_IMP, 0);
        emit_op(OP_SBC, M_IMM, 1);
        emit_op_label(OP_STA, amount);
        emit_op_label(OP_JMP, loop);
        emit_label(exhausted);
        emit_op_label(OP_LDA, result);
        emit_op_label(OP_JMP, done);
        emit_label(zero);
        emit_op(OP_LDA, M_IMM, 0);
        emit_label(done);
    }
}

static void emit_comparison(TokKind operation, const char *left, const char *right) {
    char yes[32], no[32], done[32];
    new_label(yes, sizeof yes);
    new_label(no, sizeof no);
    new_label(done, sizeof done);
    emit_op_label(OP_LDA, left);
    emit_op_label(OP_CMP, right);
    if (operation == T_EQ) emit_op_label(OP_BEQ, yes);
    else if (operation == T_NE) emit_op_label(OP_BNE, yes);
    else if (operation == T_LT) emit_op_label(OP_BCC, yes);
    else if (operation == T_GE) emit_op_label(OP_BCS, yes);
    else if (operation == T_LE) {
        emit_op_label(OP_BCC, yes);
        emit_op_label(OP_BEQ, yes);
    } else if (operation == T_GT) {
        emit_op_label(OP_BCC, no);
        emit_op_label(OP_BNE, yes);
    }
    emit_op(OP_LDA, M_IMM, 0);
    emit_op_label(OP_JMP, done);
    emit_label(yes);
    emit_op(OP_LDA, M_IMM, 1);
    emit_op_label(OP_JMP, done);
    emit_label(no);
    emit_op(OP_LDA, M_IMM, 0);
    emit_label(done);
}

static void parse_relational(void) {
    static const TokKind kinds[] = {T_LT, T_LE, T_GT, T_GE};
    parse_shift();
    while (tok.kind == T_LT || tok.kind == T_LE || tok.kind == T_GT || tok.kind == T_GE) {
        TokKind operation = tok.kind;
        char left[64], right[64];
        new_temp(left, sizeof left);
        emit_op_label(OP_STA, left);
        advance();
        parse_shift();
        new_temp(right, sizeof right);
        emit_op_label(OP_STA, right);
        (void)kinds;
        emit_comparison(operation, left, right);
    }
}

static void parse_equality(void) {
    parse_relational();
    while (tok.kind == T_EQ || tok.kind == T_NE) {
        TokKind operation = tok.kind;
        char left[64], right[64];
        new_temp(left, sizeof left);
        emit_op_label(OP_STA, left);
        advance();
        parse_relational();
        new_temp(right, sizeof right);
        emit_op_label(OP_STA, right);
        emit_comparison(operation, left, right);
    }
}

static void parse_bitand(void) {
    static const TokKind kinds[] = {T_AMP};
    static const Op ops[] = {OP_AND};
    parse_binary_level(parse_equality, kinds, ops, 1);
}

static void parse_bitxor(void) {
    static const TokKind kinds[] = {T_CARET};
    static const Op ops[] = {OP_EOR};
    parse_binary_level(parse_bitand, kinds, ops, 1);
}

static void parse_bitor(void) {
    static const TokKind kinds[] = {T_PIPE};
    static const Op ops[] = {OP_ORA};
    parse_binary_level(parse_bitxor, kinds, ops, 1);
}

static void parse_logical_and(void) {
    parse_bitor();
    while (tok.kind == T_AND_AND) {
        char no[32], done[32];
        new_label(no, sizeof no);
        new_label(done, sizeof done);
        emit_op(OP_CMP, M_IMM, 0);
        emit_op_label(OP_BEQ, no);
        advance();
        parse_bitor();
        emit_op(OP_CMP, M_IMM, 0);
        emit_op_label(OP_BEQ, no);
        emit_op(OP_LDA, M_IMM, 1);
        emit_op_label(OP_JMP, done);
        emit_label(no);
        emit_op(OP_LDA, M_IMM, 0);
        emit_label(done);
    }
}

static void parse_logical_or(void) {
    parse_logical_and();
    while (tok.kind == T_OR_OR) {
        char yes[32], done[32];
        new_label(yes, sizeof yes);
        new_label(done, sizeof done);
        emit_op(OP_CMP, M_IMM, 0);
        emit_op_label(OP_BNE, yes);
        advance();
        parse_logical_and();
        emit_op(OP_CMP, M_IMM, 0);
        emit_op_label(OP_BNE, yes);
        emit_op(OP_LDA, M_IMM, 0);
        emit_op_label(OP_JMP, done);
        emit_label(yes);
        emit_op(OP_LDA, M_IMM, 1);
        emit_label(done);
    }
}

static void parse_expression(void) {
    parse_logical_or();
}

static void parse_while(void) {
    advance();
    expect(T_LPAREN, "'('");
    char top[32], end[32];
    new_label(top, sizeof top);
    new_label(end, sizeof end);
    emit_label(top);
    parse_expression();
    expect(T_RPAREN, "')'");
    emit_op(OP_CMP, M_IMM, 0);
    emit_op_label(OP_BEQ, end);
    if (loop_depth >= sizeof loops / sizeof loops[0]) die("loop nesting too deep");
    snprintf(loops[loop_depth].break_label, sizeof loops[loop_depth].break_label, "%s", end);
    snprintf(loops[loop_depth].continue_label, sizeof loops[loop_depth].continue_label, "%s", top);
    loop_depth++;
    parse_statement();
    loop_depth--;
    emit_op_label(OP_JMP, top);
    emit_label(end);
}

static void emit_for_clause(void) {
    if (tok.kind != T_IDENT) die("expected for-loop assignment or increment");
    char name[64];
    snprintf(name, sizeof name, "%s", tok.text);
    advance();
    int index = find_symbol(name);
    if (index < 0 || symbols[index].is_array)
        die("for-loop clause requires a scalar variable");
    if (tok.kind == T_ASSIGN) {
        advance();
        parse_expression();
        emit_op_label(OP_STA, symbols[index].label);
    } else if (tok.kind == T_INC || tok.kind == T_DEC) {
        TokKind operation = tok.kind;
        advance();
        emit_op_label(OP_LDA, symbols[index].label);
        if (operation == T_INC) {
            emit_op(OP_CLC, M_IMP, 0);
            emit_op(OP_ADC, M_IMM, 1);
        } else {
            emit_op(OP_SEC, M_IMP, 0);
            emit_op(OP_SBC, M_IMM, 1);
        }
        emit_op_label(OP_STA, symbols[index].label);
    } else {
        die("expected assignment or increment in for-loop clause");
    }
}

static void parse_for(void) {
    advance();
    expect(T_LPAREN, "'('");
    if (tok.kind == T_CHAR || tok.kind == T_INT || tok.kind == T_UNSIGNED)
        parse_local_declaration();
    else if (tok.kind == T_SEMI)
        advance();
    else {
        emit_for_clause();
        expect(T_SEMI, "';'");
    }

    char condition[32], update[32], body[32], done[32];
    new_label(condition, sizeof condition);
    new_label(update, sizeof update);
    new_label(body, sizeof body);
    new_label(done, sizeof done);
    emit_label(condition);
    if (tok.kind != T_SEMI) {
        parse_expression();
        emit_op(OP_CMP, M_IMM, 0);
        emit_op_label(OP_BEQ, done);
    }
    expect(T_SEMI, "';'");
    emit_op_label(OP_JMP, body);
    emit_label(update);
    if (tok.kind != T_RPAREN) emit_for_clause();
    expect(T_RPAREN, "')'");
    emit_op_label(OP_JMP, condition);
    emit_label(body);

    if (loop_depth >= sizeof loops / sizeof loops[0]) die("loop nesting too deep");
    snprintf(loops[loop_depth].break_label, sizeof loops[loop_depth].break_label, "%s", done);
    snprintf(loops[loop_depth].continue_label, sizeof loops[loop_depth].continue_label, "%s", update);
    loop_depth++;
    parse_statement();
    loop_depth--;
    emit_op_label(OP_JMP, update);
    emit_label(done);
}

static void parse_if(void) {
    advance();
    expect(T_LPAREN, "'('");
    char otherwise[32], done[32];
    new_label(otherwise, sizeof otherwise);
    new_label(done, sizeof done);
    parse_expression();
    expect(T_RPAREN, "')'");
    emit_op(OP_CMP, M_IMM, 0);
    emit_op_label(OP_BEQ, otherwise);
    parse_statement();
    if (tok.kind == T_ELSE) {
        emit_op_label(OP_JMP, done);
        emit_label(otherwise);
        advance();
        parse_statement();
        emit_label(done);
    } else {
        emit_label(otherwise);
    }
}

static void parse_call(const char *name) {
    expect(T_LPAREN, "'('");
    if (!strcmp(name, "poke")) {
        long address = parse_const();
        expect(T_COMMA, "','");
        parse_expression();
        if (address < 0 || address > 0xFFFF) die("poke address out of range (0..65535)");
        emit_op(OP_STA, M_ABS, (int)address);
    } else if (!strcmp(name, "sprite_enable") || !strcmp(name, "sprite_x_msb") ||
               !strcmp(name, "sprite_multicolor")) {
        long sprite = parse_const();
        if (sprite < 0 || sprite > 7) die("sprite number out of range (0..7)");
        expect(T_COMMA, "','");
        parse_expression();
        expect(T_RPAREN, "')'");
        expect(T_SEMI, "';'");
        int reg = !strcmp(name, "sprite_enable") ? 0xD015 :
                  !strcmp(name, "sprite_x_msb") ? 0xD010 : 0xD01C;
        int mask = 1 << sprite;
        char clear[32], done[32];
        new_label(clear, sizeof clear);
        new_label(done, sizeof done);
        emit_op(OP_CMP, M_IMM, 0);
        emit_op_label(OP_BEQ, clear);
        emit_op(OP_LDA, M_ABS, reg);
        emit_op(OP_ORA, M_IMM, mask);
        emit_op(OP_STA, M_ABS, reg);
        emit_op_label(OP_JMP, done);
        emit_label(clear);
        emit_op(OP_LDA, M_ABS, reg);
        emit_op(OP_AND, M_IMM, 0xFF ^ mask);
        emit_op(OP_STA, M_ABS, reg);
        emit_label(done);
        return;
    } else if (!strcmp(name, "sprite_position")) {
        long sprite = parse_const();
        if (sprite < 0 || sprite > 7) die("sprite number out of range (0..7)");
        expect(T_COMMA, "','");
        parse_expression();
        emit_op(OP_STA, M_ABS, 0xD000 + (int)sprite * 2);
        expect(T_COMMA, "','");
        parse_expression();
        emit_op(OP_STA, M_ABS, 0xD001 + (int)sprite * 2);
        expect(T_RPAREN, "')'");
        expect(T_SEMI, "';'");
        return;
    } else if (!strcmp(name, "sprite_color") || !strcmp(name, "sprite_pointer")) {
        long sprite = parse_const();
        if (sprite < 0 || sprite > 7) die("sprite number out of range (0..7)");
        expect(T_COMMA, "','");
        parse_expression();
        int address = !strcmp(name, "sprite_color") ? 0xD027 + (int)sprite : 0x07F8 + (int)sprite;
        emit_op(OP_STA, M_ABS, address);
        expect(T_RPAREN, "')'");
        expect(T_SEMI, "';'");
        return;
    } else if (!strcmp(name, "sprite_bitmap_write")) {
        char offset[64], value[64];
        parse_expression();
        new_temp(offset, sizeof offset);
        emit_op_label(OP_STA, offset);
        expect(T_COMMA, "','");
        parse_expression();
        new_temp(value, sizeof value);
        emit_op_label(OP_STA, value);
        expect(T_RPAREN, "')'");
        expect(T_SEMI, "';'");
        emit_op_label(OP_LDX, offset);
        emit_op_label(OP_LDA, value);
        emit_op(OP_STA, M_ABSX, 0x2000);
        return;
    } else if (!strcmp(name, "sprite_bitmap_clear")) {
        expect(T_RPAREN, "')'");
        expect(T_SEMI, "';'");
        char loop[32];
        new_label(loop, sizeof loop);
        emit_op(OP_LDX, M_IMM, 0);
        emit_op(OP_LDA, M_IMM, 0);
        emit_label(loop);
        emit_op(OP_STA, M_ABSX, 0x2000);
        emit_op(OP_INX, M_IMP, 0);
        emit_op(OP_TXA, M_IMP, 0);
        emit_op(OP_CMP, M_IMM, 63);
        emit_op_label(OP_BNE, loop);
        return;
    } else if ((!strcmp(name, "set_border") || !strcmp(name, "set_background")) &&
               tok.kind != T_RPAREN) {
        parse_expression();
        emit_op(OP_STA, M_ABS, !strcmp(name, "set_border") ? 0xD020 : 0xD021);
    } else if (!strcmp(name, "print")) {
        long address = parse_const();
        expect(T_COMMA, "','");
        if (tok.kind != T_STR) die("expected a string");
        char text[64];
        snprintf(text, sizeof text, "%s", tok.text);
        advance();
        if (address < 0 || address + (long)strlen(text) > 0x10000)
            die("print address out of range");
        for (size_t i = 0; text[i]; i++) {
            int c = (unsigned char)text[i];
            if (c >= 'a' && c <= 'z') c -= 'a' - 1;
            else if (c >= 'A' && c <= 'Z') c -= 'A' - 1;
            else if (c < 32 || c > 63) die("unsupported character in string");
            emit_op(OP_LDA, M_IMM, c);
            emit_op(OP_STA, M_ABS, (int)(address + (long)i));
        }
    } else {
        parse_function_call(name, 0);
        expect(T_SEMI, "';'");
        return;
    }
    expect(T_RPAREN, "')'");
    expect(T_SEMI, "';'");
}

static void parse_local_declaration(void) {
    parse_byte_type();
    do {
        int is_pointer = tok.kind == T_STAR;
        if (is_pointer) advance();
        if (tok.kind != T_IDENT) die("expected variable name");
        char name[64];
        snprintf(name, sizeof name, "%s", tok.text);
        advance();
        if (is_pointer) {
            add_pointer_symbol(name, 0);
            int index = (int)symbol_count - 1;
            if (tok.kind == T_ASSIGN) {
                advance();
                long address = parse_const();
                if (address < 0 || address > 0xFFFF) die("pointer address out of range (0..65535)");
                emit_op(OP_LDA, M_IMM, (int)address & 0xFF);
                emit_op_label(OP_STA, symbols[index].label);
                emit_op(OP_LDA, M_IMM, ((int)address >> 8) & 0xFF);
                emit_op_label(OP_STA, symbols[index].high_label);
            }
            if (tok.kind != T_COMMA) break;
            advance();
            continue;
        }
        size_t array_length = 1;
        int is_array = 0;
        if (tok.kind == T_LBRACKET) {
            is_array = 1;
            advance();
            long length = parse_const();
            if (length < 1 || length > 255) die("array length out of range (1..255)");
            array_length = (size_t)length;
            expect(T_RBRACKET, "']'");
        }
        add_symbol_array(name, 0, array_length, is_array);
        int index = (int)symbol_count - 1;
        if (tok.kind == T_ASSIGN) {
            if (is_array) die("array initialization is not supported");
            advance();
            parse_expression();
            emit_op_label(OP_STA, symbols[index].label);
        }
        if (tok.kind != T_COMMA) break;
        advance();
    } while (1);
    expect(T_SEMI, "';'");
}

static void parse_statement(void) {
    switch (tok.kind) {
    case T_LBRACE: parse_block(); break;
    case T_IF: parse_if(); break;
    case T_WHILE: parse_while(); break;
    case T_FOR: parse_for(); break;
    case T_BREAK:
        advance();
        expect(T_SEMI, "';'");
        if (!loop_depth) die("break used outside a loop");
        emit_op_label(OP_JMP, loops[loop_depth - 1].break_label);
        break;
    case T_CONTINUE:
        advance();
        expect(T_SEMI, "';'");
        if (!loop_depth) die("continue used outside a loop");
        emit_op_label(OP_JMP, loops[loop_depth - 1].continue_label);
        break;
    case T_STAR: {
        advance();
        if (tok.kind != T_IDENT) die("expected pointer name after '*'");
        int index = find_symbol(tok.text);
        if (index < 0 || !symbols[index].is_pointer)
            die("assignment through '*' requires a pointer variable");
        advance();
        expect(T_ASSIGN, "'='");
        parse_expression();
        char saved_value[64];
        new_temp(saved_value, sizeof saved_value);
        emit_op_label(OP_STA, saved_value);
        emit_pointer_setup(&symbols[index]);
        emit_op(OP_LDY, M_IMM, 0);
        emit_op_label(OP_LDA, saved_value);
        emit_op_indirect_y(OP_STA, 0xFB);
        expect(T_SEMI, "';'");
        break;
    }
    case T_RETURN:
        advance();
        {
            int function_index = find_function(current_function);
            int returns_value = function_index >= 0 && functions[function_index].returns_value;
            if (tok.kind == T_SEMI) {
                if (returns_value) die("value-returning function needs a return value");
                advance();
            } else {
                if (!returns_value) die("void function cannot return a value");
                parse_expression();
                expect(T_SEMI, "';'");
            }
        }
        emit_op(OP_RTS, M_IMP, 0);
        break;
    case T_IDENT: {
        char name[64];
        snprintf(name, sizeof name, "%s", tok.text);
        advance();
        if (tok.kind == T_LPAREN) {
            parse_call(name);
            break;
        }
        int index = find_symbol(name);
        if (index < 0) die("unknown variable");
        if (symbols[index].is_pointer) {
            if (tok.kind != T_ASSIGN) die("pointer supports only constant-address assignment");
            advance();
            long address = parse_const();
            if (address < 0 || address > 0xFFFF) die("pointer address out of range (0..65535)");
            emit_op(OP_LDA, M_IMM, (int)address & 0xFF);
            emit_op_label(OP_STA, symbols[index].label);
            emit_op(OP_LDA, M_IMM, ((int)address >> 8) & 0xFF);
            emit_op_label(OP_STA, symbols[index].high_label);
            expect(T_SEMI, "';'");
            break;
        }
        if (tok.kind == T_LBRACKET) {
            if (!symbols[index].is_array) die("subscripted value is not an array");
            advance();
            parse_expression();
            expect(T_RBRACKET, "']'");
            char saved_index[64], saved_value[64];
            new_temp(saved_index, sizeof saved_index);
            emit_op_label(OP_STA, saved_index);
            expect(T_ASSIGN, "'='");
            parse_expression();
            new_temp(saved_value, sizeof saved_value);
            emit_op_label(OP_STA, saved_value);
            emit_op_label(OP_LDX, saved_index);
            emit_op_label(OP_LDA, saved_value);
            emit_op_indexed_label(OP_STA, symbols[index].label);
            expect(T_SEMI, "';'");
            break;
        }
        if (symbols[index].is_array) die("array must be indexed");
        if (tok.kind == T_ASSIGN) {
            advance();
            parse_expression();
            emit_op_label(OP_STA, symbols[index].label);
            expect(T_SEMI, "';'");
        } else if (tok.kind == T_INC || tok.kind == T_DEC) {
            TokKind operation = tok.kind;
            advance();
            emit_op_label(OP_LDA, symbols[index].label);
            if (operation == T_INC) {
                emit_op(OP_CLC, M_IMP, 0);
                emit_op(OP_ADC, M_IMM, 1);
            } else {
                emit_op(OP_SEC, M_IMP, 0);
                emit_op(OP_SBC, M_IMM, 1);
            }
            emit_op_label(OP_STA, symbols[index].label);
            expect(T_SEMI, "';'");
        } else {
            die("expected assignment or function call");
        }
        break;
    }
    case T_SEMI: advance(); break;
    default: die("unexpected token in statement");
    }
}

static void parse_global_declaration(void) {
    parse_byte_type();
    do {
        int is_pointer = tok.kind == T_STAR;
        if (is_pointer) advance();
        if (tok.kind != T_IDENT) die("expected variable name");
        char name[64];
        snprintf(name, sizeof name, "%s", tok.text);
        advance();
        if (is_pointer) {
            unsigned int address = 0;
            if (tok.kind == T_ASSIGN) {
                advance();
                long value = parse_const();
                if (value < 0 || value > 0xFFFF) die("pointer address out of range (0..65535)");
                address = (unsigned int)value;
            }
            add_pointer_symbol(name, address);
            if (tok.kind != T_COMMA) break;
            advance();
            continue;
        }
        size_t array_length = 1;
        int is_array = 0;
        if (tok.kind == T_LBRACKET) {
            is_array = 1;
            advance();
            long length = parse_const();
            if (length < 1 || length > 255) die("array length out of range (1..255)");
            array_length = (size_t)length;
            expect(T_RBRACKET, "']'");
        }
        int initial_value = 0;
        if (tok.kind == T_ASSIGN) {
            if (is_array) die("array initialization is not supported");
            advance();
            long value = parse_const();
            if (value < 0 || value > 255) die("byte initializer out of range (0..255)");
            initial_value = (int)value;
        }
        add_symbol_array(name, initial_value, array_length, is_array);
        if (tok.kind != T_COMMA) break;
        advance();
    } while (1);
    expect(T_SEMI, "';'");
}

static int looks_like_function(void) {
    Lexer look = lx;
    Token next;
    if (tok.kind == T_UNSIGNED) {
        next = lex_next(&look);
        if (next.kind != T_CHAR && next.kind != T_INT) return 0;
        next = lex_next(&look);
    } else {
        next = lex_next(&look);
    }
    if (next.kind != T_IDENT) return 0;
    return lex_next(&look).kind == T_LPAREN;
}

static int parse_return_type(void) {
    if (tok.kind == T_VOID) {
        advance();
        return 0;
    }
    if (parse_byte_type()) return 1;
    die("expected function return type");
    return 0;
}

static void parse_function(void) {
    int returns_value = parse_return_type();
    if (tok.kind != T_IDENT) die("expected function name");
    char name[64], label[72];
    snprintf(name, sizeof name, "%s", tok.text);
    snprintf(label, sizeof label, "f_%s", tok.text);
    advance();
    expect(T_LPAREN, "'('");

    char param_names[MAX_PARAMS][64];
    size_t param_count = 0;
    if (tok.kind != T_RPAREN) {
        if (tok.kind == T_VOID) {
            advance();
            if (tok.kind != T_RPAREN) die("void must be the only parameter marker");
        } else {
            do {
                if (!parse_byte_type()) die("expected parameter type");
                if (tok.kind != T_IDENT) die("expected parameter name");
                if (param_count >= MAX_PARAMS) die("too many function parameters (maximum 8)");
                snprintf(param_names[param_count], sizeof param_names[param_count], "%s", tok.text);
                advance();
                param_count++;
                if (tok.kind != T_COMMA) break;
                advance();
            } while (1);
        }
    }
    expect(T_RPAREN, "')'");

    int function_index = get_function(name);
    Function *function = &functions[function_index];
    if (function->has_signature) {
        if (function->returns_value != returns_value || function->param_count != param_count)
            die("function declaration does not match earlier signature");
    } else {
        function->returns_value = returns_value;
        function->param_count = param_count;
        function->has_signature = 1;
        for (size_t i = 0; i < param_count; i++) {
            snprintf(function->param_names[i], sizeof function->param_names[i], "%s", param_names[i]);
            snprintf(function->param_labels[i], sizeof function->param_labels[i], "p_%d_%zu", function_index, i);
            add_slot(function->param_labels[i], 0);
        }
    }
    if (tok.kind == T_SEMI) {
        advance();
        return;
    }
    if (function->defined) die("duplicate function definition");
    function->defined = 1;
    snprintf(current_function, sizeof current_function, "%s", name);
    for (size_t i = 0; i < param_count; i++)
        add_parameter_symbol(param_names[i], function->param_labels[i]);
    emit_function_label(label, name);
    parse_block();
    emit_op(OP_RTS, M_IMP, 0);
    current_function[0] = '\0';
}

static void parse_function_call(const char *name, int want_value) {
    int index = find_function(name);
    if (index < 0) {
        if (want_value) die("function must be declared before use as an expression");
        index = get_function(name);
    }
    Function *function = &functions[index];
    if (want_value && function->has_signature && !function->returns_value)
        die("void function cannot be used as an expression");

    size_t argument = 0;
    if (tok.kind != T_RPAREN) {
        if (!function->has_signature) die("function needs a prototype before arguments are used");
        do {
            if (argument >= function->param_count) die("too many function arguments");
            parse_expression();
            emit_op_label(OP_STA, function->param_labels[argument]);
            argument++;
            if (tok.kind != T_COMMA) break;
            advance();
        } while (1);
    }
    if (function->has_signature && argument != function->param_count)
        die("wrong number of function arguments");
    expect(T_RPAREN, "')'");
    char label[72];
    snprintf(label, sizeof label, "f_%s", name);
    emit_op_label(OP_JSR, label);
}

void compile(const char *source) {
    emit_reset();
    emit_source_comments(source);
    symbol_count = slot_count = function_count = 0;
    loop_depth = 0;
    label_counter = temp_counter = symbol_counter = 0;
    array_counter = 0;
    pointer_counter = 0;
    current_function[0] = '\0';
    lex_init(&lx, source);
    advance();

    emit_op_label(OP_JSR, "f_main");
    emit_label("halt");
    emit_op_label(OP_JMP, "halt");

    while (tok.kind != T_EOF) {
        if (tok.kind == T_VOID || tok.kind == T_CHAR || tok.kind == T_INT ||
            tok.kind == T_UNSIGNED) {
            if (looks_like_function()) parse_function();
            else if (tok.kind != T_VOID) parse_global_declaration();
            else die("expected function declaration");
        }
        else
            die("expected function or global variable declaration");
    }
    for (size_t i = 0; i < slot_count; i++)
        emit_data_byte(slots[i].label, slots[i].value);
    emit_optimize();
}