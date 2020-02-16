#include "env.h"

env_t *env_new(env_t *parent) {
    env_t *new_env = malloc(sizeof(env_t));
    if (!new_env)
        return NULL;

    new_env->parent = parent;
    new_env->children = list_new();
    new_env->vars = list_new();
    new_env->homes = map_new();
    new_env->sp_offset = 0;
    return new_env;
}
