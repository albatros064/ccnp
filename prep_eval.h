#ifndef PREP_EVAL_H
#define PREP_EVAL_H

#include "preprocess.h"
#include "container.h"

#include <ctype.h>

#define EVAL_TYPE_OP_BINARY 1
#define EVAL_TYPE_OP_LTR    2
#define EVAL_TYPE_OP_RTL    4

#define EVAL_TYPE_NUMBER 0
#define EVAL_TYPE_OP_L1  2
#define EVAL_TYPE_OP_L2  3
#define EVAL_TYPE_OP_R1  4
#define EVAL_TYPE_OP_R2  5

#define EVAL_TYPE_PAREN 128

#define EVAL_STACK_QUANTUM 8

#define EVAL_ERR                -1
#define EVAL_ERR_DIV_ZERO       -3
#define EVAL_ERR_RPN_EMPTY      -5
#define EVAL_ERR_OPERATOR       -6

#define EVAL_ERR_NUMBER_FLOAT   -32
#define EVAL_ERR_NUMBER_OCTAL   -33
#define EVAL_ERR_NUMBER_SUFFIX  -34

#define EVAL_NUMBER_MODE_NONE    0
#define EVAL_NUMBER_MODE_HEX     1
#define EVAL_NUMBER_MODE_DECIMAL 2
#define EVAL_NUMBER_MODE_OCTAL   3
#define EVAL_NUMBER_MODE_SUFFIX  16

typedef struct {
    uint8_t type;
    uint8_t strength;
    token  *_token;
} evaluation_node;
typedef struct {
    stack  *evaluation;
    stack  *operators;
    uint8_t operator_last;
} evaluation_state;

evaluation_state *eval_init();
int8_t eval_rpn         (evaluation_state *, int32_t*);
int8_t eval_add_number  (evaluation_state *, token *);
int8_t eval_add_operator(evaluation_state *, token *);
int8_t eval_final       (evaluation_state *);

#endif
