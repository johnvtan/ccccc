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

    if (parent) {
        list_push(parent->children, new_env);
    }
    return new_env;
}

void *env_get(env_t *env, string_t *var) {
    void *ret = NULL;
    while (env) {
        if ((ret = map_get(env->homes, var)))
            break;
        env = env->parent;
    }
    return ret;
}
