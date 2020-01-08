#include "compile.h"

static expr_t *parse_expr(list_t *tokens);

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

static expr_t *parse_primary(list_t *tokens) {
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

    if (curr->type == TOK_OPEN_PAREN) {
        expr->primary->type = PRIMARY_EXPR;
        expr->primary->expr = parse_expr(tokens);
        expect_next(tokens, TOK_CLOSE_PAREN);
        return expr;
    }

    UNREACHABLE("Error parsing expected primary expression\n");
    return NULL;
}

static expr_t *parse_unary(list_t *tokens) {
    if (!tokens || !tokens->len)
        return NULL;

    token_t *curr = list_peek(tokens);
    if (curr->type == TOK_BANG) {
        list_pop(tokens);
        return new_unary_expr(UNARY_LOGICAL_NEG, parse_unary(tokens));
    }

    if (curr->type == TOK_TILDE) {
        list_pop(tokens);
        return new_unary_expr(UNARY_BITWISE_COMP, parse_unary(tokens));
    }

    if (curr->type == TOK_MINUS) {
        list_pop(tokens);
        return new_unary_expr(UNARY_MATH_NEG, parse_unary(tokens));
    }

    return parse_primary(tokens);
}

static expr_t *parse_mul_div(list_t *tokens) {
    expr_t *ret = parse_unary(tokens);

    if (!ret)
        return NULL;

    token_t *curr = list_peek(tokens);
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_MULT) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_MUL, ret, parse_unary(tokens));
        } else if (curr->type == TOK_DIV) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_DIV, ret, parse_unary(tokens));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_add_sub(list_t *tokens) {
    expr_t *ret = parse_mul_div(tokens);

    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_PLUS) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_ADD, ret, parse_mul_div(tokens));
        } else if (curr->type == TOK_MINUS) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_SUB, ret, parse_mul_div(tokens));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_relational(list_t *tokens) {
    expr_t *ret = parse_add_sub(tokens);
    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_GT) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_GT, ret, parse_add_sub(tokens));
        } else if (curr->type == TOK_LT) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_LT, ret, parse_add_sub(tokens));
        } else if (curr->type == TOK_GTE) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_GTE, ret, parse_add_sub(tokens));
        } else if (curr->type == TOK_LTE) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_LTE, ret, parse_add_sub(tokens));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_equality(list_t *tokens) {
    expr_t *ret = parse_relational(tokens);

    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_NE) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_NE, ret, parse_relational(tokens));
        } else if (curr->type == TOK_EQ) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_EQ, ret, parse_relational(tokens));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_logical_and(list_t *tokens) {
    expr_t *ret = parse_equality(tokens);

    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_AND) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_AND, ret, parse_equality(tokens));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_logical_or(list_t *tokens) {
    expr_t *ret = parse_logical_and(tokens);

    if (!ret)
        return NULL;

    token_t *curr;
    while ((curr = list_peek(tokens)) && curr) {
        if (curr->type == TOK_OR) {
            list_pop(tokens);
            ret = new_bin_expr(BIN_OR, ret, parse_logical_and(tokens));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_expr(list_t *tokens) {
    return parse_logical_or(tokens);
}

static return_stmt_t *parse_return_stmt(list_t *tokens) {
    return_stmt_t *ret = malloc(sizeof(return_stmt_t));
    expect_next(tokens, TOK_RETURN);
    ret->expr = parse_expr(tokens);
    expect_next(tokens, TOK_SEMICOLON);
    return ret;
}

static stmt_t *parse_stmt(list_t *tokens) {
    if (!tokens)
        return NULL;
    stmt_t *ret = malloc(sizeof(stmt_t));
    token_t *curr;

    // TODO only parse return statements for now
    curr = list_peek(tokens);
    if (curr->type == TOK_RETURN) {
        return_stmt_t *ret_stmt = parse_return_stmt(tokens);
        if (!ret_stmt)
            return NULL;
        ret->type = STMT_RETURN;
        ret->ret = ret_stmt;
        return ret;
    }

    UNREACHABLE("Unknown statement type");
}

static fn_def_t *parse_fn_def(list_t *tokens) {
    fn_def_t *fn = malloc(sizeof(fn_def_t));
    fn->params = list_new();
    fn->stmts = list_new();

    // first token should be a type
    token_t *curr = list_pop(tokens);
    if (!curr)
        return NULL; // TODO cleanup? actual error handling?

    fn->ret_type = token_to_builtin_type(curr->type);
    if (fn->ret_type == TYPE_UNRECOGNIZED) {
        printf("UNRECOGNIZED TYPE\n");
        return NULL;
    }

    // next should be an identifier
    curr = expect_next(tokens, TOK_IDENT);
    fn->name = curr->ident;

    // parsing parameters
    // TODO only handling functions without parameters right now
    expect_next(tokens, TOK_OPEN_PAREN);

    // TODO tests don't put void there
    //expect_next(tokens, TOK_VOID_TYPE);
    expect_next(tokens, TOK_CLOSE_PAREN);
    expect_next(tokens, TOK_OPEN_BRACE);

    // parse the statements one by one until we find the closed brace
    while ((curr = list_peek(tokens)) && curr->type != TOK_CLOSE_BRACE) {
        stmt_t *stmt = parse_stmt(tokens); 
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
    while (tokens->len) {
        fn_def_t *next_fn = parse_fn_def(tokens);
        if (!next_fn)
            return NULL;
        list_push(prog->fn_defs, next_fn);
    }
    return prog;
}
