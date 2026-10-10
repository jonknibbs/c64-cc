#ifndef EMIT_H
#define EMIT_H

#include <stdio.h>

typedef enum {
	OP_LDA, OP_STA, OP_JMP, OP_JSR, OP_RTS,
	OP_ADC, OP_SBC, OP_AND, OP_ORA, OP_EOR, OP_CMP,
	OP_CLC, OP_SEC, OP_PHA, OP_PLA, OP_TAX, OP_TXA, OP_LDX, OP_LDY, OP_INX, OP_ASL, OP_LSR,
	OP_BEQ, OP_BNE, OP_BCC, OP_BCS, OP_COUNT
} Op;
typedef enum { M_IMP, M_IMM, M_ABS, M_ABSX, M_INDY } Mode;

void emit_reset(void);
void emit_label(const char *name);
void emit_function_label(const char *label, const char *name);
void emit_origin(int address);
void emit_source_comments(const char *source);
void emit_op(Op op, Mode mode, int value);
void emit_op_label(Op op, const char *target);
void emit_op_indexed_label(Op op, const char *target);
void emit_op_indirect_y(Op op, int zero_page_address);
void emit_data_byte(const char *label, int value);
void emit_optimize(void);

/* Human-readable assembly listing. */
void emit_write_asm(FILE *f);
/* C64 .prg: load address, BASIC "SYS" stub, then the code. */
void emit_write_prg(FILE *f);

#endif
