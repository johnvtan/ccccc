#include "compile.h"
#include <assert.h>

static uint64_t alloc_scoped_env(env_t *env, uint64_t offset) {
    pair_t *pair;

    // Allocate homes for each variable at the current level
    map_for_each(env->homes, pair) {
        var_info_t *v = pair->value;
        string_t *name = pair->key;
        debug("Found variable %s\n", string_get(name));
        v->home.reg = REG_RBP;
        v->home.offset = offset;
        offset += 8;
    }

    // Recursively allocate vars for all child envs
    env_t *child_env;
    list_for_each(env->children, child_env) {
        uint64_t child_offset = alloc_scoped_env(child_env, offset); 
        if (child_offset > offset)
            offset = child_offset;
    }

    return offset;
}

void alloc_homes(program_t *prog) {
    fn_def_t *curr_fn;
    list_for_each(prog->fn_defs, curr_fn) {
        debug("allocating for function %s\n", string_get(curr_fn->name));
        curr_fn->sp_offset = alloc_scoped_env(curr_fn->env, 8);
    }
}

