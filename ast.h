#ifndef AST_H
#define AST_H

#include "string.h"
#include "list.h"

typedef enum builtin_type {
    TYPE_INT,
    TYPE_VOID,
} builtin_type_t;

typedef enum stmt_type {
    STMT_RETURN,
    STMT_FN_DEF,
} stmt_type_t;

typedef enum expr_type {
    INT_LITERAL,
    CHAR_LITERAL,
    BIN_OP,
} expr_type_t;

typedef struct expr {
    expr_type_t type;
    union {
        int integer;
        char character;
    };
} expr_t;

typedef struct return_stmt {
    // a return statement just has an expression to be evaluated and returned
    expr_t *expr; 
} return_stmt_t;

typedef struct fn_def_stmt {
    string_t *name;
    list_t *params;
    list_t *stmts;
    builtin_type_t ret_type;
} fn_def_stmt_t;

typedef struct stmt {
    stmt_type_t type; 
    union {
        fn_def_stmt_t fn_def;
        return_stmt_t ret;
    };
} stmt_t;

// a program is defined as a list of statements
typedef struct program {
    list_t *fn_defs;
} program_t;

program_t *parse(list_t *tokens);
#endif
