#include "compile.h"
// global - needed for home allocation
env_t *global_env;
static expr_t *parse_expr(list_t *tokens, env_t *env);
static stmt_t *parse_stmt(list_t *tokens, env_t *env);
static block_t *parse_block(list_t *tokens, env_t *env);

// for now, only idents can be used in assign statements
// Later we want this to check for specific unary ops, like pointer derefs and pre/postinc and array
// refs
static bool is_valid_lhs(expr_t *expr) {
    return expr && expr->type == PRIMARY && expr->primary->type == PRIMARY_VAR;
}

// expect_next() consumes the token
// TODO better error handling stuff
static token_t *expect_next(list_t *tokens, token_type_t expectation) {
    token_t *next = list_pop(tokens);
    if (!next || next->type != expectation) {
        printf("Unexpected token: %u\n", (unsigned)next->type);
        exit(-1);
    }
    return next;
}

// consumes if true
static bool check_next(list_t *tokens, token_type_t expectation) {
    token_t *next = list_peek(tokens);
    if (!next || next->type != expectation) {
        return false;
    }
    return true;
}

// TODO is this necessar/can i put all types together
static builtin_type_t token_to_builtin_type(token_type_t t) {
    switch (t) {
        case TOK_INT_TYPE:
            return TYPE_INT;
        case TOK_VOID_TYPE:
            return TYPE_VOID;
        default:
            return TYPE_UNRECOGNIZED;
    }
}

static expr_t *new_bin_expr(enum bin_op op, expr_t *lhs, expr_t *rhs) {
    if (!rhs || !lhs)
        return NULL;
    expr_t *expr = malloc(sizeof(bin_expr_t));
    if (!expr)
        return NULL;

    expr->bin = malloc(sizeof(bin_expr_t));
    if (!expr->bin)
        return NULL;
    expr->type = BIN_OP;
    expr->bin->op = op;
    expr->bin->lhs = lhs;
    expr->bin->rhs = rhs;
    return expr;
}

static expr_t *new_unary_expr(enum unary_op op, expr_t *inner) {
    if (!inner)
        return NULL;
    expr_t *expr = malloc(sizeof(unary_expr_t));
    if (!expr)
        return NULL;
    expr->unary = malloc(sizeof(unary_expr_t));
    if (!expr->unary)
        return NULL;
    expr->type = UNARY_OP;
    expr->unary->op = op;
    expr->unary->expr = inner;
    return expr;
}

static expr_t *new_assign(expr_t *lhs, expr_t *rhs) {
   expr_t *ret = malloc(sizeof(expr_t)); 
   ret->type = ASSIGN;
   ret->assign = malloc(sizeof(assign_t));
   ret->assign->lhs = lhs;
   ret->assign->rhs = rhs;
   return ret;
}

static expr_t *new_primary_int(int val) {
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = PRIMARY;
    expr->primary = malloc(sizeof(primary_t));
    expr->primary->type = PRIMARY_INT;
    expr->primary->integer = val;
    return expr;
}

static expr_t *new_primary_char(char c) {
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = PRIMARY;
    expr->primary = malloc(sizeof(primary_t));
    expr->primary->type = PRIMARY_CHAR;
    expr->primary->integer = c;
    return expr;
}

static expr_t *new_primary_var(string_t *var) {
    if (!var) {
        UNREACHABLE("new_primary_var: invalid var\n");
    }
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = PRIMARY;
    expr->primary = malloc(sizeof(primary_t));
    expr->primary->type = PRIMARY_VAR;
    expr->primary->var = var;
    return expr;
}

static expr_t *new_primary_expr(expr_t *e) {
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = PRIMARY;
    expr->primary = malloc(sizeof(primary_t));
    expr->primary->type = PRIMARY_EXPR;
    expr->primary->expr = e;
    return expr;
}

