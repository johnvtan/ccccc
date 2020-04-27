#include "compile.h"

typedef struct {
    char *keyword;
    token_type_t type;
} keyword_pair_t;

typedef struct {
    char c;
    token_type_t type;
} special_char_pair_t;

static const keyword_pair_t keywords[] = {
    {"return", TOK_RETURN},
    {"int", TOK_INT_TYPE},
    {"char", TOK_CHAR_TYPE},
    {"void", TOK_VOID_TYPE},
    {"&&", TOK_AND},
    {"||", TOK_OR},
    {"==", TOK_EQ},
    {"!=", TOK_NE},
    {"<=", TOK_LTE},
    {">=", TOK_GTE},
    {"++", TOK_INCREMENT},
    {"--", TOK_DECREMENT},
    {"+=", TOK_PLUS_EQ},
    {"-=", TOK_MINUS_EQ},
    {"if", TOK_IF},
    {"else", TOK_ELSE},
    {"for", TOK_FOR},
    {"while", TOK_WHILE},
    {"do", TOK_DO},
    {"break", TOK_BREAK},
    {"continue", TOK_CONTINUE},
    {NULL, 0},
};

static const special_char_pair_t special_chars[] = {
    {'+', TOK_PLUS},
    {'-', TOK_MINUS},
    {'*', TOK_MULT},
    {'/', TOK_DIV},
    {'=', TOK_ASSIGN},
    {'{', TOK_OPEN_BRACE},
    {'}', TOK_CLOSE_BRACE},
    {'(', TOK_OPEN_PAREN},
    {')', TOK_CLOSE_PAREN},
    {'[', TOK_OPEN_BRACKET},
    {']', TOK_CLOSE_BRACKET},
    {';', TOK_SEMICOLON},
    {'-', TOK_MINUS},
    {'~', TOK_TILDE},
    {'!', TOK_BANG},
    {'<', TOK_LT},
    {'>', TOK_GT},
    {'?', TOK_QUESTION},
    {':', TOK_COLON},
    {'%', TOK_MODULO},
    {0, 0},
};

// returns true if s1 starts with s2
static bool startswith(char *s1, char *s2) {
    return !strncmp(s1, s2, strlen(s2));
}

static bool is_whitespace(char c) {
    switch (c) {
        case ' ':
        case '\n':
        case '\t':
            return true;
        default:
            return false;
    }
}

static int string_literal(char *p, token_t *token) {
    if (*p != '"')
        return -1;
    (void)token;
    return -1;
}

static int char_literal(char *p, token_t *token) {
    if (*p != '\'')
        return -1;
    (void)token;
    return -1;
}

// TODO only handles decimal integers
static int number_literal(char *p, token_t *token) {
    if (!isdigit(*p))
        return -1;
    char *end = p;
    token->type = TOK_INT_LIT;
    token->int_literal = strtol(p, &end, 10);
    return end - p;
}

static int keyword(char *p, token_t *token) {
    int i = 0;
    while (keywords[i].keyword != NULL) {
        if (startswith(p, keywords[i].keyword)) {
            token->type = keywords[i].type;
            return strlen(keywords[i].keyword);
        }
        i++;
    }
    return -1;
}

static int special_char(char *p, token_t *token) {
    int i = 0;
    while (special_chars[i].c) {
        if (*p == special_chars[i].c) {
            token->type = special_chars[i].type;
            return 1;
        }
        i++;
    }
    return -1;
}

static int identifier(char *p, token_t *token) {
    // identifiers can only begin with a letter or underscore
    if (!isalpha(*p) && *p != '_')
        return -1;

    // an identifier can consist of numbers, letters, underscores
    token->type = TOK_IDENT;
    token->ident = string_new();
    while (*p) {
        if (!isalpha(*p) && !isdigit(*p) && *p != '_')
            break;
        string_add(token->ident, *p++);
    }
    return token->ident->len;
}

static void unrecognized_token(char *s) {
    fprintf(stderr, "UNRECOGNIZED TOKEN IN INPUT: %s\n", s);
    exit(-1);
}

// returns a list of tokens
list_t *tokenize(string_t *input) {
    if (!input || input->len == 0)
        return NULL;

    char *buf = string_get(input);
    list_t *token_list = list_new();
    token_t *curr_token;

    int advance;
    while (*buf) {
        if (is_whitespace(*buf)) {
            buf++;
            continue;
        }

        advance = 0;
        
        // freeing stuff is for squares
        curr_token = malloc(sizeof(token_t));

        advance = string_literal(buf, curr_token);
        if (advance > 0)
            goto next;

        advance = number_literal(buf, curr_token);
        if (advance > 0)
            goto next;

        advance = char_literal(buf, curr_token);
        if (advance > 0)
            goto next;

        advance = keyword(buf, curr_token);
        if (advance > 0) {
            char next_char = *(buf + advance);
            // TODO how to make this less hacky?
            if (curr_token->type == TOK_RETURN 
                && !is_whitespace(next_char)
                && next_char != ';') {
                printf("Bad input program: malformed keyword\n");
                exit(-1);
            }
            goto next;
        }

        advance = special_char(buf, curr_token);
        if (advance > 0)
            goto next;

        advance = identifier(buf, curr_token);
        if (advance > 0)
            goto next;

        unrecognized_token(buf);

next:
        list_push(token_list, curr_token);
        buf += advance;
    }

    return token_list;
}

void print_token(token_t *token) {
    if (!token)
        return;

    if (token->type == TOK_INT_LIT) {
        printf("integer literal: %d\n", token->int_literal);
        return;
    }

    if (token->type == TOK_CHAR_LIT) {
        printf("char literal: %c\n", token->char_literal);
        return;
    }

    if (token->type == TOK_IDENT) {
        printf("identifier: %s\n", string_get(token->ident));
        return;
    }

    // try special chars and keywords now
    keyword_pair_t keyword;
    int i = 0;
    do {
        keyword = keywords[i++];
        if (token->type == keyword.type) {
            printf("keyword: %s\n", keyword.keyword);
            return;
        }
    } while (keyword.keyword);

    special_char_pair_t special;
    i = 0;
    do {
        special = special_chars[i++];
        if (token->type == special.type) {
            printf("special char: %c\n", special.c);
            return;
        }
    } while (special.c);

    printf("UNKNOWN TOKEN!!!!!!\n");
}
