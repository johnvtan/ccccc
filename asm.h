#ifndef ASM_H  
#define ASM_H

#include <stdint.h>
#include <stdio.h>
#include "ast.h" // for var_t

typedef enum {
    OP_MOV = 1,
    OP_ADD,
    OP_RET,
    OP_CMP,
    OP_SETE,
    OP_NEG,
    OP_NOT,
    OP_PUSH,
    OP_POP,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_XCHG,
    OP_CQO,
} opcode_t;

typedef enum {
    REG_RAX = 1,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RBP,
    REG_RSP,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15,
    REG_AL, // for sete
} reg_t;

typedef struct {
    // register containing base address of memory location
    reg_t reg;

    // offset from the value in reg
    int64_t offset;
} mem_loc_t;

typedef int64_t imm_t;

typedef enum {
    OPERAND_REG,
    OPERAND_MEM_LOC,
    OPERAND_VAR,
    OPERAND_IMM,
} operand_type_t;

typedef struct {
    operand_type_t type;
    union {
        reg_t reg;
        mem_loc_t mem;
        var_t var;
        imm_t imm;
    };
} operand_t;

// TODO do all instructions have only one or two operands?
typedef struct {
    opcode_t op;
    int num_args;
    operand_t src;
    operand_t dst;
} instr_t;

typedef enum {
    OUTPUT_INSTR,
    OUTPUT_LABEL,
} output_type_t;

typedef struct {
    output_type_t type;
    union {
        instr_t instr;
        string_t *label;
    };
} output_t;

list_t *gen_pseudo_asm(program_t *prog);
list_t *gen_real_asm(program_t *prog);
void print_asm(list_t *instrs);
#endif
