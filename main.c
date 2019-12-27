#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compile.h"

void usage(void) {
    printf("COMPILERBABY <filename>\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage();
        return -1;
    }
   
    char *filename = argv[1];
    string_t *input = file_to_string(filename);
    if (!input) {
        return -1;
    }
    printf("input\n%s\n", string_get(input));
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
