#include "compile.h"

static list_t *expr_to_instrs(expr_t *expr, env_t *env);
static list_t *stmt_to_instrs(stmt_t *stmt, env_t *env, output_t *epilogue_label);

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

static bool var_was_declared(env_t *env, string_t *var, var_t **var_info) {
    while (env) {
        var_t *v = env_get(env, var);
        if (v->declared) {
            *var_info = v;
            return true;
        }
        env = env->parent;
    }
    return false;
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
    if (!out) {
        UNREACHABLE("instr_r2r: malloc failed\n");
    }

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2) {
        UNREACHABLE("instr_r2r: num args != 2\n");
    }

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
    if (!out) {
        UNREACHABLE("instr_i2r: malloc failed\n");
    }

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2) {
        UNREACHABLE("instr_i2r: num args != 2\n");
    }

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_IMM;
    out->instr.src.imm = src;

    out->instr.dst.type = OPERAND_REG;
    out->instr.dst.reg = dst;
    return out;
}

static output_t *instr_r2m(opcode_t op, reg_t src, mem_loc_t dst) {
    output_t *out = malloc(sizeof(output_t)); 
    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2) {
        UNREACHABLE("instr_r2m requires opcode that uses 2 args\n");
    }

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_REG;
    out->instr.src.reg = src;

    out->instr.dst.type = OPERAND_MEM_LOC;
    out->instr.dst.mem = dst;
    return out;
}

static output_t *instr_i2m(opcode_t op, imm_t src, mem_loc_t dst) {
    output_t *out = malloc(sizeof(output_t)); 
    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2) {
        UNREACHABLE("instr_r2m requires opcode that uses 2 args\n");
    }

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_IMM;
    out->instr.src.imm = src;

    out->instr.dst.type = OPERAND_MEM_LOC;
    out->instr.dst.mem = dst;
    return out;
}

static output_t *instr_m2r(opcode_t op, mem_loc_t src, reg_t dst) {
    if (op != OP_MOV) {
        // TODO find instrs where this is illegal
        UNREACHABLE("instr_m2r is only legal for MOV opcode for now\n");
    }
    output_t *out = malloc(sizeof(output_t)); 
    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 2) {
        UNREACHABLE("instr_m2r requires opcode that uses 2 args\n");
    }

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_MEM_LOC;
    out->instr.src.mem = src;

    out->instr.dst.type = OPERAND_REG;
    out->instr.dst.reg = dst;
    return out;
}

static output_t *instr_r(opcode_t op, reg_t src) {
    output_t *out = malloc(sizeof(output_t));
    if (!out) {
        UNREACHABLE("instr_r: malloc failed\n");
    }

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 1) {
        UNREACHABLE("instr_r: num args != 1 for this opcode\n");
    }

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    out->instr.src.type = OPERAND_REG;
    out->instr.src.reg = src;
    return out;
}

static output_t *instr_jmp(opcode_t op, string_t *label) {
    if (op != OP_JMP && op != OP_JE && op != OP_JNE) {
        UNREACHABLE("instr_jmp: not a jmp opcode\n");
    }

    if (!label) {
        UNREACHABLE("instr_jmp: null label\n");
    }

    output_t *jmp = malloc(sizeof(output_t));
    jmp->type = OUTPUT_INSTR;
 
    jmp->instr.num_args = op_to_num_args(op);
    if (jmp->instr.num_args != 1) {
        UNREACHABLE("instr_jmp: opcode has wrong number of args, not even sure how this happens\n");
    }

    jmp->instr.op = op;
    jmp->instr.src.type = OPERAND_LABEL;
    jmp->instr.src.label = label;
    return jmp;
}

static output_t *instr(opcode_t op) {
    output_t *out = malloc(sizeof(output_t));
    if (!out) {
        UNREACHABLE("instr: malloc failed");
    }

    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 0) {
        UNREACHABLE("instr: opcode has nonzero number of args\n");
    }

    out->type = OUTPUT_INSTR;
    out->instr.op = op;

    return out;
}

