#ifndef ASM_H  
#define ASM_H

typedef enum opcode {
    OP_MOV,
    OP_ADD,
} opcode_t;

typedef enum reg {
    REG_RAX,
    REG_RBX,
    REG_RCX,
    REG_RDX,
    REG_RBP,
    REG_RSP,
    REG_RDI,
    REG_RBI,
    // TODO actually fill this with x86 regs
} reg_t;

// IR is like x86, but with variables instead of registers
typedef struct var {
    type_t type;
    string_t name;
} var_t;

typedef uint64_t imm_t;

typedef union operand {
    reg_t reg;
    var_t var;
    imm_t imm;
} operand_t;

typedef struct instr {
    opcode_t op;
    operand_t r1;
    operand_t r2;
    operand_t dst;
} instr_t;

typedef union output {
    instr_t instr;
    string_t label;
} output_t;

#endif
