#include "compile.h"
// global - needed for home allocation
env_t *global_env = NULL;
program_t *program = NULL;

static expr_t *parse_expr(list_t *tokens, env_t *env);
static stmt_t *parse_stmt(list_t *tokens, env_t *env);
static block_t *parse_block(list_t *tokens, env_t *env);

// for now, only idents can be used in assign statements
// Later we want this to check for specific unary ops, like pointer derefs and pre/postinc and array
// refs
static bool is_valid_lhs(expr_t *expr) {
    return expr && expr->type == PRIMARY && expr->primary->type == PRIMARY_VAR;
}

static bool fn_was_declared(string_t *fn_name) {
    return map_contains(program->fn_defs, fn_name);    
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

// Consumes the next token. Program fails if expectation is not met.
static token_t *expect_next(list_t *tokens, token_type_t expectation) {
    token_t *next = list_pop(tokens);
    if (!next || next->type != expectation) {
        debug("Unexpected token: %u\n", (unsigned)next->type);
        UNREACHABLE("Parse failed");
    }
    return next;
}

// Just peeks at the next token. Doesn't consume.
static bool check_next(list_t *tokens, token_type_t expectation) {
    token_t *next = list_peek(tokens);
    if (!next || next->type != expectation) {
        return false;
    }
    return true;
}

// Helper to check the next token and consume if it matches the expectation.
static bool match(list_t *tokens, token_type_t expectation) {
    token_t *next = list_peek(tokens);
    if (!next || next->type != expectation) {
        return false;
    }
    debug("Found expected\n");
    list_pop(tokens);
    return true;
}

static token_t *match_or_fail(list_t *tokens, token_type_t expectation) {
    token_t *next = list_pop(tokens);
    if (!next || next->type != expectation) {
        UNREACHABLE("match failed\n");
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
            UNREACHABLE("token_to_builtin_type: Trying to parse unrecognized type\n");
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

static expr_t *new_ternary(expr_t *cond, expr_t *then, expr_t *els) {
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = TERNARY;
    expr->ternary = malloc(sizeof(ternary_t));
    expr->ternary->cond = cond;
    expr->ternary->then = then;
    expr->ternary->els = els;
    return expr;
}

static expr_t *new_null_expr(void) {
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = NULL_EXPR;
    return expr;
}

static expr_t *new_fn_call(string_t *fn_name, list_t *tokens, env_t *env) {
    expr_t *expr = malloc(sizeof(expr_t));
    expr->type = PRIMARY;

    expr->primary = malloc(sizeof(primary_t));
    expr->primary->type = PRIMARY_FN_CALL;

    expr->primary->fn_call = malloc(sizeof(fn_call_t));
    expr->primary->fn_call->fn_name = fn_name;

    // Parse parameter expressions
    expect_next(tokens, TOK_OPEN_PAREN);

    if (match(tokens, TOK_CLOSE_PAREN)) {
        return expr;
    }

    while (tokens->len) {
        list_push(expr->primary->fn_call->param_exprs, parse_expr(tokens, env));
        if (match(tokens, TOK_CLOSE_PAREN)) {
            break;
        }
        expect_next(tokens, TOK_COMMA);
    }

    return expr;
}

static expr_t *parse_primary(list_t *tokens, env_t *env) {
    debug("parse primary\n");
    token_t *curr = list_peek(tokens);

    if (curr->type == TOK_INT_LIT) {
        list_pop(tokens);
        debug("Found integer literal: %d\n", curr->int_literal);
        return new_primary_int(curr->int_literal);
    }

    if (curr->type == TOK_CHAR_LIT) {
        list_pop(tokens);
        debug("Found character literal\n");
        return new_primary_char(curr->char_literal);
    }

    if (curr->type == TOK_IDENT) {
        // Ident can be either a variable or function call
        // It seems like really, functions are variables in the global
        // environment.
        list_pop(tokens);

        if (env_get(env, curr->ident)) {
            // First, assume that the identifier is a variable.
            debug("Found variable: %s\n", string_get(curr->ident));
            return new_primary_var(curr->ident);
        }

        if (map_contains(program->fn_defs, curr->ident)) {
            // Otherwise, try to see if it's a function call.
            debug("Found function call: %s\n", string_get(curr->ident));
            return new_fn_call(curr->ident, tokens, env);
        }

        UNREACHABLE("Primary: bad ident\n");
    }

    if (curr->type == TOK_OPEN_PAREN) {
        list_pop(tokens);
        debug("Found nested expr\n");
        expr_t *expr = parse_expr(tokens, env);
        expect_next(tokens, TOK_CLOSE_PAREN);
        return new_primary_expr(expr);
    }
    UNREACHABLE("parse: Unrecognized expression\n");
}

static expr_t *parse_postfix(list_t *tokens, env_t *env) {
    if (!tokens || !tokens->len) {
        UNREACHABLE("wtf you doin\n");
    }

    expr_t *primary_expr = parse_primary(tokens, env);
    if (!tokens->len) {
        UNREACHABLE("compile error? should at least be a semicolon here\n");
    }

    if (match(tokens, TOK_INCREMENT)) {
        debug("Found postinc\n");
        if (!is_valid_lhs(primary_expr)) {
            UNREACHABLE("compile error: trying to postinc invalid lhs\n");
        }
        return new_unary_expr(UNARY_POSTINC, primary_expr);
    }

    if (match(tokens, TOK_DECREMENT)) {
        debug("Found postdec\n");
        if (!is_valid_lhs(primary_expr)) {
            UNREACHABLE("compile error: trying to postdec invalid lhs\n");
        }
        return new_unary_expr(UNARY_POSTDEC, primary_expr);
    }

    return primary_expr;
}

static expr_t *parse_unary(list_t *tokens, env_t *env) {
    if (!tokens || !tokens->len) {
        UNREACHABLE("wtf you doin\n");
    }

    if (match(tokens, TOK_BANG)) {
        debug("Found unary bang\n");
        return new_unary_expr(UNARY_LOGICAL_NEG, parse_unary(tokens, env));
    }

    if (match(tokens, TOK_TILDE)) {
        debug("Found unary tilde\n");
        return new_unary_expr(UNARY_BITWISE_COMP, parse_unary(tokens, env));
    }

    if (match(tokens, TOK_MINUS)) {
        debug("Found unary neg\n");
        return new_unary_expr(UNARY_MATH_NEG, parse_unary(tokens, env));
    }

    if (match(tokens, TOK_INCREMENT)) {
        debug("Found preincrement\n");
        expr_t *lhs = parse_unary(tokens, env);
        if (!is_valid_lhs(lhs)) {
            UNREACHABLE("Compile error: invalid lhs for preincrement operator\n");
        }
        expr_t *rhs = new_bin_expr(BIN_ADD, lhs, new_primary_int(1));
        return new_assign(lhs, rhs);
    }

    if (match(tokens, TOK_DECREMENT)) {
        debug("Found predecrement\n");
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

    while (tokens->len) {
        if (match(tokens, TOK_MULT)) {
            debug("Found mult\n");
            ret = new_bin_expr(BIN_MUL, ret, parse_unary(tokens, env));
        } else if (match(tokens, TOK_DIV)) {
            debug("Found div\n");
            ret = new_bin_expr(BIN_DIV, ret, parse_unary(tokens, env));
        } else if (match(tokens, TOK_MODULO)){
            debug("Found modulo\n");
            ret = new_bin_expr(BIN_MODULO, ret, parse_mul_div(tokens, env));
        } else {
            break;
        }
    }
    return ret;
}

static expr_t *parse_add_sub(list_t *tokens, env_t *env) {
    expr_t *ret = parse_mul_div(tokens, env);

    while (tokens->len) {
        if (match(tokens, TOK_PLUS)) {
            debug("Found plus\n");
            ret = new_bin_expr(BIN_ADD, ret, parse_mul_div(tokens, env));
        } else if (match(tokens, TOK_MINUS)) {
            debug("Found minus\n");
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
    while ((curr = list_peek(tokens))) {
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
    while ((curr = list_peek(tokens))) {
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
    while ((curr = list_peek(tokens))) {
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
    while ((curr = list_peek(tokens))) {
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

static expr_t *parse_ternary(list_t *tokens, env_t *env) {
    expr_t *cond = parse_logical_or(tokens, env);
    if (!check_next(tokens, TOK_QUESTION)) {
        debug("parse_ternary: no question mark found, not a ternary\n");
        return cond;
    }

    debug("parse_ternary: Found a ternary\n");
    expect_next(tokens, TOK_QUESTION);
    expr_t *then = parse_expr(tokens, env);
    debug("parse_ternary: got then\n");
    expect_next(tokens, TOK_COLON);
    expr_t *els = parse_ternary(tokens, env);
    debug("parse_ternary: got else\n");

    return new_ternary(cond, then, els);
}

// Handles += as well as =
static expr_t *parse_assign(list_t *tokens, env_t *env) {
    expr_t *maybe_lhs = parse_ternary(tokens, env);
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
    debug("parse_expr\n");
    return parse_assign(tokens, env);
}

static expr_t *parse_optional_expr(list_t *tokens, env_t *env, token_type_t delimiter) {
    if (check_next(tokens, delimiter))
        return new_null_expr();
    return parse_expr(tokens, env);
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
    env_add(env, declare->name, declare->type);

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

static block_or_single_t *parse_block_or_single(list_t *tokens, env_t *env) {
    block_or_single_t *ret = malloc(sizeof(block_or_single_t));
    if (check_next(tokens, TOK_OPEN_BRACE)) {
        debug("Parsing block\n");
        ret->type = BLOCK;
        ret->block = parse_block(tokens, env);
    } else {
        debug("Parsing single\n");
        ret->type = SINGLE;
        ret->single = parse_stmt(tokens, env);
        if (ret->single->type == STMT_DECLARE) {
            UNREACHABLE("Error: in single of block or single, statement cannot be a declare");
        }
    }
    return ret;
}

static if_stmt_t *parse_if_stmt(list_t *tokens, env_t *env) {
    expect_next(tokens, TOK_IF);
    debug("parse_if_stmt: found if\n");
    if_stmt_t *ret = malloc(sizeof(if_stmt_t));
    expect_next(tokens, TOK_OPEN_PAREN);
    ret->cond = parse_expr(tokens, env);
    debug("parse_if_stmt: got cond\n");
    expect_next(tokens, TOK_CLOSE_PAREN);

    ret->then = malloc(sizeof(block_or_single_t));
    ret->then = parse_block_or_single(tokens, env);
    ret->els = NULL;
    if (check_next(tokens, TOK_ELSE)) {
        debug("parse_if_stmt: found else\n");
        list_pop(tokens);
        ret->els = parse_block_or_single(tokens, env);
    }
    debug("parse_if_stmt: done\n");
    return ret;
}

// A for statement init clause is either a declaration or an optional expression.
static stmt_t *parse_for_init_clause(list_t *tokens, env_t *env) {
    token_t *curr_token = list_peek(tokens);
    if (is_type(curr_token->type))
        return parse_stmt(tokens, env);

    stmt_t *ret = malloc(sizeof(stmt_t));
    ret->type = STMT_EXPR; 
    ret->expr = parse_optional_expr(tokens, env, TOK_SEMICOLON);
    expect_next(tokens, TOK_SEMICOLON);
    return ret;
}

static for_stmt_t *parse_for_stmt(list_t *tokens, env_t *env) {
    expect_next(tokens, TOK_FOR);
    debug("parse_for_stmt: found for\n");

    for_stmt_t *ret = malloc(sizeof(for_stmt_t));
    ret->env = env_new(env);
    expect_next(tokens, TOK_OPEN_PAREN);
    ret->init = parse_for_init_clause(tokens, ret->env);

    debug("for: got init\n");
    ret->cond = parse_optional_expr(tokens, ret->env, TOK_SEMICOLON);
    // For loops with no cond should run forever - replace the cond with a nonzero int.
    if (ret->cond->type == NULL_EXPR) {
        ret->cond = new_primary_int(1);
    }
    expect_next(tokens, TOK_SEMICOLON);
    debug("for: got cond\n");
    ret->post = parse_optional_expr(tokens, ret->env, TOK_CLOSE_PAREN);
    expect_next(tokens, TOK_CLOSE_PAREN);
    debug("for: got post\n");
    ret->body = parse_block_or_single(tokens, ret->env);
    return ret;
}

static while_stmt_t *parse_while_stmt(list_t *tokens, env_t *env) {
    expect_next(tokens, TOK_WHILE);
    debug("parse_while_stmt: found while\n");
    while_stmt_t *ret = malloc(sizeof(while_stmt_t));
    expect_next(tokens, TOK_OPEN_PAREN);
    ret->cond = parse_expr(tokens, env);
    debug("parse_while_stmt: got cond\n");
    expect_next(tokens, TOK_CLOSE_PAREN);
    ret->body = parse_block_or_single(tokens, env);
    debug("parse_while_stmt: got body\n");
    return ret;
}

static do_stmt_t *parse_do_stmt(list_t *tokens, env_t *env) {
    expect_next(tokens, TOK_DO);
    debug("parse_do_stmt: do found\n");
    do_stmt_t *ret = malloc(sizeof(do_stmt_t));
    ret->body = parse_block_or_single(tokens, env);
    debug("parse_do_stmt: got body\n");

    expect_next(tokens, TOK_WHILE);
    expect_next(tokens, TOK_OPEN_PAREN);
    ret->cond = parse_expr(tokens, env);
    debug("parse_do_stmt: got cond\n");
    expect_next(tokens, TOK_CLOSE_PAREN);
    expect_next(tokens, TOK_SEMICOLON);
    return ret;
}

static list_t *parse_stmt_list(list_t *tokens, env_t *env) {
    expect_next(tokens, TOK_OPEN_BRACE);
    list_t *stmt_list = list_new();
    token_t *curr;
    debug("parsing statement list\n");
    while ((curr = list_peek(tokens)) && curr->type != TOK_CLOSE_BRACE) {
        list_push(stmt_list, parse_stmt(tokens, env));
    }
    debug("done parsing statement list\n");
    expect_next(tokens, TOK_CLOSE_BRACE);
    return stmt_list;
}

static block_t *parse_block(list_t *tokens, env_t *outer) {
    block_t *block = malloc(sizeof(block_t));
    block->env = env_new(outer);
    block->stmts = parse_stmt_list(tokens, block->env);
    return block;
}

static stmt_t *parse_stmt(list_t *tokens, env_t *env) {
    if (!tokens || !env)
        return NULL;
    stmt_t *ret = malloc(sizeof(stmt_t));
    token_t *curr = list_peek(tokens);

    if (curr->type == TOK_OPEN_BRACE) {
        ret->block = parse_block(tokens, env);
        ret->type = STMT_BLOCK;
        return ret;
    }

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

    if (curr->type == TOK_FOR) {
        ret->for_stmt = parse_for_stmt(tokens, env);
        ret->type = STMT_FOR;
        return ret;
    }

    if (curr->type == TOK_WHILE) {
        ret->while_stmt = parse_while_stmt(tokens, env);
        ret->type = STMT_WHILE;
        return ret;
    }

    if (curr->type == TOK_DO) {
        debug("Found do stmt\n");
        ret->do_stmt = parse_do_stmt(tokens, env);
        ret->type = STMT_DO;
        return ret;
    }

    if (match(tokens, TOK_BREAK)) {
        debug("parse_stmt: Found break\n");
        expect_next(tokens, TOK_SEMICOLON);
        ret->type = STMT_BREAK;
        return ret;
    }

    if (match(tokens, TOK_CONTINUE)) {
        debug("parse_stmt: Found continue\n");
        expect_next(tokens, TOK_SEMICOLON);
        ret->type = STMT_CONTINUE;
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
    if (match(tokens, TOK_SEMICOLON)) {
        debug("parse_stmt: Found null expr stmt\n");
        ret->type = STMT_NULL;
        return ret;
    }

    debug("parse_stmt: Found nonnull expr stmt\n");
    expr_t *expr = parse_expr(tokens, env);
    ret->type = STMT_EXPR;
    ret->expr = expr; 
    expect_next(tokens, TOK_SEMICOLON);
    return ret;
}

// Parses var and adds it to the passed in environment
// Returns the name of the variable
static string_t *parse_param(list_t *tokens, env_t *env) {
    token_t *type_token = list_pop(tokens);
    token_t *curr = match_or_fail(tokens, TOK_IDENT);
    env_add(env, curr->ident, token_to_builtin_type(type_token->type));
    return curr->ident;
}

// Parses the type, name, and parameters of a function definition.
static fn_def_t *parse_fn_declaration(list_t *tokens) {
    fn_def_t *fn = malloc(sizeof(fn_def_t));

    // TODO The only difference between a function declaration and definition
    // is that stmts will be NULL in a declaration
    fn->stmts = NULL; 
    fn->params = NULL;

    fn->env = env_new(global_env);

    // first token should be a type
    token_t *curr = list_pop(tokens);
    if (!curr) {
        debug("Error in parse_fn_def: no tokens\n");
        return NULL;
    }

    fn->ret_type = token_to_builtin_type(curr->type);

    // next should be an identifier
    curr = expect_next(tokens, TOK_IDENT);
    fn->name = curr->ident;

    debug("parse_fn_declaration: Parsing function %s\n", string_get(fn->name));

    // parsing parameters
    expect_next(tokens, TOK_OPEN_PAREN);

    if (match(tokens, TOK_CLOSE_PAREN)) {
        return fn;
    }

    if (match(tokens, TOK_VOID_TYPE)) {
        expect_next(tokens, TOK_CLOSE_PAREN);
        return fn;
    }

    fn->params = list_new();
    while (tokens->len) {
        list_push(fn->params, parse_param(tokens, fn->env));
        if (match(tokens, TOK_CLOSE_PAREN)) {
            break;
        }
        expect_next(tokens, TOK_COMMA);
    }



    return fn;
}

program_t *parse(list_t *tokens) {
    program = malloc(sizeof(program_t));
    program->fn_defs = map_new();
    global_env = env_new(NULL);

    while (tokens->len) {
        fn_def_t *next_fn = parse_fn_declaration(tokens);
        map_set(program->fn_defs, next_fn->name, next_fn);
        if (!match(tokens, TOK_SEMICOLON)) {
            next_fn->stmts = parse_stmt_list(tokens, next_fn->env);
        }
    }
    return program;
}