static list_t *primary_to_instrs(primary_t *primary, env_t *env) {
    if (!primary) {
        UNREACHABLE("primary_to_instrs: wtf are you doing, primary is null\n");
    }

    if (primary->type == PRIMARY_INT) {
        list_t *ret = list_new();
        list_push(ret, instr_i2r(OP_MOV, primary->integer, REG_RAX));
        return ret;
    }

    if (primary->type == PRIMARY_EXPR) {
        return expr_to_instrs(primary->expr, env);
    }

    if (primary->type == PRIMARY_VAR) {
        // Do i just move the variable from its home to RAX?
        list_t *ret = list_new();
        debug("Got primary var: %s\n", string_get(primary->var));

        var_t *var_info = NULL;
        if (!var_was_declared(env, primary->var, &var_info)) {
            UNREACHABLE("Compilation error: variable referenced before declaration\n");
        }
        list_push(ret, instr_m2r(OP_MOV, var_info->home, REG_RAX));
        return ret;
    }

    UNREACHABLE("Unexpected primary expr\n");
    return NULL;
}

static list_t *unary_to_instrs(unary_expr_t *unary, env_t *env) {
    list_t *ret = expr_to_instrs(unary->expr, env);
    if (!ret) {
        UNREACHABLE("unary_to_instrs: unary expr could not be generated\n");
    }
    debug("unary to instrs\n");
    if (unary->op == UNARY_MATH_NEG) {
        list_push(ret, instr_r(OP_NEG, REG_RAX));
        return ret;
    } 

    if (unary->op == UNARY_LOGICAL_NEG) {
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETE, REG_AL));
        return ret;
    }

    if (unary->op == UNARY_BITWISE_COMP) {
        list_push(ret, instr_r(OP_NOT, REG_RAX));
        return ret;
    }

    if (unary->op == UNARY_POSTINC) {
        debug("post inc\n");
        var_t *var_info = NULL;
        if (!var_was_declared(env, unary->expr->primary->var, &var_info)) {
            UNREACHABLE("assign_to_instrs: variable is used before declaration");
        }

        // Note that while we could add 1 directly to the memory location, this code is more
        // generic and will be easier to use when dereference/array code is supported
        list_push(ret, instr_i2r(OP_ADD, 1, REG_RAX));
        list_push(ret, instr_r2m(OP_MOV, REG_RAX, var_info->home));
        return ret;
    }

    if (unary->op == UNARY_POSTDEC) {
        var_t *var_info = NULL;
        debug("Found postdec\n");
        if (!var_was_declared(env, unary->expr->primary->var, &var_info)) {
            UNREACHABLE("assign_to_instrs: variable is used before declaration");
        }
        list_push(ret, instr_i2r(OP_SUB, 1, REG_RAX));
        list_push(ret, instr_r2m(OP_MOV, REG_RAX, var_info->home));
        return ret;
    }

    UNREACHABLE("unexpected unary expr\n");
    return NULL;
}

