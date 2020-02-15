#ifndef ALLOC_H
#define ALLOC_H

#include <stddef.h>
#include "env.h"

/*
 * Allocates variable homes based on function definitions/scoping. For now, only does stack allocation.
 */

// TODO figure out what this needs
env_t *alloc_homes(void);
#endif
