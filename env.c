#include "env.h"

env_t *env_new(env_t *parent) {
    env_t *new_env = malloc(sizeof(env_t));
    if (!new_env)
        return NULL;

    new_env->parent = parent;
    new_env->children = list_new();
    new_env->homes = map_new();

    if (parent)
        list_push(parent->children, new_env);

    return new_env;
}

void *env_get(env_t *env, string_t *var) {
    void *ret = NULL;
    while (env) {
        if ((ret = map_get(env->homes, var)))
            return ret;
        env = env->parent;
    }
    return NULL;
}

void env_add(env_t *env, string_t *name, builtin_type_t type) {
    var_info_t *info = malloc(sizeof(var_info_t)); 
    info->type = type;
    info->declared = false;
    map_set(env->homes, name, info);
}
