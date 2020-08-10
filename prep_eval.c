#include "prep_eval.h"
#include "message.h"
#include "errors.h"

#include "utilities.h"

#include <stdlib.h>
#include <stdio.h>

evaluation_state *eval_init() {
    evaluation_state *state = (evaluation_state *) malloc(sizeof(evaluation_state));
    state->evaluation = st_create(null_free, EVAL_STACK_QUANTUM);
    state->operators  = st_create(null_free, EVAL_STACK_QUANTUM);

    state->operator_last = 1;

    return state;
}

int8_t eval_add_number(evaluation_state *state, token *number) {
    evaluation_node *node = (evaluation_node *) malloc(sizeof(evaluation_node));

    node->_token = number;
    node->type = EVAL_TYPE_NUMBER;
    node->strength = 0;

    st_push(state->evaluation, node);

    state->operator_last = 0;

    return 0;
}
int8_t eval_add_operator(evaluation_state *state, token *operator) {
    char a, b;
    evaluation_node *node;

    a = operator->content[0];
    b = operator->content[1];

    if (a == ')') {
        evaluation_node *paren;

        while (!st_empty(state->operators) && ((evaluation_node *) st_top(state->operators))->type != EVAL_TYPE_PAREN) {
            st_push(state->evaluation, st_pop(state->operators));
        }
        if (st_empty(state->operators)) {
            return -1;
        }
        free(st_pop(state->operators));
        return 0;
    }

    node = (evaluation_node *) malloc(sizeof(evaluation_node));
    node->_token = operator;
    node->type = EVAL_TYPE_OP_L2;

    switch (a) {
        case '(':
            node->type     = EVAL_TYPE_PAREN;
            node->strength = EVAL_TYPE_PAREN;
            break;
        case '!':
            if (b == '=') {
                node->strength = 10;
                break;
            }
            /* else fall through */
        case '~':
            node->type = EVAL_TYPE_OP_R1;
            node->strength = 17;
            break;
        case '+':
        case '-':
            if (state->operator_last) {
                node->type = EVAL_TYPE_OP_R1;
                node->strength = 17;
            }
            else {
                node->strength = 14;
            }
            break;
        case '*':
        case '/':
        case '%':
            node->strength = 15;
            break;
        case '<':
        case '>':
            if (a == b) {
                node->strength = 13;
            }
            else {
                node->strength = 11;
            }
            break;
        case '=':
            if (a != b) {
                return -1;
            }
            node->strength = 10;
            break;
        case '&':
            if (b == '&') {
                node->strength = 6;
            }
            else {
                node->strength = 9;
            }
            break;
        case '|':
            if (b == '|') {
                node->strength = 5;
            }
            else {
                node->strength = 7;
            }
            break;
        case '^':
            node->strength = 8;
            break;
        default:
            return -1;
    }

    while (!st_empty(state->operators) && node->type != EVAL_TYPE_PAREN) {
        evaluation_node *top = st_top(state->operators);
        if (top->type == EVAL_TYPE_PAREN) {
            break;
        }
        if (top->strength > node->strength || (top->strength == node->strength && top->type & EVAL_TYPE_OP_LTR)) {
            st_push(state->evaluation, st_pop(state->operators));
        }
        else {
            break;
        }
    }

    st_push(state->operators, node);

    state->operator_last = 1;

    return 0;
}

int8_t eval_final(evaluation_state *state) {
    while (!st_empty(state->operators)) {
        if (((evaluation_node *) st_top(state->operators))->type == EVAL_TYPE_PAREN) {
            return -1;
        }
        st_push(state->evaluation, st_pop(state->operators));
    }
    return 0;
}

int8_t _eval_parse_int32(token *, int32_t *);

