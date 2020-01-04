#include "compile.h"

static int op_to_num_args(opcode_t op) {
    switch (op) {
        case OP_ADD:
        case OP_MOV:
        case OP_CMP:
            return 2;
        case OP_NEG:
        case OP_NOT:
        case OP_SETE:
            return 1;
        case OP_RET:
        default:
            return 0;
    }
}

static output_t *new_instr(opcode_t op) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;
    out->type = OUTPUT_INSTR;
    out->instr.op = op;
    out->instr.num_args = op_to_num_args(op);
    return out;
}

// TODO how to not put everything into eax
static list_t *expr_to_instrs(expr_t *expr) {
    if (!expr)
        return NULL;

    list_t *ret = list_new();
    operand_t rax = {.type = OPERAND_REG, .reg = REG_RAX};

    // int literal just does a movq into dst reg
    if (expr->type == INT_LITERAL) {
        output_t *mov = new_instr(OP_MOV);
        mov->instr.src.type = OPERAND_IMM;
        mov->instr.src.imm = expr->integer;
        mov->instr.dst = rax;
        list_push(ret, mov);
        return ret;
    }

    if (expr->type == UNARY_OP) {
        list_t *inner_instrs = expr_to_instrs(expr->unary->expr);
        if (!inner_instrs)
            return NULL;
        list_concat(ret, inner_instrs);
        if (expr->unary->op == UNARY_MATH_NEG) {
            output_t *neg = new_instr(OP_NEG);
            neg->instr.src = rax;
            list_push(ret, neg);
        } else if (expr->unary->op == UNARY_LOGICAL_NEG) {
            output_t *cmp = new_instr(OP_CMP);
            cmp->instr.src.type = OPERAND_IMM;
            cmp->instr.src.imm = 0;
            cmp->instr.dst = rax;
            list_push(ret, cmp);

            output_t *mov = new_instr(OP_MOV);
            mov->instr.src.type = OPERAND_IMM;
            mov->instr.src.imm = 0;
            mov->instr.dst = rax;
            list_push(ret, mov);

            output_t *sete = new_instr(OP_SETE);
            sete->instr.src.type = OPERAND_REG;
            sete->instr.src.reg = REG_AL;
            list_push(ret, sete);
        } else if (expr->unary->op == UNARY_BITWISE_COMP) {
            output_t *not = new_instr(OP_NOT);
            not->instr.src = rax;
            list_push(ret, not);
        } else {
            UNREACHABLE("unhandled unary op type\n");
            return NULL;
        }
        return ret;
    }

    UNREACHABLE("unhandled expression\n");
    return NULL;
}

// TODO does each statement correspond to an instruction?
static list_t *stmt_to_instrs(stmt_t *stmt) {
    if (!stmt)
        return NULL;
    list_t *ret = list_new();
    if (stmt->type == STMT_RETURN) {
        list_t *expr_instrs = expr_to_instrs(stmt->ret->expr);
        if (!expr_instrs)
            return NULL;
        list_concat(ret, expr_instrs);

        output_t *retq = new_instr(OP_RET);
        list_push(ret, retq);
        return ret;
    }
    UNREACHABLE("WTF NOT A RETURN STATEMENT\n");
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

static const reg_pair_t reg_pairs[] = {
    {.reg = REG_RAX, .string = "%rax"},
    {.reg = REG_AL, .string = "%al"},
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

static const op_pair_t op_pairs[] = {
    {.op = OP_MOV, .string = "movq"},
    {.op = OP_ADD, .string = "addq"},
    {.op = OP_RET, .string = "retq"},
    {.op = OP_CMP, .string = "cmpq"},
    {.op = OP_SETE, .string = "sete"},
    {.op = OP_NEG, .string = "negq"},
    {.op = OP_NOT, .string = "notq"},
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

    string_t string;
    char buf[100];
    if (string_init(&string) < 0)
        return NULL;
    
    if (operand.type == OPERAND_IMM) {
        snprintf(buf, 100, "$%ld", operand.imm);
    } else if (operand.type == OPERAND_MEM_LOC) {
        snprintf(buf, 100, "$%ld(%s)", operand.mem.offset, reg_to_string(operand.mem.reg));
    } else {
        printf("trying to print a VAR. uncool man\n");
        exit(-1);
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
            printf(".globl %s\n", string_get(curr->label));
            printf("%s:\n", string_get(curr->label));
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