static list_t *binop_to_instrs(bin_expr_t *bin, env_t *env) {
    if (!bin) {
        UNREACHABLE("binop_to_instrs: bin is null wtf are you doing\n");
    }

    list_t *ret = expr_to_instrs(bin->lhs, env);
    if (!ret) {
        UNREACHABLE("binop_to_instrs: failed to generate lhs\n");
    }

    // AND and OR short circuit, so we don't want to evaluate the RHS if we're not certain we need
    // to.
    debug("Found bin op expr\n");
    if (bin->op == BIN_OR) {
        string_t *or_clause_2 = unique_label("second_or_clause");
        string_t *end = unique_label("or_end");

        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_jmp(OP_JE, or_clause_2));
        list_push(ret, instr_i2r(OP_MOV, 1, REG_RAX));
        list_push(ret, instr_jmp(OP_JMP, end));
        list_push(ret, new_label(or_clause_2, LABEL_STATIC));

        list_concat(ret, expr_to_instrs(bin->rhs, env));

        list_push(ret, new_label(end, LABEL_STATIC));
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETNE, REG_AL));
        return ret;
    }

    if (bin->op == BIN_AND) {
        string_t *and_clause_2 = unique_label("second_and_clause");
        string_t *end = unique_label("and_end");
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
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

    list_push(ret, instr_r(OP_PUSH, REG_RAX));
    list_concat(ret, expr_to_instrs(bin->rhs, env));
    list_push(ret, instr_r(OP_POP, REG_RCX));

    if (bin->op == BIN_ADD) {
        list_push(ret, instr_r2r(OP_ADD, REG_RCX, REG_RAX));
        return ret;
    }

    if (bin->op == BIN_SUB) {
        // sub src, dst computes dst - src
        // so we want to compute e1 - e2, and e1 is in rcx
        // so we'll do rcx - rax, then move rcx into rax
        list_push(ret, instr_r2r(OP_SUB, REG_RAX, REG_RCX));
        list_push(ret, instr_r2r(OP_MOV, REG_RCX, REG_RAX));
        return ret;
    }

    if (bin->op == BIN_MUL) {
        list_push(ret, instr_r2r(OP_MUL, REG_RCX, REG_RAX));
        return ret;
    }

    if (bin->op == BIN_DIV) {
        list_push(ret, instr_r2r(OP_XCHG, REG_RAX, REG_RCX));
        list_push(ret, instr(OP_CQO));
        list_push(ret, instr_r(OP_DIV, REG_RCX));
        return ret;
    } 

    if (bin->op == BIN_EQ) {
        list_push(ret, instr_r2r(OP_CMP, REG_RAX, REG_RCX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETE, REG_AL));
        return ret;
    }

    if (bin->op == BIN_NE) {
        list_push(ret, instr_r2r(OP_CMP, REG_RAX, REG_RCX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETNE, REG_AL));
        return ret;
    }

    if (bin->op == BIN_LT) {
        list_push(ret, instr_r2r(OP_CMP, REG_RAX, REG_RCX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETL, REG_AL));
        return ret;
    }

    if (bin->op == BIN_LTE) {
        list_push(ret, instr_r2r(OP_CMP, REG_RAX, REG_RCX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETLE, REG_AL));
        return ret;
    }

    if (bin->op == BIN_GT) {
        list_push(ret, instr_r2r(OP_CMP, REG_RAX, REG_RCX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETG, REG_AL));
        return ret;
    }

    if (bin->op == BIN_GTE) {
        list_push(ret, instr_r2r(OP_CMP, REG_RAX, REG_RCX));
        list_push(ret, instr_i2r(OP_MOV, 0, REG_RAX));
        list_push(ret, instr_r(OP_SETGE, REG_AL));
        return ret;
    }
    UNREACHABLE("Unknown binary op\n");
    return NULL;
}

static list_t *ternary_to_instrs(ternary_t *ternary, env_t *env) {
    list_t *ret = list_new();
    string_t *els_label = unique_label("else");
    string_t *post_cond_label = unique_label("post_cond");

    list_concat(ret, expr_to_instrs(ternary->cond, env));

    list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
    list_push(ret, instr_jmp(OP_JE, els_label));

    list_concat(ret, expr_to_instrs(ternary->then, env));
    list_push(ret, instr_jmp(OP_JMP, post_cond_label));

    list_push(ret, new_label(els_label, LABEL_STATIC));
    list_concat(ret, expr_to_instrs(ternary->els, env));
    list_push(ret, new_label(post_cond_label, LABEL_STATIC));
    return ret;
}

static bool is_valid_assign_lhs(expr_t *expr) {
    return expr->type == PRIMARY && expr->primary->type == PRIMARY_VAR;
}

static list_t *assign_to_instrs(assign_t *assign, env_t *env) {
    debug("Found assignment statement\n");
    if (!is_valid_assign_lhs(assign->lhs)) {
        UNREACHABLE("assign_to_instrs: invalid lhs to assignment statement\n");
    }

    list_t *ret = list_new();
    list_concat(ret, expr_to_instrs(assign->rhs, env));
    var_t *var_info = NULL;
    if (!var_was_declared(env, assign->lhs->primary->var, &var_info)) {
        UNREACHABLE("assign_to_instrs: variable is used before declaration");
    }
    output_t *mov = instr_r2m(OP_MOV, REG_RAX, var_info->home);
    list_push(ret, mov);
    return ret;
}

// TODO how to not put everything into eax
static list_t *expr_to_instrs(expr_t *expr, env_t *env) {
    if (!expr) {
        UNREACHABLE("expr_to_instrs: expr is invalid\n");
    }

    debug("expr to instrs\n");
    if (expr->type == PRIMARY) {
        return primary_to_instrs(expr->primary, env);
    }

    if (expr->type == UNARY_OP) {
        return unary_to_instrs(expr->unary, env);
    }

    if (expr->type == BIN_OP) {
        return binop_to_instrs(expr->bin, env);
    }

    if (expr->type == TERNARY) {
        return ternary_to_instrs(expr->ternary, env);
    }

    if (expr->type == ASSIGN) {
        return assign_to_instrs(expr->assign, env);
    }

    if (expr->type == NULL_EXPR) {
        return list_new();
    }

    UNREACHABLE("unhandled expression\n");
    return NULL;
}

