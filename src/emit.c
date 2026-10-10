#include "emit.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define LOAD_ADDR 0x0801
#define CODE_ADDR 0x080D /* directly after the BASIC stub; "SYS 2061" */

static const struct {
    const char *name;
    int opcode[5]; /* indexed by Mode, -1 = invalid */
} OPS[OP_COUNT] = {
    [OP_LDA] = {"lda", {-1, 0xA9, 0xAD, 0xBD, 0xB1}},
    [OP_STA] = {"sta", {-1, -1, 0x8D, 0x9D, 0x91}},
    [OP_JMP] = {"jmp", {-1, -1, 0x4C, -1, -1}},
    [OP_JSR] = {"jsr", {-1, -1, 0x20, -1, -1}},
    [OP_RTS] = {"rts", {0x60, -1, -1, -1, -1}},
    [OP_ADC] = {"adc", {-1, 0x69, 0x6D, 0x7D, -1}},
    [OP_SBC] = {"sbc", {-1, 0xE9, 0xED, 0xFD, -1}},
    [OP_AND] = {"and", {-1, 0x29, 0x2D, 0x3D, -1}},
    [OP_ORA] = {"ora", {-1, 0x09, 0x0D, 0x1D, -1}},
    [OP_EOR] = {"eor", {-1, 0x49, 0x4D, 0x5D, -1}},
    [OP_CMP] = {"cmp", {-1, 0xC9, 0xCD, 0xDD, -1}},
    [OP_CLC] = {"clc", {0x18, -1, -1, -1, -1}},
    [OP_SEC] = {"sec", {0x38, -1, -1, -1, -1}},
    [OP_PHA] = {"pha", {0x48, -1, -1, -1, -1}},
    [OP_PLA] = {"pla", {0x68, -1, -1, -1, -1}},
    [OP_TAX] = {"tax", {0xAA, -1, -1, -1, -1}},
    [OP_TXA] = {"txa", {0x8A, -1, -1, -1, -1}},
    [OP_LDX] = {"ldx", {-1, 0xA2, 0xAE, -1, -1}},
    [OP_LDY] = {"ldy", {-1, 0xA0, -1, -1, -1}},
    [OP_INX] = {"inx", {0xE8, -1, -1, -1, -1}},
    [OP_ASL] = {"asl", {0x0A, -1, -1, -1, -1}},
    [OP_LSR] = {"lsr", {0x4A, -1, -1, -1, -1}},
    [OP_BEQ] = {"beq", {-1, -1, -1, -1, -1}},
    [OP_BNE] = {"bne", {-1, -1, -1, -1, -1}},
    [OP_BCC] = {"bcc", {-1, -1, -1, -1, -1}},
    [OP_BCS] = {"bcs", {-1, -1, -1, -1, -1}},
};

static const int BRANCH_OPCODE[OP_COUNT] = {
    [OP_BEQ] = 0xF0, [OP_BNE] = 0xD0,
    [OP_BCC] = 0x90, [OP_BCS] = 0xB0,
};

static const Op INVERSE_BRANCH[OP_COUNT] = {
    [OP_BEQ] = OP_BNE, [OP_BNE] = OP_BEQ,
    [OP_BCC] = OP_BCS, [OP_BCS] = OP_BCC,
};

typedef struct {
    char label[64]; /* label definition if non-empty */
    char function_name[64];
    int is_label;
    int is_function;
    Op op;
    Mode mode;
    int value;
    char target[64]; /* symbolic operand if non-empty */
    int is_data;
    int long_branch;
} Item;

static Item *items;
static size_t count, cap;
static char **source_comments;
static size_t source_comment_count, source_comment_cap;

static void die(const char *msg, const char *arg) {
    fprintf(stderr, "error: %s%s%s\n", msg, arg ? ": " : "", arg ? arg : "");
    exit(1);
}

static Item *push(void) {
    if (count == cap) {
        cap = cap ? cap * 2 : 64;
        items = realloc(items, cap * sizeof *items);
        if (!items) die("out of memory", NULL);
    }
    Item *it = &items[count++];
    memset(it, 0, sizeof *it);
    return it;
}