static expr_t *parse_primary(list_t *tokens, env_t *env) {
    token_t *curr = list_pop(tokens);

    if (curr->type == TOK_INT_LIT) {
        debug("Found integer literal: %d\n", curr->int_literal);
        return new_primary_int(curr->int_literal);
    }

    if (curr->type == TOK_CHAR_LIT) {
        debug("Found character literal\n");
        return new_primary_char(curr->char_literal);
    }

    if (curr->type == TOK_IDENT) {
        debug("Found identifier: %s\n", string_get(curr->ident));
        return new_primary_var(curr->ident);
    }

    if (curr->type == TOK_OPEN_PAREN) {
        debug("Found nested expr\n");
        expr_t *expr = parse_expr(tokens, env);
        expect_next(tokens, TOK_CLOSE_PAREN);
        return new_primary_expr(expr);
    }

    UNREACHABLE("Error parsing expected primary expression\n");
    return NULL;
}

static expr_t *parse_postfix(list_t *tokens, env_t *env) {
    if (!tokens || !tokens->len) {
        UNREACHABLE("wtf you doin\n");
    }

    expr_t *primary_expr = parse_primary(tokens, env);
    if (!tokens->len) {
        UNREACHABLE("compile error? should at least be a semicolon here\n");
    }

    token_t *curr = list_peek(tokens);
    if (curr->type == TOK_INCREMENT) {
        debug("Found postinc\n");
        if (!is_valid_lhs(primary_expr)) {
            UNREACHABLE("compile error: trying to postinc invalid lhs\n");
        }
        list_pop(tokens);
        return new_unary_expr(UNARY_POSTINC, primary_expr);
    }

    if (curr->type == TOK_DECREMENT) {
        debug("Found postdec\n");
        if (!is_valid_lhs(primary_expr)) {
            UNREACHABLE("compile error: trying to postdec invalid lhs\n");
        }
        list_pop(tokens);
        return new_unary_expr(UNARY_POSTDEC, primary_expr);
    }

    return primary_expr;
}

static expr_t *parse_unary(list_t *tokens, env_t *env) {
    if (!tokens || !tokens->len) {
        UNREACHABLE("wtf you doin\n");
    }

    token_t *curr = list_peek(tokens);
    if (curr->type == TOK_BANG) {
        debug("Found unary bang\n");
        list_pop(tokens);
        return new_unary_expr(UNARY_LOGICAL_NEG, parse_unary(tokens, env));
    }

    if (curr->type == TOK_TILDE) {
        debug("Found unary tilde\n");
        list_pop(tokens);
        return new_unary_expr(UNARY_BITWISE_COMP, parse_unary(tokens, env));
    }

    if (curr->type == TOK_MINUS) {
        debug("Found unary neg\n");
        list_pop(tokens);
        return new_unary_expr(UNARY_MATH_NEG, parse_unary(tokens, env));
    }

    if (curr->type == TOK_INCREMENT) {
        debug("Found preincrement\n");
        list_pop(tokens);
        expr_t *lhs = parse_unary(tokens, env);
        if (!is_valid_lhs(lhs)) {
            UNREACHABLE("Compile error: invalid lhs for preincrement operator\n");
        }
        expr_t *rhs = new_bin_expr(BIN_ADD, lhs, new_primary_int(1));
        return new_assign(lhs, rhs);
    }

    if (curr->type == TOK_DECREMENT) {
        debug("Found predecrement\n");
        list_pop(tokens);
        expr_t *lhs = parse_unary(tokens, env);
        if (!is_valid_lhs(lhs)) {
            debug("lhs type: %d\n", (int)lhs->type);
            UNREACHABLE("Compile error: invalid lhs for predecrement operator\n");
        }
        expr_t *rhs = new_bin_expr(BIN_SUB, lhs, new_primary_int(1));
        return new_assign(lhs, rhs);
    }

    // Thanks 9cc
    return parse_postfix(tokens, env);
}

