#include "compile.h"

static map_t *variable_homes;
static list_t *expr_to_instrs(expr_t *expr, env_t *env);

static int op_to_num_args(opcode_t op) {
    switch (op) {
        case OP_ADD:
        case OP_MOV:
        case OP_CMP:
        case OP_SUB:
        case OP_MUL:
        case OP_XCHG:
            return 2;
        case OP_NEG:
        case OP_NOT:
        case OP_SETE:
        case OP_PUSH:
        case OP_POP:
        case OP_DIV:
        case OP_SETNE:
        case OP_SETL:
        case OP_SETLE:
        case OP_SETG:
        case OP_SETGE:
        case OP_JMP:
        case OP_JE:
        case OP_JNE:
            return 1;
        case OP_RET:
        case OP_CQO:
        default:
            return 0;
    }
}

static string_t *unique_label(char *seed) {
    static int count = 0;
    char buf[32];

    string_t *name = string_new();
    string_append(name, seed, strnlen(seed, 100));

    snprintf(buf, 32, "%d", count++);
    string_append(name, buf, strnlen(buf, 32));
    return name;
}

static output_t *new_label(string_t *name, int linkage) {
    output_t *ret = malloc(sizeof(output_t));
    ret->type = OUTPUT_LABEL;
    ret->label.name = name;
    ret->label.linkage = linkage;
    return ret;
}

static output_t *instr_r2r(opcode_t op, reg_t src, reg_t dst) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2)
        return NULL;

    out->type = OUTPUT_INSTR;
    out->instr.op = op;
    out->instr.src.type = OPERAND_REG;
    out->instr.src.reg = src;

    out->instr.dst.type = OPERAND_REG;
    out->instr.dst.reg = dst;
    return out;
}

static output_t *instr_i2r(opcode_t op, imm_t src, reg_t dst) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2)
        return NULL;

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_IMM;
    out->instr.src.imm = src;

    out->instr.dst.type = OPERAND_REG;
    out->instr.dst.reg = dst;
    return out;
}

static output_t *instr_r2v(opcode_t op, reg_t src, var_t dst) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2)
        return NULL;

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_REG;
    out->instr.src.reg = src;

    out->instr.dst.type = OPERAND_VAR;
    out->instr.dst.var = dst;
    return out;
}

static output_t *instr_v2r(opcode_t op, var_t src, reg_t dst) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2)
        return NULL;

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_VAR;
    out->instr.src.var = src;

    out->instr.dst.type = OPERAND_REG;
    out->instr.dst.reg = dst;
    return out;
}

// For IR
static output_t *instr_v2v(opcode_t op, var_t src, var_t dst) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2)
        return NULL;

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_VAR;
    out->instr.src.var = src;

    out->instr.dst.type = OPERAND_VAR;
    out->instr.dst.var = dst;
    return out;
}

static output_t *instr_i2v(opcode_t op, imm_t src, var_t dst) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2)
        return NULL;

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_IMM;
    out->instr.src.imm = src;

    out->instr.dst.type = OPERAND_VAR;
    out->instr.dst.var = dst;
    return out;
}

static output_t *instr_r(opcode_t op, reg_t src) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 1)
        return NULL;

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_REG;
    out->instr.src.reg = src;
    return out;
}

static output_t *instr_v(opcode_t op, var_t src) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 1)
        return NULL;

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_VAR;
    out->instr.src.var = src;
    return out;
}

static output_t *instr_jmp(opcode_t op, string_t *label) {
    if (op != OP_JMP && op != OP_JE && op != OP_JNE)
        return NULL;

    if (!label)
        return NULL;

    output_t *jmp = malloc(sizeof(output_t));
    jmp->type = OUTPUT_INSTR;
 
    jmp->instr.num_args = op_to_num_args(op);
    if (jmp->instr.num_args != 1)
        return NULL;

    jmp->instr.op = op;
    jmp->instr.src.type = OPERAND_LABEL;
    jmp->instr.src.label = label;
    return jmp;
}

static output_t *instr(opcode_t op) {
    output_t *out = malloc(sizeof(output_t));
    if (!out)
        return NULL;

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 0)
        return NULL;

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    return out;
}

static list_t *primary_to_instrs(primary_t *primary, env_t *env) {
    if (!primary)
        return NULL;

    if (primary->type == PRIMARY_INT) {
        list_t *ret = list_new();
        list_push(ret, instr_i2r(OP_MOV, primary->integer, REG_RAX));
        return ret;
    }

    if (primary->type == PRIMARY_EXPR) {
        return expr_to_instrs(primary->expr, env);
    }

    if (primary->type == PRIMARY_VAR) {
        list_t *ret = list_new();
        var_t var;
        var.name = primary->var;
        var.type = *(builtin_type_t*)map_get(env->map, var.name);
        list_push(ret, instr_v2r(OP_MOV, var, REG_RAX));
        return ret;
    }

    UNREACHABLE("Unexpected primary expr\n");
    return NULL;
}

