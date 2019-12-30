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
    list_t *tokens = tokenize(input);

    program_t *prog = parse(tokens);
    if (!prog)
        return -1;

    list_t *instrs = gen_pseudo_asm(prog);
    if (!instrs || instrs->len == 0) {
        printf("OH NO\n");
        return -1;
    }

    print_asm(instrs);
    return 0;
}