int8_t eval_rpn(evaluation_state* state, int32_t *ret) {
    evaluation_node *node;
    int32_t l, r;
    uint8_t is_binary, is_rtl;
    int8_t  ret_code;
    char a, b;
    evaluation_node *iter_token;

    if (st_empty(state->evaluation)) {
        return ERR_EVAL_RPN_EMPTY;
    }

    node = st_pop(state->evaluation);

    if (node->type == EVAL_TYPE_NUMBER) {
        ret_code = _eval_parse_int32(node->_token, ret);
        if (ret_code < 0) {
            free(node);
        }
        return ret_code;
    }

    is_binary = node->type & EVAL_TYPE_OP_BINARY;
    is_rtl    = node->type & EVAL_TYPE_OP_RTL;

    a = node->_token->content[0];
    b = node->_token->content[1];

    free(node);

    ret_code = eval_rpn(state, &r);
    if (ret_code < 0) {
        return ret_code;
    }
    if (is_binary) {
        ret_code = eval_rpn(state, &l);
        if (ret_code < 0) {
            return ret_code;
        }
    }

    switch (a) {
        case '+':
            if (is_binary) {
                *ret = l + r;
            }
            else {
                *ret = r;
            }
            break;
        case '-':
            if (is_binary) {
                *ret = l - r;
            }
            else {
                *ret = -r;
            }
            break;
        case '*':
            *ret = l * r;
            break;
        case '/':
            if (r == 0) {
                return ERR_EVAL_DIV_ZERO;
            }
            *ret = l / r;
            break;
        case '%':
            *ret = l % r;
            break;
        case '!':
            if (is_binary) {
                *ret = l != r;
            }
            else {
                *ret = !r;
            }
            break;
        case '~':
            *ret = ~r;
            break;
        case '<':
            if (a == b) {
                *ret = l << r;
            }
            else {
                *ret = l <= r;
            }
            break;
        case '>':
            if (a == b) {
                *ret = l >> r;
            }
            else {
                *ret = l >= r;
            }
            break;
        case '=':
            *ret = l == r;
            break;
        case '&':
            if (a == b) {
                *ret = l && r;
            }
            else {
                *ret = l & r;
            }
            break;
        case '|':
            if (a == b) {
                *ret = l || b;
            }
            else {
                *ret = l | b;
            }
            break;
        case '^':
            *ret = l ^ r;
            break;
        default:
            return ERR_EVAL_OPERATOR;
    }
    
    return 0;
}

int8_t _eval_parse_int32(token *_token, int32_t *ret) {
    char c;
    int8_t parse_mode;
    char s = 0;

    parse_mode = EVAL_NUMBER_MODE_NONE;

    if (_token->type == TOT_CHARACTER) {
        /* TODO: escape sequences, multi-char values */
        *ret = _token->content[0];
        return 0;
    }

    *ret = 0;
    for (int i = 0; i < _token->length; i++) {
        c = _token->content[i];

        if (c == '.') {
            return ERR_EVAL_NUMBER_FLOAT;
        }

        if (c > 0x40) {
            c &= 0x5f;
        }

        switch (parse_mode) {
            case EVAL_NUMBER_MODE_NONE:
                if (c == '0') {
                    parse_mode = EVAL_NUMBER_MODE_OCTAL;
                }
                else if (c > '0' && c <= '9') {
                    parse_mode = EVAL_NUMBER_MODE_DECIMAL;
                    goto parse_eval_number_decimal;
                }
                else {
                    return ERR_EVAL_NUMBER_FLOAT;
                }
                break;
            case EVAL_NUMBER_MODE_OCTAL:
                /* Check if this is actuall a hexadecimal number */
                if (i == 1 && c == 'X') {
                    parse_mode = EVAL_NUMBER_MODE_HEX;
                }
                else if (c >= '0' && c <= '7') {
                    *ret <<= 3;
                    *ret += c - '0';
                }
                else if (c == '8' || c == '9') {
                    return ERR_EVAL_NUMBER_OCTAL;
                }
                else {
                    s = c;
                    parse_mode = EVAL_NUMBER_MODE_SUFFIX;
                }
                break;
            case EVAL_NUMBER_MODE_HEX:
                if (c >= 'A' && c <= 'F') {
                    *ret <<= 4;
                    *ret += c - 'A' + 10;
                }
                else if (c >= '0' && c <= '9') {
                    *ret <<= 4;
                    *ret += c - '0';
                }
                else {
                    s = c;
                    parse_mode = EVAL_NUMBER_MODE_SUFFIX;
                }
                break;
            case EVAL_NUMBER_MODE_DECIMAL:
            parse_eval_number_decimal:
                if (c >= '0' && c <= '9') {
                    *ret *= 10;
                    *ret += c - '0';
                }
                else {
                    s = c;
                    parse_mode = EVAL_NUMBER_MODE_SUFFIX;
                }
                break;
            case EVAL_NUMBER_MODE_SUFFIX:
            default:
                if (s != 'U' || c != 'L') {
                    return ERR_EVAL_NUMBER_SUFFIX;
                }
                s = c;
                break;
        }
    }
    if (s && s != 'L') {
        return ERR_EVAL_NUMBER_SUFFIX;
    }

    return 0;
}
