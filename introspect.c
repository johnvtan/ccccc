#include "ast.h"
#include "tokenize.h"

static void print_expr(expr_t *e, int indent);

static void print_indent(int indent) {
    for (int i = 0; i < indent; i++)
        printf("\t");
}

static void print_primary(primary_t *p, int indent) {
    if (!p)
        printf("BAD AST: EXPECTED NONNULL EXPR");
    print_indent(indent);
    switch (p->type) {
        case PRIMARY_INT:
            printf("int literal %d\n", p->integer);
            return;
        case PRIMARY_CHAR:
            printf("char literal %c\n", p->character);
            return;
        case PRIMARY_EXPR:
            printf("nested expr\n");
            print_expr(p->expr, indent + 1);
            return;
        default:
            printf("UNKNOWN PRIMARY TYPE\n");
            return;
    }
}

static void print_unary(unary_expr_t *u, int indent) {
    if (!u)
        printf("BAD AST: EXPECTED NONNULL EXPR");
    print_indent(indent);
    switch (u->op) {
        case UNARY_MATH_NEG:
            printf("math neg\n");
            print_expr(u->expr, indent+1);
            return;
        case UNARY_BITWISE_COMP:
            printf("bitwise cmp\n");
            print_expr(u->expr, indent+1);
            return;
        case UNARY_LOGICAL_NEG:
            printf("logical neg\n");
            print_expr(u->expr, indent+1);
            return;
        default:
            printf("UNKNOWN UNARY OP\n");
            return;
    }
}

static void print_binop(bin_expr_t *b, int indent) {
    if (!b)
        printf("BAD AST: EXPECTED NONNULL EXPR");
    print_indent(indent);
    switch(b->op) {
        case BIN_ADD:
            printf("bin add\n");
            print_expr(b->lhs, indent+1);
            print_expr(b->rhs, indent+1);
            return;
        case BIN_SUB:
            printf("bin sub\n");
            print_expr(b->lhs, indent+1);
            print_expr(b->rhs, indent+1);
            return;
        case BIN_MUL:
            printf("bin mul\n");
            print_expr(b->lhs, indent+1);
            print_expr(b->rhs, indent+1);
            return;
        case BIN_DIV:
            printf("bin div\n");
            print_expr(b->lhs, indent+1);
            print_expr(b->rhs, indent+1);
            return;
        default:
            printf("UNKNOWN BINOP\n");
            return;
    }
}

static void print_expr(expr_t *e, int indent) {
    if (!e)
        printf("BAD AST: EXPECTED NONNULL EXPR");
    print_indent(indent);
    switch(e->type) {
        case BIN_OP:
            printf("bin op \n");
            print_binop(e->bin, indent+1);
            break;
        case UNARY_OP:
            printf("unary op\n");
            print_unary(e->unary, indent+1);
            break;
        case PRIMARY:
            printf("primary\n");
            print_primary(e->primary, indent+1);
            break;
        default:
            printf("UNKNOWN EXPR\n");
            break;
    }
}

static void print_stmt(stmt_t *s, int indent) {
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
