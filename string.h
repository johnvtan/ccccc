#ifndef STRING_H
#define STRING_H

#include <stdlib.h>

typedef struct {
    char *buf;
    int len;
    int capacity;
} string_t;

string_t *string_new(void);
void string_add(string_t *string, char c);
void string_append(string_t *string, char *s, int len);
char *string_get(string_t *string);
void string_free(string_t *string);

#endif
