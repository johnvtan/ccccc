#include "compile.h"

static token_t *expect_next(list_t *tokens, token_type_t expectation) {
    token_t *next = list_pop(tokens);
    if (!next || next->type != expectation) {
        printf("Unexpected token\n");
        exit(-1);
    }
    return next;
}

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

static expr_t *parse_expr(list_t *tokens) {
    return NULL;
}

static return_stmt_t *parse_return_stmt(list_t *tokens) {
    return NULL;
}

static stmt_t *parse_stmt(list_t *tokens) {
    stmt_t *ret = malloc(sizeof(stmt_t));
    token_t *curr;
    while ()
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
    if (fn->ret_type == TYPE_UNRECOGNIZED)
        return NULL;

    // next should be an identifier
    curr = expect_next(tokens, TOK_IDENT);
    fn->name = curr->ident;

    // parsing parameters
    // TODO only handling functions without parameters right now
    expect_next(tokens, TOK_OPEN_PAREN);
    expect_next(tokens, TOK_VOID_TYPE);
    expect_next(tokens, TOK_CLOSE_PAREN);
    expect_next(tokens, TOK_OPEN_BRACE);

    // parse the statements one by one
    while (tokens->len && (list_peek(tokens))->type != TOK_CLOSE_BRACE) {
        stmt_t *stmt = parse_stmt(tokens); 
        list_push(fn->stmts, stmt);
    }

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