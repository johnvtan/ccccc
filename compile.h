#ifndef COMPILE_H
#define COMPILE_H

#include "list.h"
#include "string.h"
#include "tokenize.h"
#include "ast.h"
#include "asm.h"

#define UNREACHABLE(msg) \
    printf("%s line %d: Reached unreachable branch with message - %s\n", __FILE__, __LINE__, msg);\
    exit(-1);

#endif
