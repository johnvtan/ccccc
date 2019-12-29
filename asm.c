#include "compile.h"

// TODO does each statement correspond to an instruction?
static list_t *stmt_to_instrs(stmt_t *stmt) {
    if (!stmt)
        return NULL;
    list_t *ret = list_new();
    if (stmt->type == STMT_RETURN) {
        output_t *out = malloc(sizeof(output_t));  
        out->type = OUTPUT_INSTR;
        out->instr = malloc(sizeof(instr_t));
        out->instr->op = OP_MOV;

        // TODO only handle integers
        if (stmt->ret->expr->type != INT_LITERAL)
            return NULL;

        out->instr->src.imm = stmt->ret->expr->integer;
        out->instr->dst.reg = REG_RAX;
        list_push(ret, out);

        output_t *retq = malloc(sizeof(output_t));
        out->type = OUTPUT_INSTR;
        out->instr = malloc(sizeof(instr_t));
        out->instr->op = OP_RETQ;
        list_push(ret, retq);
        return ret;
    }
    UNREACHABLE("WTF\n");
    return NULL;
} 

// transforms an fn_def_t ast node to a list of pseudo-x86 instructions
static list_t *fn_def_to_pseudo_asm(fn_def_t *fn_def) {
    if (!fn_def)
        return NULL;
    if (!fn_def->name)
        return NULL;
    if (!fn_def->stmts)
        return NULL;

    // TODO totally skipping params now
    // TODO type checking on return type, but also skipping that for now 
    list_t *ret = list_new();
    output_t *fn_label = malloc(sizeof(output_t));
    fn_label->type = OUTPUT_LABEL;
    fn_label->label = fn_def->name;
    list_push(ret, fn_label);
    
    stmt_t *curr_stmt = list_pop(fn_def->stmts);
    for (; curr_stmt; curr_stmt = list_pop(fn_def->stmts)) {
        list_t *stmt_instrs = stmt_to_instrs(curr_stmt);         
        if (!stmt_instrs)
            return NULL;
        list_concat(ret, stmt_instrs);
    }

    return ret;
}

list_t *gen_pseudo_asm(program_t *prog) {
    if (!prog || !prog->fn_defs)
        return NULL;
    list_t *output = list_new();
    fn_def_t *fn_def = list_pop(prog->fn_defs);
    for (; fn_def; fn_def = list_pop(prog->fn_defs)) {
        list_t *fn_instrs = fn_def_to_pseudo_asm(fn_def);
        if (!fn_instrs)
            return NULL;

        list_concat(output, fn_instrs);
    }
    return output;
}

typedef struct reg_pair {
    reg_t reg;
    char *string;
} reg_pair_t;

static const reg_pair_t reg_pairs[2] = {
    {.reg = REG_RAX, .string = "%rax"},
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
    return NULL;
}

typedef struct op_pair {
    opcode_t op;
    char *string;
} op_pair_t;

static const op_pair_t op_pairs[3] = {
    {.op = OP_MOV, .string = "movq"},
    {.op = OP_RETQ, .string = "retq"},
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


void print_asm(list_t *output) {
    if (!output)
        return;

    output_t *curr = list_pop(output);
    for (; curr; curr = list_pop(output)) {
        if (curr->type == OUTPUT_LABEL) {
            printf("%s:\n", string_get(curr->label));
            continue;
        }

        if (curr->type == OUTPUT_INSTR) {
            instr_t *instr = curr->instr;
            if (instr->op == OP_MOV) {
                printf("\t%s $%lu, %s\n", op_to_string(instr->op), instr->src.imm, reg_to_string(instr->dst.reg));
            } else if (instr->op == OP_RETQ) {
                printf("retq\n");
            }
            continue;
        }
    }

}