void emit_reset(void) {
    count = 0;
    for (size_t i = 0; i < source_comment_count; i++) free(source_comments[i]);
    source_comment_count = 0;
}

void emit_label(const char *name) {
    Item *it = push();
    it->is_label = 1;
    snprintf(it->label, sizeof it->label, "%s", name);
}

void emit_function_label(const char *label, const char *name) {
    Item *it = push();
    it->is_label = 1;
    it->is_function = 1;
    snprintf(it->label, sizeof it->label, "%s", label);
    snprintf(it->function_name, sizeof it->function_name, "%s", name);
}

static void save_source_comment(const char *start, size_t length) {
    if (source_comment_count == source_comment_cap) {
        source_comment_cap = source_comment_cap ? source_comment_cap * 2 : 16;
        char **comments = realloc(source_comments, source_comment_cap * sizeof *comments);
        if (!comments) die("out of memory", NULL);
        source_comments = comments;
    }
    char *comment = malloc(length + 1);
    if (!comment) die("out of memory", NULL);
    memcpy(comment, start, length);
    comment[length] = '\0';
    source_comments[source_comment_count++] = comment;
}

void emit_source_comments(const char *source) {
    for (size_t i = 0; source[i];) {
        if (source[i] == '"' || source[i] == '\'') {
            char quote = source[i++];
            while (source[i]) {
                if (source[i] == '\\' && source[i + 1]) i += 2;
                else if (source[i++] == quote) break;
            }
        } else if (source[i] == '/' && source[i + 1] == '/') {
            size_t start = i;
            i += 2;
            while (source[i] && source[i] != '\n') i++;
            save_source_comment(source + start, i - start);
        } else if (source[i] == '/' && source[i + 1] == '*') {
            size_t start = i;
            i += 2;
            while (source[i] && !(source[i] == '*' && source[i + 1] == '/')) i++;
            if (source[i]) i += 2;
            save_source_comment(source + start, i - start);
        } else {
            i++;
        }
    }
}

void emit_op(Op op, Mode mode, int value) {
    if (OPS[op].opcode[mode] < 0) die("invalid addressing mode for", OPS[op].name);
    Item *it = push();
    it->op = op;
    it->mode = mode;
    it->value = value;
}

void emit_op_label(Op op, const char *target) {
    if (op == OP_BEQ || op == OP_BNE || op == OP_BCC || op == OP_BCS) {
        Item *it = push();
        it->op = op;
        it->mode = M_ABS;
    } else {
        emit_op(op, M_ABS, 0);
    }
    snprintf(items[count - 1].target, sizeof items[count - 1].target, "%s", target);
}

void emit_op_indexed_label(Op op, const char *target) {
    if (OPS[op].opcode[M_ABSX] < 0) die("invalid indexed addressing mode for", OPS[op].name);
    Item *it = push();
    it->op = op;
    it->mode = M_ABSX;
    snprintf(it->target, sizeof it->target, "%s", target);
}

void emit_op_indirect_y(Op op, int zero_page_address) {
    if (OPS[op].opcode[M_INDY] < 0) die("invalid indirect indexed mode for", OPS[op].name);
    if (zero_page_address < 0 || zero_page_address > 0xFF)
        die("indirect pointer must be in zero page", NULL);
    Item *it = push();
    it->op = op;
    it->mode = M_INDY;
    it->value = zero_page_address;
}

void emit_data_byte(const char *label, int value) {
    Item *it = push();
    it->is_data = 1;
    it->value = value & 0xFF;
    snprintf(it->label, sizeof it->label, "%s", label);
}

static int is_op(const Item *it, Op op, Mode mode) {
    return !it->is_label && !it->is_data && it->op == op && it->mode == mode;
}

static void remove_items(size_t start, size_t length) {
    memmove(&items[start], &items[start + length], (count - start - length) * sizeof *items);
    count -= length;
}