static list_t *unary_to_instrs(unary_expr_t *unary, env_t *env) {
    list_t *ret = expr_to_instrs(unary->expr, env);
    if (!ret)
        return NULL;

    if (unary->op == UNARY_MATH_NEG) {
        list_push(ret, instr_r(OP_NEG, REG_RAX));
        return ret;
    } 

    if (unary->op == UNARY_LOGICAL_NEG) {
        output_t *cmp = instr_i2r(OP_CMP, 0, REG_RAX);
        list_push(ret, cmp);

        output_t *mov = instr_i2r(OP_MOV, 0, REG_RAX);
        list_push(ret, mov);

        output_t *sete = instr_r(OP_SETE, REG_AL);
        list_push(ret, sete);
        return ret;
    }

    if (unary->op == UNARY_BITWISE_COMP) {
        output_t *not = instr_r(OP_NOT, REG_RAX);
        list_push(ret, not);
        return ret;
    }

    UNREACHABLE("unexpected unary expr\n");
    return NULL;
}

static list_t *binop_to_instrs(bin_expr_t *bin, env_t *env) {
    if (!bin)
        return NULL;
    list_t *ret = expr_to_instrs(bin->lhs, env);
    if (!ret)
        return NULL;

    // AND and OR short circuit, so we don't want to evaluate the RHS if we're not certain we need
    // to.
    if (bin->op == BIN_OR) {
        string_t *or_clause_2 = unique_label("second_or_clause");
        string_t *end = unique_label("or_end");

        output_t *cmp = instr_i2r(OP_CMP, 0, REG_RAX);
        list_push(ret, cmp);

        list_push(ret, instr_jmp(OP_JE, or_clause_2));

        output_t *mov = instr_i2r(OP_MOV, 1, REG_RAX);
        list_push(ret, mov);
        list_push(ret, instr_jmp(OP_JMP, end));
        
        output_t *clause_2_label = new_label(or_clause_2, LABEL_STATIC);
        list_push(ret, clause_2_label);

        list_t *rhs = expr_to_instrs(bin->rhs, env);
        list_concat(ret, rhs);

        output_t *end_label = new_label(end, LABEL_STATIC);
        list_push(ret, end_label);
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETNE, REG_AL));
        return ret;
    }

    if (bin->op == BIN_AND) {
        string_t *and_clause_2 = unique_label("second_and_clause");
        string_t *end = unique_label("and_end");

        output_t *cmp = instr_i2r(OP_CMP, 0, REG_RAX);
        list_push(ret, cmp);

        list_push(ret, instr_jmp(OP_JNE, and_clause_2));
        list_push(ret, instr_jmp(OP_JMP, end));
        list_push(ret, new_label(and_clause_2, LABEL_STATIC));
        list_concat(ret, expr_to_instrs(bin->rhs, env));
        list_push(ret, new_label(end, LABEL_STATIC));
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETNE, REG_AL));
        return ret;
    }

    output_t *push = instr_r(OP_PUSH, REG_RAX);
    list_push(ret, push);

    list_concat(ret, expr_to_instrs(bin->rhs, env));
    output_t *pop = instr_r(OP_POP, REG_RCX);
    list_push(ret, pop);

    if (bin->op == BIN_ADD) {
        output_t *add = instr_r2r(OP_ADD, REG_RCX, REG_RAX);
        list_push(ret, add);
        return ret;
    }

    if (bin->op == BIN_SUB) {
        // sub src, dst computes dst - src
        // so we want to compute e1 - e2, and e1 is in rcx
        // so we'll do rcx - rax, then move rcx into rax
        output_t *sub = instr_r2r(OP_SUB, REG_RAX, REG_RCX);
        list_push(ret, sub);
        
        output_t *mov = instr_r2r(OP_MOV, REG_RCX, REG_RAX);
        list_push(ret, mov);

        return ret;
    }

    if (bin->op == BIN_MUL) {
        output_t *mul = instr_r2r(OP_MUL, REG_RCX, REG_RAX);
        list_push(ret, mul);
        return ret;
    }

    if (bin->op == BIN_DIV) {
        output_t *xchg = instr_r2r(OP_XCHG, REG_RAX, REG_RCX);
        list_push(ret, xchg);

        output_t *cqo = instr(OP_CQO);
        list_push(ret, cqo);

        output_t *div = instr_r(OP_DIV, REG_RCX);
        list_push(ret, div);
        return ret;
    } 

    if (bin->op == BIN_EQ) {
        output_t *cmp = instr_r2r(OP_CMP, REG_RAX, REG_RCX);
        list_push(ret, cmp);

        output_t *mov = instr_i2r(OP_MOV, 0, REG_RAX);
        list_push(ret, mov);

        output_t *sete = instr_r(OP_SETE, REG_AL);
        list_push(ret, sete);
        return ret;
    }

    if (bin->op == BIN_NE) {
        output_t *cmp = instr_r2r(OP_CMP, REG_RAX, REG_RCX);
        list_push(ret, cmp);

        output_t *mov = instr_i2r(OP_MOV, 0, REG_RAX);
        list_push(ret, mov);

        output_t *setne = instr_r(OP_SETNE, REG_AL);
        list_push(ret, setne);
        return ret;
    }

    if (bin->op == BIN_LT) {
        output_t *cmp = instr_r2r(OP_CMP, REG_RAX, REG_RCX);
        list_push(ret, cmp);

        output_t *mov = instr_i2r(OP_MOV, 0, REG_RAX);
        list_push(ret, mov);

        output_t *setb = instr_r(OP_SETL, REG_AL);
        list_push(ret, setb);
        return ret;
    }

    if (bin->op == BIN_LTE) {
        output_t *cmp = instr_r2r(OP_CMP, REG_RAX, REG_RCX);
        list_push(ret, cmp);

        output_t *mov = instr_i2r(OP_MOV, 0, REG_RAX);
        list_push(ret, mov);

        output_t *setbe = instr_r(OP_SETLE, REG_AL);
        list_push(ret, setbe);
        return ret;
    }

    if (bin->op == BIN_GT) {
        output_t *cmp = instr_r2r(OP_CMP, REG_RAX, REG_RCX);
        list_push(ret, cmp);

        output_t *mov = instr_i2r(OP_MOV, 0, REG_RAX);
        list_push(ret, mov);

        output_t *seta = instr_r(OP_SETG, REG_AL);
        list_push(ret, seta);
        return ret;
    }

    if (bin->op == BIN_GTE) {
        output_t *cmp = instr_r2r(OP_CMP, REG_RAX, REG_RCX);
        list_push(ret, cmp);

        output_t *mov = instr_i2r(OP_MOV, 0, REG_RAX);
        list_push(ret, mov);

        output_t *setae = instr_r(OP_SETGE, REG_AL);
        list_push(ret, setae);
        return ret;
    }
    UNREACHABLE("Unknown binary op\n");
    return NULL;
}

