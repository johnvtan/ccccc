#include "compile.h"

typedef struct reg_pair {
    reg_t reg;
    char *string;
} reg_pair_t;

static const reg_pair_t reg_pairs[] = {
    {.reg = REG_RAX, .string = "%rax"},
    {.reg = REG_RCX, .string = "%rcx"},
    {.reg = REG_AL, .string = "%al"},
    {.reg = REG_RBP, .string = "%rbp"},
    {.reg = REG_RSP, .string = "%rsp"},
    {0, NULL},
};

static char *reg_to_string(reg_t reg) {
    int i = 0;
    while (reg_pairs[i].string != NULL) {
        if (reg == reg_pairs[i].reg) {
            return reg_pairs[i].string;
        }
        i++;
    }
    debug("reg_to_string failed: reg = %d\n", (int)reg);
    return NULL;
}

typedef struct op_pair {
    opcode_t op;
    char *string;
} op_pair_t;

static const op_pair_t op_pairs[] = {
    {.op = OP_MOV, .string = "movq"},
    {.op = OP_RET, .string = "retq"},
    {.op = OP_CMP, .string = "cmpq"},
    {.op = OP_SETE, .string = "sete"},
    {.op = OP_NEG, .string = "negq"},
    {.op = OP_NOT, .string = "notq"},
    {.op = OP_ADD, .string = "addq"},
    {.op = OP_SUB, .string = "subq"},
    {.op = OP_MUL, .string = "imulq"},
    {.op = OP_DIV, .string = "idivq"},
    {.op = OP_PUSH, .string = "pushq"},
    {.op = OP_POP, .string = "popq"},
    {.op = OP_XCHG, .string = "xchg"},
    {.op = OP_CQO, .string = "cqo"},
    {.op = OP_SETNE, .string = "setne"},
    {.op = OP_SETL, .string = "setl"},
    {.op = OP_SETLE, .string = "setle"},
    {.op = OP_SETG, .string = "setg"},
    {.op = OP_SETGE, .string = "setge"},
    {.op = OP_JMP, .string = "jmp"},
    {.op = OP_JE, .string = "je"},
    {.op = OP_JNE, .string = "jne"},
    {0, NULL},
};

static char *op_to_string(opcode_t op) {
    int i = 0;
    while (op_pairs[i].string != NULL) {
        if (op == op_pairs[i].op) {
            return op_pairs[i].string;
        }
        i++;
    }
    return NULL;
}

static char *operand_to_string(operand_t operand) {
    if (operand.type == OPERAND_REG) {
        return reg_to_string(operand.reg);
    }

    if (operand.type == OPERAND_LABEL) {
        return string_get(operand.label);
    }

    if (operand.type == OPERAND_VAR) {
        UNREACHABLE("VARs should not be in output\n");
    }

    string_t string;
    char buf[100];
    if (string_init(&string) < 0)
        return NULL;
    
    if (operand.type == OPERAND_IMM) {
        snprintf(buf, 100, "$%ld", operand.imm);
    } else if (operand.type == OPERAND_MEM_LOC) {
        snprintf(buf, 100, "%ld(%s)", operand.mem.offset, reg_to_string(operand.mem.reg));
    }
    string_append(&string, buf, strlen(buf));
    return string_get(&string);
}

void print_asm(list_t *output) {
    if (!output)
        return;

    output_t *curr = list_pop(output);
    for (; curr; curr = list_pop(output)) {
        if (curr->type == OUTPUT_LABEL) {
            if (curr->label.linkage == LABEL_GLOBAL) {
                printf(".globl %s\n", string_get(curr->label.name));
            }
            printf("%s:\n", string_get(curr->label.name));
            continue;
        }

        if (curr->type == OUTPUT_INSTR) {
            instr_t instr = curr->instr;
            if (instr.num_args == 2) {
                printf("\t%s %s, %s\n", op_to_string(instr.op), operand_to_string(instr.src), operand_to_string(instr.dst));
            } else if (instr.num_args == 1) {
                printf("\t%s %s\n", op_to_string(instr.op), operand_to_string(instr.src));
            } else if (instr.num_args == 0) {
                printf("\t%s\n", op_to_string(instr.op));
            } else {
                printf("BAD NUM ARGS\n");
                exit(-1);
            }
            continue;
        }
    }
}
