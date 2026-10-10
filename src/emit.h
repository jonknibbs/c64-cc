#ifndef EMIT_H
#define EMIT_H

#include <stdio.h>

typedef enum { OP_LDA, OP_STA, OP_JMP, OP_JSR, OP_RTS, OP_COUNT } Op;
typedef enum { M_IMP, M_IMM, M_ABS } Mode;

void emit_reset(void);
void emit_label(const char *name);
void emit_op(Op op, Mode mode, int value);
void emit_op_label(Op op, const char *target);

/* Human-readable assembly listing. */
void emit_write_asm(FILE *f);
/* C64 .prg: load address, BASIC "SYS" stub, then the code. */
void emit_write_prg(FILE *f);

#endif