static list_t *block_to_instrs(block_t *block, output_t *epilogue_label) {
    list_t *ret = list_new();
    stmt_t *curr_stmt = list_pop(block->stmts);
    for (; curr_stmt; curr_stmt = list_pop(block->stmts)) {
        list_concat(ret, stmt_to_instrs(curr_stmt, block->env, epilogue_label));
    }
    return ret;
}

static list_t *block_or_single_to_instrs(block_or_single_t *block_or_single, env_t *env, output_t *epilogue_label) {
    list_t *ret = list_new();
    if (block_or_single->type == SINGLE) {
        list_concat(ret, stmt_to_instrs(block_or_single->single, env, epilogue_label));
    } else {
        list_concat(ret, block_to_instrs(block_or_single->block, epilogue_label));
    }
    return ret;
}

static list_t *stmt_to_instrs(stmt_t *stmt, env_t *env, output_t *epilogue_label) {
    if (!stmt) {
        UNREACHABLE("stmt_to_instrs: stmt is invalid\n");
    }

    if (stmt->type == STMT_RETURN) {
        list_t *ret = list_new();
        debug("Found return statement\n");
        list_t *expr_instrs = expr_to_instrs(stmt->ret->expr, env);
        list_concat(ret, expr_instrs);
        list_push(ret, instr_jmp(OP_JMP, epilogue_label->label.name));
        return ret;
    }

    if (stmt->type == STMT_IF) {
        debug("Found if statement\n");
        list_t *ret = list_new();
        
        list_concat(ret, expr_to_instrs(stmt->if_stmt->cond, env));
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));

        string_t *post_cond_label = unique_label("post_cond");
        output_t *post_cond_label_output = new_label(post_cond_label, LABEL_STATIC);
        list_t *then_instrs = block_or_single_to_instrs(stmt->if_stmt->then, env, epilogue_label);

        if (stmt->if_stmt->els) {
            string_t *else_label = unique_label("else");
            list_push(ret, instr_jmp(OP_JE, else_label));
            list_concat(ret, then_instrs);
            list_push(ret, instr_jmp(OP_JMP, post_cond_label));
            list_push(ret, new_label(else_label, LABEL_STATIC));
            list_concat(ret, block_or_single_to_instrs(stmt->if_stmt->els, env, epilogue_label));
            list_push(ret, post_cond_label_output);
        } else {
            // No else statement, just emit the then instructions
            list_push(ret, instr_jmp(OP_JE, post_cond_label));
            list_concat(ret, then_instrs);
            list_push(ret, post_cond_label_output);
        }
        return ret;
    }

    if (stmt->type == STMT_BLOCK) {
        debug("Found block statement\n");
        return block_to_instrs(stmt->block, epilogue_label);
    }

    if (stmt->type == STMT_DECLARE) {
        debug("Found a declare statement for var %s\n", string_get(stmt->declare->name));

        // only check if the variable is declared in this scope.
        var_t *var_info = map_get(env->homes, stmt->declare->name);
        if (var_info->declared) {
            UNREACHABLE("Compilation error: variable has multiple definitions in the same scope");
        }

        debug("Set declared for var\n");
        var_info->declared = true;
        if (stmt->declare->init_expr) {
            list_t *ret = list_new();
            list_concat(ret, expr_to_instrs(stmt->declare->init_expr, env));

            // expr should be in rax, so move it to the variable home.
            output_t *mov = instr_r2m(OP_MOV, REG_RAX, var_info->home);
            list_push(ret, mov);
            return ret;
        }
        // TODO need better way of signalling that we don't want to add any instructions
        // for this statement
        return NULL;
    }

    if (stmt->type == STMT_EXPR) {
        list_t *ret = list_new();
        debug("Found an expr statement\n");
        list_t *expr_instrs = expr_to_instrs(stmt->expr, env);
        if (!expr_instrs) {
            UNREACHABLE("expr_instrs is invalid")
        }
        list_concat(ret, expr_instrs);
        return ret;
    }

    if (stmt->type == STMT_FOR) {
        /*
         * Init 
         * begin for label * cond
         * jz post_for
         * body
         * post
         * jmp begin for
         * post_for
         */
        list_t *ret = list_new();
        list_concat(ret, stmt_to_instrs(stmt->for_stmt->init, stmt->for_stmt->env, epilogue_label));
        
        string_t *begin_for_label = unique_label("begin_for");
        string_t *post_for_label = unique_label("post_for");

        list_push(ret, new_label(begin_for_label, LABEL_STATIC));
        list_concat(ret, expr_to_instrs(stmt->for_stmt->cond, stmt->for_stmt->env));
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_jmp(OP_JE, post_for_label));

        list_concat(ret, block_or_single_to_instrs(stmt->for_stmt->body, stmt->for_stmt->env, epilogue_label));
        list_concat(ret, expr_to_instrs(stmt->for_stmt->post, stmt->for_stmt->env));
        list_push(ret, instr_jmp(OP_JMP, begin_for_label));
        list_push(ret, new_label(post_for_label, LABEL_STATIC));
        return ret;
    }

    if (stmt->type == STMT_WHILE) {
        list_t *ret = list_new();
        string_t *begin_while_label = unique_label("begin_while");
        string_t *post_while_label = unique_label("post_while");

        list_push(ret, new_label(begin_while_label, LABEL_STATIC));

        list_concat(ret, expr_to_instrs(stmt->while_stmt->cond, env));

        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_jmp(OP_JE, post_while_label));

        list_concat(ret, block_or_single_to_instrs(stmt->while_stmt->body, env, epilogue_label));
        list_push(ret, instr_jmp(OP_JMP, begin_while_label));
        list_push(ret, new_label(post_while_label, LABEL_STATIC));

        return ret;
    }

    if (stmt->type == STMT_DO) {
        list_t *ret = list_new();
        string_t *begin_do_label = unique_label("begin_do");
        list_push(ret, new_label(begin_do_label, LABEL_STATIC));
        list_concat(ret, block_or_single_to_instrs(stmt->do_stmt->body, env, epilogue_label));
        debug("body\n");
        list_concat(ret, expr_to_instrs(stmt->do_stmt->cond, env));
        debug("cond\n");
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_jmp(OP_JNE, begin_do_label));
        return ret;
    }

    UNREACHABLE("Unrecognized statement type\n");
    return NULL;
} 

