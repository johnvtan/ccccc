#ifndef ASM_TYPES_H
#define ASM_TYPES_H

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
    OP_SETNE,
    OP_SETL,
    OP_SETLE,
    OP_SETG,
    OP_SETGE,
    OP_JMP,
    OP_JE,
    OP_JNE,
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

#endif
