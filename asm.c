#include "compile.h"

static list_t *expr_to_instrs(expr_t *expr, env_t *env);
static list_t *stmt_to_instrs(stmt_t *stmt, context_t context);

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
        case OP_CALL:
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

static output_t *instr_label(opcode_t op, string_t *label) {
    if (op != OP_JMP && op != OP_JE && op != OP_JNE && op != OP_CALL) {
        UNREACHABLE("intsr_label: not a jmp or call opcode\n");
    }

    if (!label) {
        UNREACHABLE("intsr_label: null label\n");
    }

    output_t *out = malloc(sizeof(output_t));
    out->type = OUTPUT_INSTR;
 
    out->instr.num_args = op_to_num_args(op);
    if (out->instr.num_args != 1) {
        UNREACHABLE("instr_label: opcode has wrong number of args, not even sure how this happens\n");
    }

    out->instr.op = op;
    out->instr.src.type = OPERAND_LABEL;
    out->instr.src.label = label;
    return out;
}

static output_t *instr_noarg(opcode_t op) {
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

static const reg_t ordered_param_regs[] = {
    REG_RDI,
    REG_RSI,
    REG_RDX,
    REG_RCX,
    REG_R8,
    REG_R9,
};

static list_t *fn_caller_prepare(list_t *params, env_t *env) {
    debug("fn_caller_prepare\n");
    list_t *ret = list_new();

    // Push all caller save regs onto the stack
    list_push(ret, instr_r(OP_PUSH, REG_R10));
    list_push(ret, instr_r(OP_PUSH, REG_R11));
    for (int i = 0; i < 6; i++) {
        list_push(ret, instr_r(OP_PUSH, ordered_param_regs[i]));
    }

    if (!params || !params->len)
        return ret;

    if (params->len > 6) {
        UNREACHABLE("fn_caller_before: Don't support more than 6 parameters yet\n");
    }

    // Generate code for each parameter and put it into the correct register 
    int param_number = 0;
    expr_t *param_expr = list_pop(params);
    for (; param_expr; param_expr = list_pop(params)) {
        debug("dealing with param %d\n", param_number);
        list_concat(ret, expr_to_instrs(param_expr, env));
        list_push(ret, instr_r2r(OP_MOV, REG_RAX, ordered_param_regs[param_number++]));
    }

    debug("prepare done\n");
    return ret;
}

static list_t *fn_caller_restore(void) {
    debug("fn_caller_restore\n");
    list_t *ret = list_new();
    for (int i = 5; i >= 0; i--) {
        list_push(ret, instr_r(OP_POP, ordered_param_regs[i]));
    }

    list_push(ret, instr_r(OP_POP, REG_R11));
    list_push(ret, instr_r(OP_POP, REG_R10));
    return ret;
}

static list_t *fn_callee_prologue(fn_def_t *fn_def) {
    // function prologue
    list_t *ret = list_new();

    // Allocate space for locals
    list_push(ret, instr_r(OP_PUSH, REG_RBP));
    list_push(ret, instr_r2r(OP_MOV, REG_RSP, REG_RBP));
    list_push(ret, instr_i2r(OP_ADD, fn_def->sp_offset, REG_RSP));

    // Save callee-save registers
    list_push(ret, instr_r(OP_PUSH, REG_RBX));
    //list_push(ret, instr_r(OP_PUSH, REG_R12));
    //list_push(ret, instr_r(OP_PUSH, REG_R13));
    //list_push(ret, instr_r(OP_PUSH, REG_R14));
    //list_push(ret, instr_r(OP_PUSH, REG_R15));

    if (!fn_def->params || !fn_def->params->len)
        return ret;

    // Move parameters from registers into their homes on the stack
    string_t *var_name;
    var_info_t *var_info;
    int param_number = 0;
    list_for_each(fn_def->params, var_name) {
        var_info = map_get(fn_def->env->homes, var_name);
        list_push(ret, instr_r2m(OP_MOV, ordered_param_regs[param_number++], var_info->home));
    }
    return ret;
}

static list_t *fn_callee_epilogue(void) {
    list_t *ret = list_new();

    // Restore callee-save registers
    //list_push(ret, instr_r(OP_PUSH, REG_R15));
    //list_push(ret, instr_r(OP_PUSH, REG_R14));
    //list_push(ret, instr_r(OP_PUSH, REG_R13));
    //list_push(ret, instr_r(OP_PUSH, REG_R12));
    list_push(ret, instr_r(OP_PUSH, REG_RBX));

    list_push(ret, instr_r2r(OP_MOV, REG_RBP, REG_RSP));
    list_push(ret, instr_r(OP_POP, REG_RBP));

    return ret;
}

static list_t *primary_to_instrs(primary_t *primary, env_t *env) {
    if (!primary) {
        UNREACHABLE("primary_to_instrs: wtf are you doing, primary is null\n");
    }

    debug("primary to instrs\n");

    if (primary->type == PRIMARY_INT) {
        list_t *ret = list_new();
        list_push(ret, instr_i2r(OP_MOV, primary->integer, REG_RAX));
        debug("int %u\n", primary->integer);
        return ret;
    }

    if (primary->type == PRIMARY_EXPR) {
        return expr_to_instrs(primary->expr, env);
    }

    if (primary->type == PRIMARY_VAR) {
        // Do i just move the variable from its home to RAX?
        list_t *ret = list_new();
        debug("Got primary var: %s\n", string_get(primary->var));

        var_info_t *var_info = env_get_declared(env, primary->var);
        if (!var_info) {
            UNREACHABLE("Compilation error: variable referenced before declaration\n");
        }
        list_push(ret, instr_m2r(OP_MOV, var_info->home, REG_RAX));
        return ret;
    }

    if (primary->type == PRIMARY_FN_CALL) {
        // Parsing checks if this was declared before it was used, so assume this call is good
        debug("Got fn call for function %s\n", string_get(primary->fn_call->fn_name));
        list_t *ret = fn_caller_prepare(primary->fn_call->param_exprs, env);
        list_push(ret, instr_label(OP_CALL, primary->fn_call->fn_name));
        list_concat(ret, fn_caller_restore());

        // Return value should still be in RAX
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
        var_info_t *var_info = env_get_declared(env, unary->expr->primary->var);
        if (!var_info) {
            UNREACHABLE("assign_to_instrs: variable is used before declaration");
        }

        // Note that while we could add 1 directly to the memory location, this code is more
        // generic and will be easier to use when dereference/array code is supported
        list_push(ret, instr_i2m(OP_ADD, 1, var_info->home));
        //list_push(ret, instr_r2m(OP_MOV, REG_RAX, var_info->home));
        return ret;
    }

    if (unary->op == UNARY_POSTDEC) {
        var_info_t *var_info = env_get_declared(env, unary->expr->primary->var);
        debug("Found postdec\n");
        if (!var_info) {
            UNREACHABLE("assign_to_instrs: variable is used before declaration");
        }
        //list_push(ret, instr_i2r(OP_SUB, 1, REG_RAX));
        //list_push(ret, instr_r2m(OP_MOV, REG_RAX, var_info->home));
        list_push(ret, instr_i2m(OP_SUB, 1, var_info->home));
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
        list_push(ret, instr_label(OP_JE, or_clause_2));
        list_push(ret, instr_i2r(OP_MOV, 1, REG_RAX));
        list_push(ret, instr_label(OP_JMP, end));
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
        list_push(ret, instr_label(OP_JNE, and_clause_2));
        list_push(ret, instr_label(OP_JMP, end));
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
        list_push(ret, instr_noarg(OP_CQO));
        list_push(ret, instr_r(OP_DIV, REG_RCX));
        return ret;
    } 

    if (bin->op == BIN_MODULO) {
        // idivq puts quotient in RAX, remainder in RDX
        list_push(ret, instr_r2r(OP_XCHG, REG_RAX, REG_RCX));
        list_push(ret, instr_noarg(OP_CQO));
        list_push(ret, instr_r(OP_DIV, REG_RCX));
        list_push(ret, instr_r2r(OP_MOV, REG_RDX, REG_RAX));
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
    list_push(ret, instr_label(OP_JE, els_label));

    list_concat(ret, expr_to_instrs(ternary->then, env));
    list_push(ret, instr_label(OP_JMP, post_cond_label));

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
    var_info_t *var_info = env_get_declared(env, assign->lhs->primary->var);
    if (!var_info) {
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

static list_t *block_to_instrs(block_t *block, context_t context) {
    list_t *ret = list_new();
    stmt_t *curr_stmt = list_pop(block->stmts);
    context.env = block->env;
    for (; curr_stmt; curr_stmt = list_pop(block->stmts)) {
        list_concat(ret, stmt_to_instrs(curr_stmt, context));
    }
    return ret;
}

static list_t *block_or_single_to_instrs(block_or_single_t *block_or_single, context_t context) {
    list_t *ret = list_new();
    if (block_or_single->type == SINGLE) {
        list_concat(ret, stmt_to_instrs(block_or_single->single, context));
    } else {
        list_concat(ret, block_to_instrs(block_or_single->block, context));
    }
    return ret;
}

static list_t *stmt_to_instrs(stmt_t *stmt, context_t context) {
    if (!stmt) {
        UNREACHABLE("stmt_to_instrs: stmt is invalid\n");
    }
    
    if (stmt->type == STMT_NULL) {
        return list_new();
    }

    if (stmt->type == STMT_RETURN) {
        list_t *ret = list_new();
        debug("Found return statement\n");
        list_t *expr_instrs = expr_to_instrs(stmt->ret->expr, context.env);
        list_concat(ret, expr_instrs);
        list_push(ret, instr_label(OP_JMP, context.return_label));
        return ret;
    }

    if (stmt->type == STMT_BREAK) {
        list_t *ret = list_new();
        list_push(ret, instr_label(OP_JMP, context.iter_break_label));
        return ret;
    }

    if (stmt->type == STMT_CONTINUE) {
        list_t *ret = list_new();
        list_push(ret, instr_label(OP_JMP, context.iter_continue_label));
        return ret;
    }

    if (stmt->type == STMT_IF) {
        debug("Found if statement\n");
        list_t *ret = list_new();
        
        list_concat(ret, expr_to_instrs(stmt->if_stmt->cond, context.env));
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));

        string_t *post_cond_label = unique_label("post_cond");
        output_t *post_cond_label_output = new_label(post_cond_label, LABEL_STATIC);
        list_t *then_instrs = block_or_single_to_instrs(stmt->if_stmt->then, context);

        if (stmt->if_stmt->els) {
            string_t *else_label = unique_label("else");
            list_push(ret, instr_label(OP_JE, else_label));
            list_concat(ret, then_instrs);
            list_push(ret, instr_label(OP_JMP, post_cond_label));
            list_push(ret, new_label(else_label, LABEL_STATIC));
            list_concat(ret, block_or_single_to_instrs(stmt->if_stmt->els, context));
            list_push(ret, post_cond_label_output);
        } else {
            // No else statement, just emit the then instructions
            list_push(ret, instr_label(OP_JE, post_cond_label));
            list_concat(ret, then_instrs);
            list_push(ret, post_cond_label_output);
        }
        return ret;
    }

    if (stmt->type == STMT_BLOCK) {
        debug("Found block statement\n");
        return block_to_instrs(stmt->block, context);
    }

    if (stmt->type == STMT_DECLARE) {
        debug("Found a declare statement for var %s\n", string_get(stmt->declare->name));

        // only check if the variable is declared in this scope.
        var_info_t *var_info = map_get(context.env->homes, stmt->declare->name);
        if (var_info->declared) {
            UNREACHABLE("Compilation error: variable has multiple definitions in the same scope");
        }

        debug("Set declared for var\n");
        var_info->declared = true;
        if (stmt->declare->init_expr) {
            list_t *ret = list_new();
            list_concat(ret, expr_to_instrs(stmt->declare->init_expr, context.env));

            // expr should be in rax, so move it to the variable home.
            output_t *mov = instr_r2m(OP_MOV, REG_RAX, var_info->home);
            list_push(ret, mov);
            return ret;
        }
        return list_new();
    }

    if (stmt->type == STMT_EXPR) {
        list_t *ret = list_new();
        debug("Found an expr statement\n");
        list_t *expr_instrs = expr_to_instrs(stmt->expr, context.env);
        if (!expr_instrs) {
            UNREACHABLE("expr_instrs is invalid")
        }
        list_concat(ret, expr_instrs);
        return ret;
    }

    if (stmt->type == STMT_FOR) {
        string_t *begin_for_label = unique_label("begin_for");
        string_t *post_for_label = unique_label("end_for");
        string_t *post_body_label = unique_label("post_for_body");

        context.env = stmt->for_stmt->env;
        context.iter_continue_label = post_body_label;
        context.iter_break_label = post_for_label;

        list_t *ret = list_new();
        list_concat(ret, stmt_to_instrs(stmt->for_stmt->init, context));

        list_push(ret, new_label(begin_for_label, LABEL_STATIC));
        list_concat(ret, expr_to_instrs(stmt->for_stmt->cond, context.env));
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_label(OP_JE, post_for_label));

        list_concat(ret, block_or_single_to_instrs(stmt->for_stmt->body, context));
        list_push(ret, new_label(post_body_label, LABEL_STATIC));

        list_concat(ret, expr_to_instrs(stmt->for_stmt->post, context.env));
        list_push(ret, instr_label(OP_JMP, begin_for_label));
        list_push(ret, new_label(post_for_label, LABEL_STATIC));
        return ret;
    }

    if (stmt->type == STMT_WHILE) {
        list_t *ret = list_new();
        string_t *begin_while_label = unique_label("begin_while");
        string_t *post_while_label = unique_label("post_while");
        context.iter_continue_label = begin_while_label;
        context.iter_break_label = post_while_label;

        list_push(ret, new_label(begin_while_label, LABEL_STATIC));

        list_concat(ret, expr_to_instrs(stmt->while_stmt->cond, context.env));

        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_label(OP_JE, post_while_label));

        list_concat(ret, block_or_single_to_instrs(stmt->while_stmt->body, context));
        list_push(ret, instr_label(OP_JMP, begin_while_label));
        list_push(ret, new_label(post_while_label, LABEL_STATIC));

        return ret;
    }

    if (stmt->type == STMT_DO) {
        list_t *ret = list_new();
        string_t *begin_do_label = unique_label("begin_do");
        string_t *end_do_label = unique_label("end_do");

        context.iter_continue_label = begin_do_label;
        context.iter_break_label = end_do_label;
        list_push(ret, new_label(begin_do_label, LABEL_STATIC));
        list_concat(ret, block_or_single_to_instrs(stmt->do_stmt->body, context));
        list_concat(ret, expr_to_instrs(stmt->do_stmt->cond, context.env));
        list_push(ret, instr_i2r(OP_CMP, 0, REG_RAX));
        list_push(ret, instr_label(OP_JNE, begin_do_label));
        list_push(ret, new_label(end_do_label, LABEL_STATIC));
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

    debug("Compiling function %s\n", string_get(fn_def->name));

    // TODO totally skipping params now
    // TODO type checking on return type, but also skipping that for now 
    list_t *ret = list_new();
    output_t *fn_label = new_label(fn_def->name, LABEL_GLOBAL);
    list_push(ret, fn_label);
    list_concat(ret, fn_callee_prologue(fn_def));
    
    // function epilogue label
    string_t *fn_epilogue = unique_label("fn_epilogue");
    output_t *epilogue_label = new_label(fn_epilogue, LABEL_STATIC);

    context_t context;
    context.return_label = fn_epilogue;
    context.iter_continue_label = NULL;
    context.iter_break_label = NULL;
    context.env = fn_def->env;

    stmt_t *curr_stmt = list_pop(fn_def->stmts);
    for (; curr_stmt; curr_stmt = list_pop(fn_def->stmts)) {
        list_t *instrs = stmt_to_instrs(curr_stmt, context);
        list_concat(ret, instrs);
    }

    // function epilogue
    list_push(ret, epilogue_label);
    list_concat(ret, fn_callee_epilogue());
    list_push(ret, instr_noarg(OP_RET));

    debug("fn_def_to_asm done\n");
    return ret;
}

list_t *gen_asm(program_t *prog) {
    if (!prog || !prog->fn_defs) {
        UNREACHABLE("gen_asm: malformed program\n");
    }
    debug("=====================Generating ASM=====================\n");
    list_t *output = list_new();

    pair_t *fn_pair;
    map_for_each(prog->fn_defs, fn_pair) {
        fn_def_t *fn_def = fn_pair->value;
        list_t *fn_instrs = fn_def_to_asm(fn_def);
        if (!fn_instrs) {
            UNREACHABLE("gen_asm: null fn_instrs\n");
        }
        list_concat(output, fn_instrs);
    }
    debug("length: %d\n", output->len);
    return output;
}

