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
    program_t *prog = parse(tokens);
    if (!prog)
        printf("UHOHSPAGHETIOS\n");

    print_ast(prog);
    return 0;
}
