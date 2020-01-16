#ifndef STRING_H
#define STRING_H

#include <stdlib.h>
#include <stdio.h>

typedef struct {
    char *buf;
    int len;
    int capacity;
} string_t;

string_t *file_to_string(char *filename);
string_t *string_new(void);
int string_init(string_t *string);
void string_add(string_t *string, char c);
void string_append(string_t *string, char *s, int len);
char *string_get(string_t *string);
void string_free(string_t *string);
int string_eq(string_t *s1, string_t *s2);

#endif
