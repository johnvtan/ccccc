#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compile.h"

int main(void) {
    char *text = "int main(void) {\
                  return x;\
                  }";

    string_t *input = string_new();
    string_append(input, text, strlen(text));

    list_t *tokens = tokenize(input);
#if 1
    program_t *prog = parse(tokens);
    if (!prog)
        printf("UHOHSPAGHETIOS\n");

    print_ast(prog);
#else
    token_t *token;
    while ((token = list_pop(tokens))) {
        print_token(token);
    }
#endif
    return 0;
}
