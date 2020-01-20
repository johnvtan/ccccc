#include "compile.h"

static env_t global_env;
static expr_t *parse_expr(list_t *tokens, env_t *env);

// expect_next() consumes the token
// TODO better error handling stuff
static token_t *expect_next(list_t *tokens, token_type_t expectation) {
    token_t *next = list_pop(tokens);
    if (!next || next->type != expectation) {
        printf("Unexpected token\n");
        exit(-1);
    }
    return next;
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

static expr_t *parse_primary(list_t *tokens, env_t *env) {
    token_t *curr = list_pop(tokens);
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = PRIMARY;
    expr->primary = malloc(sizeof(primary_t));

    if (curr->type == TOK_INT_LIT) {
        expr->primary->integer = curr->int_literal;
        expr->primary->type = PRIMARY_INT;
        return expr;
    }

    if (curr->type == TOK_CHAR_LIT) {
        expr->primary->character = curr->char_literal;
        expr->primary->type = PRIMARY_CHAR;
        return expr;
    }

    if (curr->type == TOK_IDENT) {
        expr->primary->var = curr->ident;
        expr->primary->type = PRIMARY_VAR;
        return expr;
    }

    if (curr->type == TOK_OPEN_PAREN) {
        expr->primary->type = PRIMARY_EXPR;
        expr->primary->expr = parse_expr(tokens, env);
        expect_next(tokens, TOK_CLOSE_PAREN);
        return expr;
    }

    UNREACHABLE("Error parsing expected primary expression\n");
    return NULL;
}

static expr_t *parse_unary(list_t *tokens, env_t *env) {
    if (!tokens || !tokens->len)
        return NULL;

    token_t *curr = list_peek(tokens);
    if (curr->type == TOK_BANG) {
        list_pop(tokens);
        return new_unary_expr(UNARY_LOGICAL_NEG, parse_unary(tokens, env));
    }

    if (curr->type == TOK_TILDE) {
        list_pop(tokens);
        return new_unary_expr(UNARY_BITWISE_COMP, parse_unary(tokens, env));
    }

    if (curr->type == TOK_MINUS) {
        list_pop(tokens);
        return new_unary_expr(UNARY_MATH_NEG, parse_unary(tokens, env));
    }

    return parse_primary(tokens, env);
}

static expr_t *parse_mul_div(list_t *tokens, env_t *env) {
    expr_t *ret = parse_unary(tokens, env);

    if (!ret)
        return NULL;

    token_t *curr = list_peek(tokens);
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_MULT) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_MUL, ret, parse_unary(tokens, env));
        } else if (curr->type == TOK_DIV) {
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

    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_PLUS) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_ADD, ret, parse_mul_div(tokens, env));
        } else if (curr->type == TOK_MINUS) {
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
    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_GT) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_GT, ret, parse_add_sub(tokens, env));
        } else if (curr->type == TOK_LT) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_LT, ret, parse_add_sub(tokens, env));
        } else if (curr->type == TOK_GTE) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_GTE, ret, parse_add_sub(tokens, env));
        } else if (curr->type == TOK_LTE) {
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

    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_NE) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_NE, ret, parse_relational(tokens, env));
        } else if (curr->type == TOK_EQ) {
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

    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_AND) {
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

    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_OR) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_OR, ret, parse_logical_and(tokens, env));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_expr(list_t *tokens, env_t *env) {
    return parse_logical_or(tokens, env);
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
    assert(env->map != NULL);
    map_set(env->map, declare->name, &declare->type);
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

static bool is_type(token_type_t type) {
    switch (type) {
        case TOK_INT_TYPE:
        case TOK_CHAR_TYPE:
            return true;
        default:
            return false;
    }
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
    debug("parse_stmt: Found experssion statement\n");
    expr_t *expr = parse_expr(tokens, env);
    if (!expr)
        return NULL;
    ret->type = STMT_EXPR;
    ret->expr = expr; 
    return ret;
}

static fn_def_t *parse_fn_def(list_t *tokens) {
    fn_def_t *fn = malloc(sizeof(fn_def_t));
    fn->params = list_new();
    fn->stmts = list_new();
    fn->env = malloc(sizeof(env_t));
    fn->env->map = map_new();
    fn->env->outer = &global_env;

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

    // parsing parameters
    // TODO only handling functions without parameters right now
    expect_next(tokens, TOK_OPEN_PAREN);

    // TODO tests don't put void there
    //expect_next(tokens, TOK_VOID_TYPE);
    expect_next(tokens, TOK_CLOSE_PAREN);
    expect_next(tokens, TOK_OPEN_BRACE);

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
        list_push(fn->stmts, stmt);
    }

    // consume the close brace token
    expect_next(tokens, TOK_CLOSE_BRACE);
    return fn;
}

program_t *parse(list_t *tokens) {
    program_t *prog = malloc(sizeof(program_t));
    prog->fn_defs = list_new();
    global_env.map = map_new();

    // global environment is the outermost environment. Should contain all fn defs.
    global_env.outer = NULL;
    while (tokens->len) {
        fn_def_t *next_fn = parse_fn_def(tokens);
        if (!next_fn)
            return NULL;
        map_set(global_env.map, next_fn->name, &next_fn->ret_type);
        list_push(prog->fn_defs, next_fn);
    }
    return prog;
}
