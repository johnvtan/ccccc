#ifndef AST_H
#define AST_H

#include "string.h"
#include "list.h"
#include "map.h"

struct expr;
typedef struct expr expr_t;

typedef enum {
    TYPE_UNRECOGNIZED,
    TYPE_INT,
    TYPE_VOID,
} builtin_type_t;

typedef struct {
    builtin_type_t type;
    string_t name; } var_t;

typedef struct {
    enum unary_op {
        UNARY_MATH_NEG,
        UNARY_BITWISE_COMP,
        UNARY_LOGICAL_NEG,
    } op;
    expr_t *expr;
} unary_expr_t;

typedef struct {
    enum bin_op {
        BIN_ADD,
        BIN_SUB,
        BIN_MUL,
        BIN_DIV,
        BIN_LT,
        BIN_GT,
        BIN_LTE,
        BIN_GTE,
        BIN_EQ,
        BIN_NE,
        BIN_AND,
        BIN_OR,
    } op;
    expr_t *lhs;
    expr_t *rhs;
} bin_expr_t;

typedef struct {
    enum primary_type {
        PRIMARY_INT,
        PRIMARY_CHAR,
        PRIMARY_VAR,
        PRIMARY_EXPR,
    } type;
    union {
        int integer;
        char character;
        string_t *var;

        // TODO is this necessary? seems like I could parse these as just another expr_t instead of
        // nesting it like this
        expr_t *expr;
    };
} primary_t;

typedef struct {
    expr_t *lhs;
    expr_t *rhs;
} assign_t;

typedef struct expr {
    enum {
        PRIMARY,
        UNARY_OP,
        BIN_OP,
        ASSIGN,
    } type;
    union {
        primary_t *primary;
        unary_expr_t *unary;
        bin_expr_t *bin;
        assign_t *assign;
    };
} expr_t;

typedef struct {
    // a return statement just has an expression to be evaluated and returned
    expr_t *expr; 
} return_stmt_t;

// Note that a declaration is different from an assign statement. A declaration is the first time
// some variable is referenced (e.g., int x; or int x = 0;), but an assignment occurs when the
// variable is actually assigned a value.
typedef struct {
    // TODO this won't always be a builtin type, but it's okay for now.
    builtin_type_t type;    
    string_t *name;

    // This expression is optional. In a statement like "int a;", there's no init_expr."
    expr_t *init_expr;
} declare_stmt_t;

typedef struct {
    enum {
        STMT_RETURN,
        STMT_DECLARE,
        STMT_EXPR,
    } type;
    union {
        return_stmt_t *ret;
        declare_stmt_t *declare;

        // Apparently, standalone expressions (ie, not in an assign or return statement) is totally
        // valid C. GCC will compile it, but will warn you.
        expr_t *expr;
    };
} stmt_t;

// An environment contains all the variable idents mapped to their types in the current scope
// The pointer to the outer scope lets the user search other scopes to see if a variable is defined
typedef struct env {
    map_t *map;
    struct env *outer;
} env_t;

typedef struct {
    string_t *name;
    list_t *params;
    list_t *stmts;
    builtin_type_t ret_type;
    env_t *env; // maps 
} fn_def_t;

// a program is defined as a list of functions
// the asm that is emitted will only be blocks of functions. it's the linkers job to make sure they
// all call the correct memory address or whatever.
typedef struct {
    list_t *fn_defs;
} program_t;

program_t *parse(list_t *tokens);
void print_ast(program_t *prog);

#endif
