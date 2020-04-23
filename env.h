#ifndef ENV_H
#define ENV_H

#include "map.h"
#include "list.h"
#include "ast.h"

// An environment contains all the variable idents mapped to their types in the current scope
// The pointer to the parent scope lets the user search other scopes to see if a variable is defined
// The list of children environments is useful for allocating homes.
typedef struct env {
    // Maps from var name (string) to var_t type, which contains home info.
    map_t *homes;
    list_t *children; // list of children environments
    struct env *parent;
} env_t;

env_t *env_new(env_t *parent);

// Searches for var in current env. If not found, keeps checking parents until it is found or
// we run out of envs.
void *env_get(env_t *env, string_t *var);

// Adds var to env at current level. 
void env_add(env_t *env, string_t *var, builtin_type_t type);

#endif