// I think I can use scoping instead of uniquifying to get this right...
static list_t *assign_to_instrs(assign_t *assign, env_t *env) {
    list_t *ret = list_new();
    return ret;
}


// TODO how to not put everything into eax
static list_t *expr_to_instrs(expr_t *expr, env_t *env) {
    if (!expr)
        return NULL;

    if (expr->type == PRIMARY) {
        return primary_to_instrs(expr->primary, env);
    }

    if (expr->type == UNARY_OP) {
        return unary_to_instrs(expr->unary, env);
    }

    if (expr->type == BIN_OP) {
        return binop_to_instrs(expr->bin, env);
    }

    if (expr->type == ASSIGN) {
        return assign_to_instrs(expr->assign, env);
    }

    UNREACHABLE("unhandled expression\n");
    return NULL;
}

// TODO does each statement correspond to an instruction?
// This should probably return a (map? env?) as well.
static list_t *stmt_to_instrs(stmt_t *stmt, env_t *env) {
    if (!stmt)
        return NULL;
    list_t *ret = list_new();
    if (stmt->type == STMT_RETURN) {
        debug("Found return statement\n");
        list_t *expr_instrs = expr_to_instrs(stmt->ret->expr, env);
        if (!expr_instrs)
            return NULL;
        list_concat(ret, expr_instrs);

        output_t *retq = instr(OP_RET);
        list_push(ret, retq);
        return ret;
    }

    if (stmt->type == STMT_DECLARE) {
        debug("Found a declare statement\n");
        if (stmt->declare->init_expr) {
            list_concat(ret, expr_to_instrs(stmt->declare->init_expr, env));
            var_t var;
            var.name = stmt->declare->name;
            var.type = stmt->declare->type;
            output_t *mov = instr_r2v(OP_MOV, REG_RAX, var);
            list_push(ret, mov);
        }
        return ret;
    }

    if (stmt->type == STMT_EXPR) {
        debug("Found an expr statement\n");
        list_t *expr_instrs = expr_to_instrs(stmt->expr, env);
        if (!expr_instrs)
            return NULL;
        list_concat(ret, expr_instrs);
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
    output_t *fn_label = new_label(fn_def->name, LABEL_GLOBAL);
    list_push(ret, fn_label);
    
    stmt_t *curr_stmt = list_pop(fn_def->stmts);
    for (; curr_stmt; curr_stmt = list_pop(fn_def->stmts)) {
        list_t *stmt_instrs = stmt_to_instrs(curr_stmt, fn_def->env);         
        if (!stmt_instrs)
            return NULL;
        list_concat(ret, stmt_instrs);
    }

    return ret;
}

list_t *gen_pseudo_asm(program_t *prog) {
    if (!prog || !prog->fn_defs)
        return NULL;
    debug("=====================Generating IR=====================\n");
    variable_homes = map_new();
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
    {.reg = REG_RCX, .string = "%rcx"},
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
        return string_get(operand.var.name);
    }

    string_t string;
    char buf[100];
    if (string_init(&string) < 0)
        return NULL;
    
    if (operand.type == OPERAND_IMM) {
        snprintf(buf, 100, "$%ld", operand.imm);
    } else if (operand.type == OPERAND_MEM_LOC) {
        snprintf(buf, 100, "$%ld(%s)", operand.mem.offset, reg_to_string(operand.mem.reg));
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
