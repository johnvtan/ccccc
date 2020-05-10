#include "compile.h"
#include <assert.h>

static int alloc_scoped_env(env_t *env, int offset) {
    pair_t *pair;

    // Allocate homes for each variable at the current level
    map_for_each(env->homes, pair) {
        var_info_t *v = pair->value;
        string_t *name = pair->key;

        offset -= 8;
        debug("Found variable %s\n", string_get(name));
        v->home.reg = REG_RBP;
        v->home.offset = offset;
    }

    // Recursively allocate vars for all child envs
    env_t *child_env;
    list_for_each(env->children, child_env) {
        int child_offset = alloc_scoped_env(child_env, offset); 
        if (child_offset < offset)
            offset = child_offset;
    }

    return offset;
}

void alloc_homes(program_t *prog) {
    pair_t *pair;
    map_for_each(prog->fn_defs, pair) {
        fn_def_t *curr_fn = pair->value;
        debug("allocating for function %s\n", string_get(curr_fn->name));
        curr_fn->sp_offset = alloc_scoped_env(curr_fn->env, 0);
    }
}