void emit_optimize(void) {
    int changed;
    do {
        changed = 0;
        for (size_t i = 0; i < count; i++) {
            if (i + 1 < count && is_op(&items[i], OP_LDA, M_IMM) &&
                is_op(&items[i + 1], OP_EOR, M_IMM) && items[i + 1].value == 0xFF) {
                int value = items[i].value ^ 0xFF;
                size_t length = 2;
                if (i + 3 < count && is_op(&items[i + 2], OP_CLC, M_IMP) &&
                    is_op(&items[i + 3], OP_ADC, M_IMM) && items[i + 3].value == 1) {
                    value = (value + 1) & 0xFF;
                    length = 4;
                }
                items[i].value = value;
                remove_items(i + 1, length - 1);
                changed = 1;
                break;
            }
            if (i + 6 >= count || !is_op(&items[i], OP_LDA, M_IMM) ||
                !is_op(&items[i + 1], OP_STA, M_ABS) || !items[i + 1].target[0] ||
                !is_op(&items[i + 2], OP_LDA, M_IMM) ||
                !is_op(&items[i + 3], OP_STA, M_ABS) || !items[i + 3].target[0] ||
                !is_op(&items[i + 4], OP_LDA, M_ABS) ||
                strcmp(items[i + 1].target, items[i + 4].target)) continue;

            size_t operation_index = i + 5;
            Op operation;
            size_t length;
            if ((is_op(&items[operation_index], OP_CLC, M_IMP) ||
                 is_op(&items[operation_index], OP_SEC, M_IMP)) && i + 7 < count) {
                operation_index++;
                operation = items[operation_index].op;
                if ((operation != OP_ADC && operation != OP_SBC) ||
                    !is_op(&items[operation_index], operation, M_ABS) ||
                    strcmp(items[i + 3].target, items[operation_index].target)) continue;
                if ((operation == OP_ADC && !is_op(&items[i + 5], OP_CLC, M_IMP)) ||
                    (operation == OP_SBC && !is_op(&items[i + 5], OP_SEC, M_IMP))) continue;
                length = 8;
            } else {
                operation = items[operation_index].op;
                if ((operation != OP_AND && operation != OP_ORA && operation != OP_EOR) ||
                    !is_op(&items[operation_index], operation, M_ABS) ||
                    strcmp(items[i + 3].target, items[operation_index].target)) continue;
                length = 7;
            }

            int left = items[i].value & 0xFF;
            int right = items[i + 2].value & 0xFF;
            int result = operation == OP_ADC ? left + right :
                         operation == OP_SBC ? left - right :
                         operation == OP_AND ? left & right :
                         operation == OP_ORA ? left | right : left ^ right;
            items[i].value = result & 0xFF;
            remove_items(i + 1, length - 1);
            changed = 1;
            break;
        }
    } while (changed);
}

static int item_size(const Item *it) {
    if (it->is_label) return 0;
    if (it->is_data) return 1;
    if (BRANCH_OPCODE[it->op]) return it->long_branch ? 5 : 2;
        return it->mode == M_IMP ? 1 :
            it->mode == M_IMM || it->mode == M_INDY ? 2 : 3;
}

static int find_label(const char *name, const int *addr) {
    for (size_t i = 0; i < count; i++)
        if ((items[i].is_label || items[i].is_data) && !strcmp(items[i].label, name)) return addr[i];
    return -1;
}

/* Computes each item's address and checks that labels are unique. */
static int *layout(void) {
    int *addr = malloc((count + 1) * sizeof *addr);
    if (!addr) die("out of memory", NULL);
    int changed;
    do {
        int pc = CODE_ADDR;
        changed = 0;
        for (size_t i = 0; i < count; i++) {
            addr[i] = pc;
            pc += item_size(&items[i]);
            if (pc > 0x10000) die("program exceeds the 6502 address space", NULL);
        }
        addr[count] = pc;
        for (size_t i = 0; i < count; i++) {
            if (!BRANCH_OPCODE[items[i].op] || items[i].long_branch) continue;
            int target = find_label(items[i].target, addr);
            if (target < 0) die("undefined symbol", items[i].target);
            int offset = target - (addr[i] + 2);
            if (offset < -128 || offset > 127) {
                items[i].long_branch = 1;
                changed = 1;
            }
        }
    } while (changed);
    for (size_t i = 0; i < count; i++) {
        if (!items[i].is_label && !items[i].is_data) continue;
        const char *name = items[i].is_data ? items[i].label : items[i].label;
        if (!*name) die("empty label", NULL);
        for (size_t j = 0; j < i; j++) {
            if ((items[j].is_label || items[j].is_data) && !strcmp(items[j].label, name))
                die("duplicate label", name);
        }
    }
    return addr;
}

