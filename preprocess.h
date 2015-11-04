#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#define NULL_TO_SPACE 1
#define DOLLAR_IS_LETTER 0

#include "container.h"

#define TOT_NONE       0
#define TOT_IDENTIFIER 1
#define TOT_NUMBER     2
#define TOT_STRING     3
#define TOT_CHARACTER  4
#define TOT_PUNCTUATOR 5
#define TOT_OTHER      6

#define TOT_SYSFILE   10

#define TOT_EXCEPT   128
#define TOT_BCOMMENT 254
#define TOT_LCOMMENT 255

#define PREP_NONE    0
#define PREP_PEND    1
#define PREP_INCLUDE 2
#define PREP_IF      4
#define PREP_ELSE    6
#define PREP_ELIF    8
#define PREP_ENDIF  10
#define PREP_DEFINE 16
#define PREP_IFDEF  18
#define PREP_IFNDEF 20
#define PREP_UNDEF  22
#define PREP_PRAGMA 32
#define PREP_ERROR  34
#define PREP_LINE   36

#define DIRECTIVE_COUNT 12

#define TOKEN_BUFFER_SIZE 64
#define TOKEN_LIST_SIZE   64


#define TOKEN_LIST_QUANTUM 8

#define PREP_IF_STACK_QUANTUM 8
#define PREP_IF_BASELINE  0
#define PREP_IF_MATCHING  1
#define PREP_IF_INCLUDING 2
#define PREP_IF_ENDING    3

typedef struct {
    int   char_offset;
    int   line_offset;

    unsigned char type;
    char *content;
    int   length;
} token;

typedef struct  {
    int    start_line;
    char  *file;

    list *tokens;
} logical_line;

typedef struct {
    unsigned char token_type,
                  last_type;
    unsigned char token_overflow;
    int   token_length;
    char  token_buffer[TOKEN_BUFFER_SIZE];

    unsigned char directive_line;
    unsigned char generate,
                  suppress_c;
    
    list *lines;

    list *if_stack;

    hash_map *macros;
    hash_map *directives;
} preprocessor_state;

typedef struct {
    uint8_t type;
    char  **args;
    list   *tokens;
} macro;

preprocessor_state *preprocess_init();

int8_t generate_line (preprocessor_state *, unsigned int, char *);
int8_t generate_token(preprocessor_state *, unsigned int, unsigned int);
char *null_terminate(char *, unsigned int);

int8_t preprocess(preprocessor_state *, char *);

void print_lines(list *);

#endif

