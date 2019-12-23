#ifndef TOKENIZE_H
#define TOKENIZE_H

#include "string.h"
#include "list.h"

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

typedef enum token_type {
    // literal tokens
    TOK_INT_LIT = 1,
    TOK_CHAR_LIT,

    // keyword tokens
    TOK_IDENT,
    TOK_RETURN,
    TOK_INT_TYPE,
    TOK_CHAR_TYPE,
    TOK_VOID_TYPE,
    TOK_PLUS,
    TOK_MINUS,
    TOK_MULT,
    TOK_DIV,
    TOK_ASSIGN,

    // idk what these are. also keywords sort of?
    TOK_OPEN_BRACE,
    TOK_CLOSE_BRACE,
    TOK_OPEN_PAREN,
    TOK_CLOSE_PAREN,
    TOK_OPEN_BRACKET,
    TOK_CLOSE_BRACKET,
    TOK_SEMICOLON,
} token_type_t;

typedef struct token {
    token_type_t type;
    union {
        int int_literal;
        char char_literal;
        string_t *ident;
    };
} token_t;

// TODO change this to use string_t
list_t *tokenize(string_t *input);
void print_token(token_t *token);
#endif