void emit_write_asm(FILE *f) {
    fprintf(f, "; generated by c64cc\n*= $%04X\n", LOAD_ADDR);
    fprintf(f, "  .word $080B\n  .word 10\n  .byte $9E\n  .text \"2061\"\n  .byte 0,0,0\n\n");
    for (size_t i = 0; i < source_comment_count; i++) {
        fputs("; ", f);
        for (const char *p = source_comments[i]; *p; p++) {
            fputc(*p, f);
            if (*p == '\n' && p[1]) fputs("; ", f);
        }
        fputc('\n', f);
    }
    if (source_comment_count) fputc('\n', f);
    for (size_t i = 0; i < count; i++) {
        const Item *it = &items[i];
        if (it->is_label) {
            if (it->is_function) fprintf(f, "; function: %s\n", it->function_name);
            fprintf(f, "%s:\n", it->label);
        } else if (it->is_data) {
            fprintf(f, "%s:\n  .byte $%02X\n", it->label, it->value);
        } else if (BRANCH_OPCODE[it->op]) {
            if (it->long_branch) {
                fprintf(f, "  %s .__branch_%zu\n  jmp %s\n.__branch_%zu:\n",
                        OPS[INVERSE_BRANCH[it->op]].name, i, it->target, i);
            } else {
                fprintf(f, "  %s %s\n", OPS[it->op].name, it->target);
            }
        } else if (it->mode == M_IMP) {
            fprintf(f, "  %s\n", OPS[it->op].name);
        } else if (it->mode == M_INDY) {
            fprintf(f, "  %s ($%02X),y\n", OPS[it->op].name, it->value);
        } else if (it->target[0]) {
            fprintf(f, "  %s %s%s\n", OPS[it->op].name, it->target,
                    it->mode == M_ABSX ? ",x" : "");
        } else {
            fprintf(f, "  %s %s$%0*X%s\n", OPS[it->op].name,
                    it->mode == M_IMM ? "#" : "", it->mode == M_IMM ? 2 : 4,
                    it->value, it->mode == M_ABSX ? ",x" : "");
        }
    }
}

void emit_write_prg(FILE *f) {
    int *addr = layout();

    /* 2-byte load address, then a BASIC line: 10 SYS 2061 */
    static const uint8_t header[] = {
        LOAD_ADDR & 0xFF, LOAD_ADDR >> 8,
        0x0B, 0x08, 0x0A, 0x00, 0x9E, '2', '0', '6', '1', 0x00, 0x00, 0x00,
    };
    fwrite(header, 1, sizeof header, f);

    for (size_t i = 0; i < count; i++) {
        const Item *it = &items[i];
        if (it->is_label) continue;
        if (it->is_data) {
            fputc(it->value, f);
            continue;
        }

        int value = it->value;
        if (it->target[0]) {
            value = find_label(it->target, addr);
            if (value < 0) die("undefined symbol", it->target);
        }
        if (BRANCH_OPCODE[it->op]) {
            if (it->long_branch) {
                fputc(BRANCH_OPCODE[INVERSE_BRANCH[it->op]], f);
                fputc(3, f);
                fputc(0x4C, f);
                fputc(value & 0xFF, f);
                fputc((value >> 8) & 0xFF, f);
            } else {
                int offset = value - (addr[i] + 2);
                fputc(BRANCH_OPCODE[it->op], f);
                fputc(offset & 0xFF, f);
            }
            continue;
        }
        fputc(OPS[it->op].opcode[it->mode], f);
        if (it->mode == M_IMM) {
            fputc(value & 0xFF, f);
        } else if (it->mode == M_INDY) {
            fputc(value & 0xFF, f);
        } else if (it->mode == M_ABS || it->mode == M_ABSX) {
            fputc(value & 0xFF, f);
            fputc((value >> 8) & 0xFF, f);
        }
    }
    free(addr);
}
