#ifndef COMPILE_H
#define COMPILE_H

#include <stdio.h>

#include "list.h"
#include "string.h"
#include "map.h"
#include "env.h"

#include "tokenize.h"
#include "ast.h"
#include "asm.h"

#define UNREACHABLE(msg) \
    printf("%s line %d: Reached unreachable branch with message - %s\n", __FILE__, __LINE__, msg);\
    exit(-1);

#ifdef DEBUG
#define debug(...) do { fprintf(stderr, __VA_ARGS__); } while(0) 
#else
#define debug(...) do {} while(0)
#endif

list_t *tokenize(string_t *input);
program_t *parse(list_t *tokens);

// Allocates homes in place.
void alloc_homes(env_t *global);
list_t *gen_asm(program_t *prog, env_t *global_env);
void print_asm(list_t *output);

void print_token(token_t *token);
void print_ast(program_t *prog);
#endif
