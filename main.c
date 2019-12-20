#include <stdio.h>
#include <string.h>
#include "list.h"
#include "string.h"

// returns a list of tokens
list_t *tokenize(char *buf, unsigned int len) {
    if (len == 0) {
        return NULL;
    }
    string_t *token = new_string();
    list_t *token_list = new_list();
    char c;
    while ((c = *buf) != '\0') {
        if (c == ' ' || c == '\n') {
            if (token->len > 0) {
                list_push(token_list, token);
                token = new_string();
            }
        } else if (c == '{'
                || c == '}'
                || c == '('
                || c == ')'
                || c == ';') {
            if (token->len > 0) {
                list_push(token_list, token);
            }
            token = new_string();
            string_add(token, c);
            list_push(token_list, token);
            token = new_string();
        } else {
            string_add(token, c);
        }
        buf++;
    }
    return token_list;
}

int main(void) {
    char *text = "int main(void) { return 0; }";
    list_t *tokens = tokenize(text, strlen(text));
    string_t *curr;
    while ((curr = list_pop(tokens))) {
        printf("%s, ", string_get(curr));
        string_free(curr);
    }
    printf("\n");
    list_free(tokens);
    return 0;
}
