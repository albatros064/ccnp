#include "utilities.h"

#include "preprocess.h"

#include <stdio.h>

void print_token(token *_token) {
    if (_token->type == TOT_STRING) {
        putchar('"');
    }
    if (_token->type == TOT_CHARACTER) {
        putchar('\'');
    }
    if (_token->type == TOT_SYSFILE) {
        putchar('<');
    }

    printf("%s", _token->content);

    if (_token->type == TOT_STRING) {
        putchar('"');
    }
    if (_token->type == TOT_CHARACTER) {
        putchar('\'');
    }
    if (_token->type == TOT_SYSFILE) {
        putchar('>');
    }
}

void print_lines(list *lines) {
    logical_line *_line;
    token *_token;
    list_iterator *line_iterator;
    list_iterator *token_iterator;
    char c;
    int i, depth;
    int state, new_state;

    state = new_state = depth = 0;

    line_iterator = lst_iterator(lines);

    while ( (_line = (logical_line *) lst_next(line_iterator) ) ) {
        print_line(_line->tokens);

        if (!lst_is_end(line_iterator) ) {
            putchar(' ');
        }
    }

    putchar('\n');
}

void print_line(list *_line) {
    token *_token;
    list_iterator *token_iterator;

    token_iterator = lst_iterator(_line);
    while ( (_token = (token *) lst_next(token_iterator) ) ) {
        print_token(_token);

        if (!lst_is_end(token_iterator) ) {
            putchar(' ');
        }
    }
}


void print_macros(hash_map *macros) {
    hash_map_iterator *iterator;
    list_iterator *token_iterator;
    hash_map_pair *pair;
    token *_token;

    iterator = hmp_iterator(macros);

    while ( (pair = hmp_next(iterator) ) ) {
        printf("%s: ", (char *) pair->key);
        token_iterator = lst_iterator( ( (macro *) pair->datum)->tokens);

        while ( (_token = (token *) lst_next(token_iterator) ) ) {
            print_token(_token);
            if (!lst_is_end(token_iterator) ) {
                putchar(' ');
            }
        }

        putchar('\n');
    }
}
