#ifndef ASM_H  
#define ASM_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "asm_types.h"
#include "env.h"

typedef enum {
    OPERAND_REG,
    OPERAND_MEM_LOC,
    OPERAND_VAR,
    OPERAND_IMM,
    OPERAND_LABEL,
} operand_type_t;

typedef struct {
    string_t *name; 
    enum {
        LABEL_STATIC,
        LABEL_GLOBAL,
    } linkage;
} label_t;

typedef struct {
    operand_type_t type;
    union {
        reg_t reg;
        mem_loc_t mem;
        //var_t var;
        imm_t imm;
        string_t *label;
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
        label_t label;
    };
} output_t;

typedef struct {
    string_t *return_label;
    string_t *iter_start_label;
    string_t *iter_end_label;
    env_t *env;
} context_t;

#endif
