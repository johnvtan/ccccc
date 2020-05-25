#ifndef ENV_H
#define ENV_H

#include <stdbool.h>
#include "list.h"
#include "map.h"
#include "asm_types.h"

typedef enum {
    TYPE_UNRECOGNIZED,
    TYPE_INT,
    TYPE_CHAR,
    TYPE_VOID,
} builtin_type_t;

typedef struct {
    builtin_type_t type;
    mem_loc_t home;
    bool declared;
} var_info_t;

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
var_info_t *env_get(env_t *env, string_t *var);

// Searches for first declared var starting in current env. If not found, keeps checking
// parents until it is found or we run out of envs. If it is found but undeclared, keeps searching.
var_info_t *env_get_declared(env_t *env, string_t *var);

// Adds var to env at current level. 
void env_add(env_t *env, string_t *var, builtin_type_t type, bool declared);

#endif
