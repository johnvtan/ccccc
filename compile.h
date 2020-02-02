#ifndef COMPILE_H
#define COMPILE_H

#include <stdio.h>
#include "list.h"
#include "string.h"
#include "map.h"
#include "tokenize.h"
#include "ast.h"
#include "asm.h"
#include "output.h"

#define UNREACHABLE(msg) \
    printf("%s line %d: Reached unreachable branch with message - %s\n", __FILE__, __LINE__, msg);\
    exit(-1);

#ifdef DEBUG
#define debug(...) do { fprintf(stderr, __VA_ARGS__); } while(0) 
#else
#define debug(...) do {} while(0)
#endif

#endif
