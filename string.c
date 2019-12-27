#include "string.h"

#define STRING_DEFAULT_CAPACITY (8)

static void realloc_string(string_t *string) {
    string->capacity *= 2;
    string->buf = realloc(string->buf, string->capacity);
}

string_t *file_to_string(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return NULL;
    char buf[1024];
    string_t *new = string_new(); 
    while (1) {
        int nread = fread(buf, 1, sizeof(buf), fp);
        if (nread == 0)
            break;
        string_append(new, buf, nread);
    }

    string_append(new, "\n\n", 2);
    return new;
}

string_t *string_new(void) {
    string_t *new = malloc(sizeof(string_t));
    new->buf = malloc(sizeof(char) * STRING_DEFAULT_CAPACITY);
    new->len = 0;
    new->capacity = STRING_DEFAULT_CAPACITY;
    return new;
}

void string_add(string_t *string, char c) {
    if (string->len >= string->capacity) {
        realloc_string(string);
    }
    string->buf[string->len++] = c;
}

void string_append(string_t *string, char *s, int len) {
    while (*s && len--) {
        string_add(string, *s++);
    }
}

char *string_get(string_t *string) {
    string_add(string, '\0');
    return string->buf;
}

void string_free(string_t *string) {
    if (!string)
        return;
    if (string->buf)
        free(string->buf);
    free(string);
}