// transforms an fn_def_t ast node to a list of x86 instructions
static list_t *fn_def_to_asm(fn_def_t *fn_def) {
    if (!fn_def || !fn_def->name || !fn_def->stmts) {
        UNREACHABLE("fn_def_to_asm: malformed fn_def\n");
    }

    // TODO totally skipping params now
    // TODO type checking on return type, but also skipping that for now 
    list_t *ret = list_new();
    output_t *fn_label = new_label(fn_def->name, LABEL_GLOBAL);
    list_push(ret, fn_label);

    // function prologue
    list_push(ret, instr_r(OP_PUSH, REG_RBP));
    list_push(ret, instr_r2r(OP_MOV, REG_RSP, REG_RBP));

    list_push(ret, instr_i2r(OP_ADD, fn_def->env->sp_offset, REG_RSP));
    
    // function epilogue label
    output_t *epilogue_label = new_label(unique_label("fn_epilogue"), LABEL_STATIC);

    stmt_t *curr_stmt = list_pop(fn_def->stmts);
    for (; curr_stmt; curr_stmt = list_pop(fn_def->stmts)) {
        list_t *instrs = stmt_to_instrs(curr_stmt, fn_def->env, epilogue_label);
        list_concat(ret, instrs);
    }

    // function epilogue
    list_push(ret, epilogue_label);
    list_push(ret, instr_r2r(OP_MOV, REG_RBP, REG_RSP));
    list_push(ret, instr_r(OP_POP, REG_RBP));
    list_push(ret, instr(OP_RET));

    return ret;
}

list_t *gen_asm(program_t *prog) {
    if (!prog || !prog->fn_defs) {
        UNREACHABLE("gen_asm: malformed program\n");
    }
    debug("=====================Generating ASM=====================\n");
    list_t *output = list_new();
    fn_def_t *fn_def = list_pop(prog->fn_defs);
    for (; fn_def; fn_def = list_pop(prog->fn_defs)) {
        list_t *fn_instrs = fn_def_to_asm(fn_def);
        if (!fn_instrs) {
            UNREACHABLE("gen_asm: null fn_instrs\n");
        }
        list_concat(output, fn_instrs);
    }
    debug("length: %d\n", output->len);
    return output;
}

