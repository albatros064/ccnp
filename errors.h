#ifndef ERRORS_H
#define ERRORS_H

#define ERR_EVAL_GENERIC        -1
#define ERR_EVAL_DIV_ZERO       -28
#define ERR_EVAL_RPN_EMPTY      -29
#define ERR_EVAL_OPERATOR       -30
#define ERR_EVAL_NUMBER         -31
#define ERR_EVAL_PAREN          -32

#define ERR_EVAL_NUMBER_FLOAT   -35
#define ERR_EVAL_NUMBER_OCTAL   -36
#define ERR_EVAL_NUMBER_SUFFIX  -37

#define MAX_ERROR_NUMBER 38

#include <stdint.h>
char *get_error_string(int8_t test);

#endif
