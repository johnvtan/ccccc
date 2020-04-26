#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compile.h"

// From parse.c
extern env_t *global_env;

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

    debug("Tokenizing...\n");
    list_t *tokens = tokenize(input);
    if (!tokens || !tokens->len)
        return -1;

    debug("Parsing...\n");
    program_t *prog = parse(tokens);
    if (!prog || !prog->fn_defs || !prog->fn_defs->len)
        return -1;

    /*
     * TODO - do variable allocation here.
     */
    debug("Allocating variable homes...\n");
    alloc_homes(prog);

    debug("Generating asm...\n");
    list_t *instrs = gen_asm(prog);
    if (!instrs || !instrs->len)
        return -1;

    debug("Outputting asm...\n");
    print_asm(instrs);
    return 0;
}
