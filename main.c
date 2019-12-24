#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compile.h"

int main(void) {
    char *text = "int main(void) {\
                  int x=2+2345;\
                  return x;\
                  }";

    string_t *input = string_new();
    string_append(input, text, strlen(text));
    list_t *tokens = tokenize(input);

    token_t *token;
    while ((token = list_pop(tokens))) {
        print_token(token);
    }
    return 0;
}
