#include "preprocess.h"
#include "compile.h"
#include "container.h"

#include <stdio.h>

#define SOMETHING_INTERESTING is about>to< <<happen>>here

void print_lines(list *);
void print_macros(hash_map *);

int main(int argc, char **argv) {
    list *lines;
    preprocessor_state *state = preprocess_init();
    preprocess(state, argv[1]);

    print_lines(state->lines);
    /* print_macros(state->macros); */

    return 0;
}

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
        token_iterator = lst_iterator(_line->tokens);

        while ( (_token = (token *) lst_next(token_iterator) ) ) {
            print_token(_token);

            if (!lst_is_end(token_iterator) ) {
                putchar(' ');
            }
        }

        if (!lst_is_end(line_iterator) ) {
            putchar(' ');
        }
    }

    putchar('\n');
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
