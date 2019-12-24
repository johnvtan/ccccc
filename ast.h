#ifndef AST_H
#define AST_H

#include "string.h"
#include "list.h"

typedef enum builtin_type {
    TYPE_UNRECOGNIZED,
    TYPE_INT,
    TYPE_VOID,
} builtin_type_t;

typedef enum stmt_type {
    STMT_RETURN,
    STMT_ASSIGN,
} stmt_type_t;

typedef enum expr_type {
    INT_LITERAL,
    CHAR_LITERAL,
    BIN_OP,
} expr_type_t;

typedef struct var {
    builtin_type_t type;
    string_t name;
} var_t;

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

typedef struct stmt {
    stmt_type_t type; 
    union {
        return_stmt_t *ret;
        // TODO fill in with other statements
    };
} stmt_t;

typedef struct fn_def {
    string_t *name;
    list_t *params;
    list_t *stmts;
    builtin_type_t ret_type;
} fn_def_t;


// a program is defined as a list of functions
// the asm that is emitted will only be blocks of functions. it's the linkers job to make sure they
// all call the correct memory address or whatever.
typedef struct program {
    list_t *fn_defs;
} program_t;

program_t *parse(list_t *tokens);
#endif