static expr_t *parse_mul_div(list_t *tokens, env_t *env) {
    expr_t *ret = parse_unary(tokens, env);

    token_t *curr = list_peek(tokens);
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_MULT) {
            debug("Found mult\n");
            list_pop(tokens);
            ret = new_bin_expr(BIN_MUL, ret, parse_unary(tokens, env));
        } else if (curr->type == TOK_DIV) {
            debug("Found div\n");
            list_pop(tokens);
            ret = new_bin_expr(BIN_DIV, ret, parse_unary(tokens, env));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_add_sub(list_t *tokens, env_t *env) {
    expr_t *ret = parse_mul_div(tokens, env);

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_PLUS) {
            debug("Found plus\n");
            list_pop(tokens);
            ret = new_bin_expr(BIN_ADD, ret, parse_mul_div(tokens, env));
        } else if (curr->type == TOK_MINUS) {
            debug("Found minus\n");
            list_pop(tokens);
            ret = new_bin_expr(BIN_SUB, ret, parse_mul_div(tokens, env));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_relational(list_t *tokens, env_t *env) {
    expr_t *ret = parse_add_sub(tokens, env);

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_GT) {
            debug("Found gt");
            list_pop(tokens);
            ret = new_bin_expr(BIN_GT, ret, parse_add_sub(tokens, env));
        } else if (curr->type == TOK_LT) {
            debug("Found lt");
            list_pop(tokens);
            ret = new_bin_expr(BIN_LT, ret, parse_add_sub(tokens, env));
        } else if (curr->type == TOK_GTE) {
            debug("Found lte");
            list_pop(tokens);
            ret = new_bin_expr(BIN_GTE, ret, parse_add_sub(tokens, env));
        } else if (curr->type == TOK_LTE) {
            debug("Found lte");
            list_pop(tokens);
            ret = new_bin_expr(BIN_LTE, ret, parse_add_sub(tokens, env));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_equality(list_t *tokens, env_t *env) {
    expr_t *ret = parse_relational(tokens, env);

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_NE) {
            debug("Found not equal\n");
            list_pop(tokens);
            ret = new_bin_expr(BIN_NE, ret, parse_relational(tokens, env));
        } else if (curr->type == TOK_EQ) {
            debug("Found equal to\n");
            list_pop(tokens);
            ret = new_bin_expr(BIN_EQ, ret, parse_relational(tokens, env));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_logical_and(list_t *tokens, env_t *env) {
    expr_t *ret = parse_equality(tokens, env);

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_AND) {
            debug("Found logical and expr\n");
            list_pop(tokens);
            ret = new_bin_expr(BIN_AND, ret, parse_equality(tokens, env));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_logical_or(list_t *tokens, env_t *env) {
    expr_t *ret = parse_logical_and(tokens, env);

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_OR) {
            debug("Found logical or expr\n");
            list_pop(tokens);
            ret = new_bin_expr(BIN_OR, ret, parse_logical_and(tokens, env));
        } else {
            break;
        }
    }
    return ret;
}

// Handles += as well as =
static expr_t *parse_assign(list_t *tokens, env_t *env) {
    expr_t *maybe_lhs = parse_logical_or(tokens, env);
    if (!is_valid_lhs(maybe_lhs)) {
        debug("parse_assign: Not a valid lhs so returning\n");
        return maybe_lhs;
    }
    // here we might have an assignment statement
    // we have a valid lhs at least
    debug("parse_assign: Maybe an assign\n");
    token_t *next = list_peek(tokens);
    if (next->type == TOK_ASSIGN) {
        // ok cool we got an assignment statement
        debug("parse_assign: got an assignment statement\n");
        list_pop(tokens);
        expr_t *rhs = parse_expr(tokens, env);
        return new_assign(maybe_lhs, rhs);
    }

    if (next->type == TOK_PLUS_EQ) {
        debug("parse_assign: Got plus equals assignment statement\n");
        list_pop(tokens);

        // plus equals means that lhs = lhs + remaining expr 
        expr_t *rhs = new_bin_expr(BIN_ADD, maybe_lhs, parse_expr(tokens, env));
        return new_assign(maybe_lhs, rhs);
    }

    if (next->type == TOK_MINUS_EQ) {
        debug("parse_assign: Got minus equals assignment statement\n");
        list_pop(tokens);
        expr_t *rhs = new_bin_expr(BIN_SUB, maybe_lhs, parse_expr(tokens, env));
        return new_assign(maybe_lhs, rhs);
    }

    debug("parse_assign: Didn't actually get an assignment :(\n");
    return maybe_lhs;
}

static expr_t *parse_expr(list_t *tokens, env_t *env) {
    return parse_assign(tokens, env);
}

static return_stmt_t *parse_return_stmt(list_t *tokens, env_t *env) {
    debug("parse_return_stmt: parsing return stmt\n");
    return_stmt_t *ret = malloc(sizeof(return_stmt_t));
    expect_next(tokens, TOK_RETURN);
    ret->expr = parse_expr(tokens, env);
    expect_next(tokens, TOK_SEMICOLON);
    return ret;
}

static declare_stmt_t *parse_declare_stmt(list_t *tokens, env_t *env) {
    declare_stmt_t *declare = malloc(sizeof(declare_stmt_t));
    declare->init_expr = NULL;
    token_t *next = list_pop(tokens);
    declare->type = token_to_builtin_type(next->type); 
    next = list_peek(tokens);
    if (next->type != TOK_IDENT) {
        debug("Error in parse_declare_stmt: No identifier found following the type.\n");
        return NULL;
    }
    declare->name = next->ident;
    debug("parse_declare_stmt: Found variable %s\n", string_get(declare->name));
    // TODO do something with env and var
    //map_set(env->map, declare->name, &declare->type);
    var_t *new_var = malloc(sizeof(var_t));
    new_var->name = declare->name;
    new_var->type = declare->type;
    new_var->declared = false;
    list_push(env->vars, new_var);

    list_pop(tokens);
    next = list_peek(tokens);
    if (next->type != TOK_SEMICOLON) {
        debug("parse_declare_stmt: Found init_expr\n");
        expect_next(tokens, TOK_ASSIGN);
        declare->init_expr = parse_expr(tokens, env);
    }
    expect_next(tokens, TOK_SEMICOLON);
    debug("parse_declare_stmt: Returning\n");
    return declare;
}

static if_stmt_t *parse_if_stmt(list_t *tokens, env_t *env) {
    expect_next(tokens, TOK_IF);
    debug("parse_if_stmt: found if\n");
    if_stmt_t *ret = malloc(sizeof(if_stmt_t));
    debug("expected: %u\n", (unsigned)TOK_OPEN_PAREN);
    expect_next(tokens, TOK_OPEN_PAREN);
    ret->cond = parse_expr(tokens, env);
    debug("parse_if_stmt: got cond\n");
    expect_next(tokens, TOK_CLOSE_PAREN);

    ret->then = malloc(sizeof(block_or_single_t));

    if (check_next(tokens, TOK_OPEN_BRACE)) {
        // statement list
        debug("parse_if_stmt: got then block\n");
        ret->then->type = BLOCK;
        ret->then->block = parse_block(tokens, env);
    } else {
        debug("parse_if_stmt: got then single\n");
        // single statement
        ret->then->type = SINGLE;
        ret->then->single = parse_stmt(tokens, env);
    }

    ret->els = NULL;
    if (check_next(tokens, TOK_ELSE)) {
        debug("parse_if_stmt: found else\n");
        list_pop(tokens);
        ret->els = malloc(sizeof(block_or_single_t));
        if (check_next(tokens, TOK_OPEN_BRACE)) {
            // statement list
            debug("parse_if_stmt: found else block\n");
            ret->els->type = BLOCK;
            ret->els->block = parse_block(tokens, env);
        } else {
            // single statement
            debug("parse_if_stmt: found else single\n");
            ret->els->type = SINGLE;
            ret->els->single = parse_stmt(tokens, env);
        }
    }

    return ret;
}

static bool is_type(token_type_t type) {
    switch (type) {
        case TOK_INT_TYPE:
        case TOK_CHAR_TYPE:
            return true;
        default:
            return false;
    }
}

static block_t *parse_block(list_t *tokens, env_t *outer) {
    expect_next(tokens, TOK_OPEN_BRACE);
    token_t *curr; 
    block_t *block = malloc(sizeof(block_t));
    block->stmts = list_new();
    block->env = env_new(outer);
    while ((curr = list_peek(tokens)) && curr->type != TOK_CLOSE_BRACE) {
        list_push(block->stmts, parse_stmt(tokens, block->env));
    }
    expect_next(tokens, TOK_CLOSE_BRACE);
    return block;
}

static stmt_t *parse_stmt(list_t *tokens, env_t *env) {
    if (!tokens || !env)
        return NULL;
    stmt_t *ret = malloc(sizeof(stmt_t));
    token_t *curr;

    // TODO only parse return statements for now
    curr = list_peek(tokens);
    if (curr->type == TOK_RETURN) {
        debug("parse_stmt: Found return statement\n");
        return_stmt_t *ret_stmt = parse_return_stmt(tokens, env);
        if (!ret_stmt)
            return NULL;
        ret->type = STMT_RETURN;
        ret->ret = ret_stmt;
        return ret;
    }

    if (curr->type == TOK_IF) {
        if_stmt_t *if_stmt = parse_if_stmt(tokens, env);
        ret->type = STMT_IF;
        ret->if_stmt = if_stmt;
        return ret;
    }

    // The first token in a declaration is a type
    if (is_type(curr->type)) {
        debug("parse_stmt: Found declaration statement\n");
        declare_stmt_t *declare = parse_declare_stmt(tokens, env);
        if (!declare)
            return NULL;
        ret->type = STMT_DECLARE;
        ret->declare = declare;
        return ret;
    }

    // Otherwise, try to parse an expression
    debug("parse_stmt: Found expression statement\n");
    expr_t *expr = parse_expr(tokens, env);
    if (!expr)
        return NULL;
    ret->type = STMT_EXPR;
    ret->expr = expr; 
    expect_next(tokens, TOK_SEMICOLON);
    return ret;
}

static fn_def_t *parse_fn_def(list_t *tokens) {
    fn_def_t *fn = malloc(sizeof(fn_def_t));
    fn->params = list_new();
    fn->stmts = list_new();
    fn->env = env_new(global_env);

    // first token should be a type
    token_t *curr = list_pop(tokens);
    if (!curr) {
        debug("Error in parse_fn_def: no tokens\n");
        return NULL;
    }

    fn->ret_type = token_to_builtin_type(curr->type);
    if (fn->ret_type == TYPE_UNRECOGNIZED) {
        debug("Error in parse_fn_def: unrecognized type\n");
        return NULL;
    }

    // next should be an identifier
    curr = expect_next(tokens, TOK_IDENT);
    fn->name = curr->ident;

    debug("parse_fn_def: Parsing function %s\n", string_get(fn->name));

    // parsing parameters // TODO only handling functions without parameters right now
    expect_next(tokens, TOK_OPEN_PAREN);

    // TODO tests don't put void there
    //expect_next(tokens, TOK_VOID_TYPE);
    expect_next(tokens, TOK_CLOSE_PAREN);
    expect_next(tokens, TOK_OPEN_BRACE);

    bool return_stmt_found = false;

    // parse the statements one by one until we find the closed brace
    while ((curr = list_peek(tokens)) && curr->type != TOK_CLOSE_BRACE) {
        stmt_t *stmt = parse_stmt(tokens, fn->env); 
        debug("parse_fn_def: Got complete statement\n");
        // TODO return type checking here?
        /* something like:
         * if (stmt->type == STMT_RETURN) {
         *     assert(stmt->expr->type == fn->ret_type);
         * }
         */
        if (stmt->type == STMT_RETURN) {
            return_stmt_found = true;
        }
        list_push(fn->stmts, stmt);
    }

    if (!return_stmt_found) {
        // return 0 if no return statement found.
        // Hacky way to do this is to just generate a return statement here that returns 0.
        stmt_t *stmt = malloc(sizeof(stmt_t));
        stmt->type = STMT_RETURN;
        stmt->ret = malloc(sizeof(return_stmt_t));
        stmt->ret->expr = malloc(sizeof(expr_t));
        stmt->ret->expr->type = PRIMARY;
        stmt->ret->expr->primary = malloc(sizeof(primary_t));
        stmt->ret->expr->primary->type = PRIMARY_INT;
        stmt->ret->expr->primary->integer = 0;
        list_push(fn->stmts, stmt);
    }

    // consume the close brace token
    expect_next(tokens, TOK_CLOSE_BRACE);
    return fn;
}

program_t *parse(list_t *tokens) {
    program_t *prog = malloc(sizeof(program_t));
    prog->fn_defs = list_new();

    // global environment is the outermost environment. Should contain all fn defs.
    global_env = env_new(NULL);

    while (tokens->len) {
        fn_def_t *next_fn = parse_fn_def(tokens);
        //map_set(global_env.map, next_fn->name, &next_fn->ret_type);
        // TODO do something with function names?
        list_push(prog->fn_defs, next_fn);
        list_push(global_env->children, next_fn->env);
    }
    return prog;
}
