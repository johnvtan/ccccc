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
    if (!input || !input->len)
        return -1;

    list_t *tokens = tokenize(input);
    if (!tokens || !tokens->len)
        return -1;

    program_t *prog = parse(tokens);
    if (!prog || !prog->fn_defs || !prog->fn_defs->len)
        return -1;

    printf("PARSING GOOD\n");
    print_ast(prog);
    list_t *instrs = gen_pseudo_asm(prog);
    if (!instrs || !instrs->len)
        return -1;

    print_asm(instrs);
    return 0;
}
