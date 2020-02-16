#ifndef ENV_H
#define ENV_H

#include "map.h"
#include "list.h"

// An environment contains all the variable idents mapped to their types in the current scope
// The pointer to the parent scope lets the user search other scopes to see if a variable is defined
// The list of children environments is useful for allocating homes.
typedef struct env {
    list_t *vars;
    map_t *homes;
    list_t *children; // list of children environments
    struct env *parent;
    int sp_offset;
} env_t;

env_t *env_new(env_t *parent);

#endif
