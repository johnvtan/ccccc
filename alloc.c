#include "compile.h"
#include <assert.h>

static void alloc_scoped_env(env_t *env, int offset) {
    list_for_each(env->vars, curr_node) {
        var_t *var = (var_t*)curr_node->data; 

        // TODO give the var a home
        var->home.reg = REG_RBP;
        var->home.offset = offset;

        debug("Found var %s: home = %d(rbp)\n", string_get(var->name), offset);

        // This seems kinda dirty, but useful-ish for easier lookup
        // TODO now i just have two lists for the same thing
        map_set(env->homes, var->name, var);

        if (offset < env->sp_offset) {
            env->sp_offset = offset;
        }

        // TODO actually find the size of each variable
        // For now, assume everything is 64 bits.
        offset -= 8;
    }

    // TODO rewrite this env sucks
    if (env->children->len == 0) {
        env_t *curr = env;
        while (curr->parent) {
            if (env->sp_offset < curr->sp_offset)
                curr->sp_offset = env->sp_offset;
            curr = curr->parent;
        }
    }

    list_for_each(env->children, curr_node) {
        env_t *child_env = (env_t*)curr_node->data;
        alloc_scoped_env(child_env, offset);
    }

}

void alloc_homes(env_t *global) {
    assert(global->parent == NULL); 
    assert(global->children->len > 0);

    // for now global variables are homeless. they can't live on the stack.
    list_for_each(global->children, curr_node) {
        debug("Iterating through global children\n");
        env_t *curr_env = (env_t *)curr_node->data;
        alloc_scoped_env(curr_env, -8);
    }
}

