#include "ast.h"
#include "tokenize.h"

void print_expr(expr_t *e, int indent) {
    if (!e)
        return;
    for (int i = 0; i < indent; i++)
        printf("\t");
    switch(e->type) {
        case INT_LITERAL:
            printf("Int literal %d\n", e->integer);
            break;
        case CHAR_LITERAL:
            printf("char literal %c\n", e->character);
            break;
        default:
            printf("UNKNOWN EXPR\n");
            break;
    }
}

void print_stmt(stmt_t *s, int indent) {
    if (!s)
        return;
    for (int i = 0; i < indent; i++)
        printf("\t");
    printf("Statement: ");
    switch (s->type) {
        case STMT_RETURN:
            printf("return\n");
            print_expr(s->ret->expr, indent + 1);
            break;
        default:
            printf("IDK PANIC STATEMENT: %d\n", (int)(s->type));
            break;
    }
}

void print_ast(program_t *prog) {
    if (!prog)
        return;
    for (fn_def_t *fn = list_pop(prog->fn_defs); fn; fn = list_pop(prog->fn_defs)) {
        printf("Function Definition: %s\n", string_get(fn->name)); 
        printf("Params: ");
        if (!fn->params)
            printf("VOID\n");
        else
            printf("IDK\n");
        for (stmt_t *s = list_pop(fn->stmts); s; s = list_pop(fn->stmts)) {
            print_stmt(s, 1);
        }
    }
}
