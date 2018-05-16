#include "preprocess.h"
#include "message.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

struct {
    char *label;
    uint8_t state;
} _directives[DIRECTIVE_COUNT] = {
    { "define" , PREP_DEFINE  },
    { "include", PREP_INCLUDE },
    { "if"     , PREP_IF      },
    { "else"   , PREP_ELSE    },
    { "endif"  , PREP_ENDIF   },
    { "ifdef"  , PREP_IFDEF   },
    { "ifndef" , PREP_IFNDEF  },
    { "elif"   , PREP_ELIF    },
    { "undef"  , PREP_UNDEF   },
    { "pragma" , PREP_PRAGMA  },
    { "error"  , PREP_ERROR   },
    { "line"   , PREP_LINE    }
};

void line_free (void *);
void token_free(void *);

/*void if_state_free(void *);
if_state *if_state_alloc();*/

void   string_free   (void *);
int8_t string_compare(void *, void *);

void     macro_free(void *);
uint32_t macro_hash(void *, uint32_t);
void     directive_free(void *);
uint32_t directive_hash(void *, uint32_t);

int8_t macro_replacement(preprocessor_state *, token *, int32_t, uint8_t);
int8_t process_directive(preprocessor_state *);
int8_t _retoken_ifdef(preprocessor_state *, logical_line *);

void    if_free(void *);
void    if_push(preprocessor_state *, uint8_t);
uint8_t if_top (preprocessor_state *);
uint8_t if_pop (preprocessor_state *);

int8_t handle_if(preprocessor_state *, logical_line *);

preprocessor_state *preprocess_init() {
    preprocessor_state *state;
    uint8_t i;
    char *k;
    uint8_t *v;
    state = (preprocessor_state *) malloc(sizeof(preprocessor_state) );

    state->token_length = 0;
    state->token_type = TOT_NONE;
    state->last_type  = TOT_NONE;

    state->token_overflow = 0;
    state->token_length   = 0;

    state->directive_line = PREP_NONE;

    state->if_stack   = lst_create(&if_free, PREP_IF_STACK_QUANTUM);

    state->lines      = lst_create(&line_free,     TOKEN_LIST_QUANTUM   );
    state->macros     = hmp_create(&string_free, &macro_free,     &string_compare, &macro_hash,     256);

    /* Initialize the preprocessor directives */
    state->directives = hmp_create(&string_free, &directive_free, &string_compare, &directive_hash, DIRECTIVE_COUNT);
    for (i = 0; i < DIRECTIVE_COUNT; i++) {
        k =  _directives[i].label;
        v = &_directives[i].state;
        hmp_put(state->directives, (void *) k, (void *) v);
    }

    return state;
}

int8_t preprocess(preprocessor_state *state, char *file_name) {
    int   c,
          p;
    char  co[3];
    unsigned char generate,
                  suppress;
    unsigned int line_number, char_number;
    unsigned int tline_number, tchar_number;

    FILE *stream;

    if (!state) {
        state = preprocess_init();
    }

    c = co[0] = co[1] = co[2] = 0;
    generate = suppress = 0;

    line_number = tline_number = tchar_number = 1;
    char_number = 0;

    state->token_type = TOT_NONE;
    state->last_type  = TOT_NONE;

    stream = fopen(file_name, "r");
    if (!stream) {
        message_out(MESSAGE_ERROR, file_name, 0, 0, "File not found.");
        return -1;
    }

    if_push(state, PREP_IF_BASELINE);

    generate_line(state, 1, file_name);

    p = fgetc(stream);
    while ( (c = p) != EOF) {
        p = fgetc(stream);

        char_number++;

        if (p == '\n' && c == '\\') {
            p = fgetc(stream);
            line_number++;
            char_number = 0;
            continue;
        }

#if NULL_TO_SPACE
        if (c == '\0') {
            c = ' ';
        }
#endif

        generate = suppress = 0;
    
        /* Proceed with tokenization */
        switch (state->token_type) {
            case TOT_CHARACTER:
                if (c == '\'' && (co[0] != '\\' || (co[0] == '\\' && co[1] == '\\') ) ) {
                    generate = 1;
                    suppress = 1;
                    state->token_type = TOT_NONE;
                }
                break;
            case TOT_STRING:
                if (c == '"' && (co[0] != '\\' || (co[0] == '\\' && co[1] == '\\') ) ) {
                    generate = 1;
                    suppress = 1;
                    state->token_type = TOT_NONE;
                }
                break;
            case TOT_SYSFILE:
                if (c == '>') {
                    generate = 1;
                    suppress = 1;
                    state->token_type = TOT_NONE;
                }
                break;
            case TOT_IDENTIFIER:
                if (isalnum(c) || c == '_' || (c == '$' && DOLLAR_IS_LETTER) ) {
                    /* continue */
                }
                else if (ispunct(c) && c != '$' && c != '@') {
                    generate = 1;
                    state->token_type = TOT_PUNCTUATOR;
                }
                else if (isspace(c) ) {
                    generate = 1;
                    suppress = 1;
                    state->token_type = TOT_NONE;
                }
                else {
                    generate = 1;
                    state->token_type = TOT_OTHER;
                }
                break;
            case TOT_NUMBER:
                if ( (  isalnum(c) ) ||
                     (  c     == '$' && DOLLAR_IS_LETTER) ||
                     (  c     == '.' || c     == '_') ||
                     ( (c     == '+' || c     == '-') &&
                       (co[0] == 'e' || co[0] == 'E'  || co[0] == 'p' || co[0] == 'P') ) ) {
                    /* continue */
                }
                else {
                    generate = 1;
                    if (c == '"') {
                        suppress = 1;
                        state->token_type = TOT_STRING;
                    }
                    else if (c == '\'') {
                        suppress = 1;
                        state->token_type = TOT_CHARACTER;
                    }
                    else if (isspace(c) ) {
                        suppress = 1;
                        state->token_type = TOT_NONE;
                    }
                    else if (ispunct(c) && c != '$' && c != '@') {
                        state->token_type = TOT_PUNCTUATOR;
                    }
                    else {
                        state->token_type = TOT_OTHER;
                    }
                }
                break;
            case TOT_PUNCTUATOR:
                if (isdigit(c) ) {
                    state->token_type = TOT_NUMBER;
                    if (co[0] != '.') {
                        generate = 1;
                    }
                }
                else if (c == '/' && co[0] == '/') {
                    suppress = 1;
                    state->token_length = 0;
                    state->token_type = TOT_LCOMMENT;
                }
                else if (c == '*' && co[0] == '/') {
                    suppress = 1;
                    state->token_length = 0;
                    state->token_type = TOT_BCOMMENT;
                }
                else if (isalpha(c) || c == '_' || (c == '$' && DOLLAR_IS_LETTER) ) {
                    generate = 1;
                    state->token_type = TOT_IDENTIFIER;
                }
                else if (isspace(c) ) {
                    generate = 1;
                    suppress = 1;
                    state->token_type = TOT_NONE;
                }
                else if (c == '"') {
                    generate = 1;
                    suppress = 1;
                    state->token_type = TOT_STRING;
                }
                else if (c == '\'') {
                    generate = 1;
                    suppress = 1;
                    state->token_type = TOT_CHARACTER;
                }
                else if (ispunct(c) && c != '$' && c != '@') {
                    char tb = state->token_buffer[0];
                    if (state->token_length == 3) {
                        generate = 1;
                    }
                    else if (state->token_length == 2) {
                        if (c == '=' && (state->token_buffer[1] == '<' || state->token_buffer[1] == '>') && tb != '-') {
                            /* continue */
                        }
                        else {
                            generate = 1;
                        }
                    }
                    else if (c == '=') {
                        if (tb == '=' || tb == '<' ||
                            tb == '>' || tb == '!' ||
                            tb == '+' || tb == '-' ||
                            tb == '*' || tb == '/' ||
                            tb == '%' || tb == '&' ||
                            tb == '^' || tb == '|') {
                            /* continue */
                        }
                        else {
                            generate = 1;
                        }
                    }
                    else if ((c == '<' ||
                              c == '>' ||
                              c == '+' ||
                              c == '-' ||
                              c == '|' ||
                              c == '&') &&
                            c == tb) {
                        /* continue */
                    }
                    else if (c == '>' && tb == '-') {
                        /* continue */
                    }
                    else {
                        generate = 1;
                    }
                }
                else {
                    generate = 1;
                    state->token_type = TOT_OTHER;
                }
                break;
            case TOT_BCOMMENT:
                suppress = 1;
                if (c == '/' && co[0] == '*') {
                    state->token_type = TOT_NONE;
                }
                break;
            case TOT_LCOMMENT:
                suppress = 1;
                if (c == '\n') {
                    state->token_type = TOT_NONE;
                }
                break;
            case TOT_OTHER:
                generate = 1;
            case TOT_NONE:
            default:
                if (isspace(c) ) {
                    suppress = 1;
                }
                else if (isalpha(c) || c == '_' || (c == '$' && DOLLAR_IS_LETTER) ) {
                    state->token_type = TOT_IDENTIFIER;
                }
                else if (isdigit(c) ) {
                    state->token_type = TOT_NUMBER;
                }
                else if (c == '\'') {
                    suppress = 1;
                    state->token_type = TOT_CHARACTER;
                }
                else if (c == '"') {
                    suppress = 1;
                    state->token_type = TOT_STRING;
                }
                else if (c == '<' && state->directive_line == PREP_INCLUDE) {
                    suppress = 1;
                    state->token_type = TOT_SYSFILE;
                }
                else if (ispunct(c) && c != '$' && c != '@') {
                    state->token_type = TOT_PUNCTUATOR;
                }
                else {
                    state->token_type = TOT_OTHER;
                }
                break;
        }
        
        /* Check that we won't overrun our token buffer */
        if (state->token_length >= TOKEN_BUFFER_SIZE) {
            state->token_overflow |= 1;
        }
        if (generate || state->token_overflow & 1) {
            /* Create a token */
            if (generate_token(state, tline_number, tchar_number) ) {
                return -1;
            }

            if (generate) {
                tline_number = line_number;
                tchar_number = char_number;
            }

            generate = 0;
            state->token_length = 0;
        }

        if (!suppress && !(state->token_type & TOT_EXCEPT) ) {
            state->token_buffer[state->token_length++] = (char) c;
        }

        if (c == '\n') {
            /* Handle preprocessor directive lines */
            if (state->directive_line) {
                if (process_directive(state)) {
                    message_out(MESSAGE_ERROR, file_name, line_number, char_number, "Oops");
                }
            }
            
            char_number = 0;
            line_number++;
            generate_line(state, line_number, file_name);
        }

        /* Shift chars */
        /*co[2] = co[1];*/
        co[1] = co[0];
        co[0] = c;

        state->last_type = state->token_type;
    }

    fclose(stream);

    if (if_top(state) != PREP_IF_BASELINE) {
        message_out(MESSAGE_ERROR, file_name, line_number, 0, "Unmatched #if state.");
    }
    if_pop(state);

    return 0;
}

int8_t generate_line(preprocessor_state *state, unsigned int line, char *file) {
    logical_line *new_line;

    new_line = (logical_line *) lst_tail(state->lines);

    /* Short-circuit on empty line to save memory */
    if (new_line && !new_line->tokens->count) {
        new_line->start_line = line;
        new_line->file = file;
        return 0;
    }

    new_line = (logical_line *) malloc(sizeof(logical_line) );
    new_line->start_line = line;
    new_line->file = file;
    new_line->tokens = lst_create(&token_free, TOKEN_LIST_QUANTUM);
    lst_append(state->lines, (void *) new_line);

    return 0;
}


token *_token_alloc(int char_offset, int line_offset, char token_type, char *content, int content_length) {
    token *_token;
    int i;
    _token = (token *) malloc(sizeof(token) );
    _token->char_offset = char_offset;
    _token->line_offset = line_offset;
    _token->content = 0;
    _token->type = token_type;

    if (content != 0) {
        _token->content = (char *) malloc(content_length);
        for (i = 0; i < content_length; ++i) {
            _token->content[i] = content[i];
        }
    }

    return _token;
}

int8_t generate_token(preprocessor_state *state, unsigned int line_number, unsigned int char_number) {
    logical_line *_line;

    token *_token;
    int copy_offset;
    int token_length;
    int i;

    unsigned char directive_line;

    _line = (logical_line *) lst_tail(state->lines);
    directive_line = state->directive_line;

    if (state->token_overflow & 2) {
        /* Append to the most recent token */
        _token = (token *) lst_tail(_line->tokens);
        copy_offset = _token->length;
        token_length = copy_offset + state->token_length;
    }
    else {
        /* Create a new token */
        _token = _token_alloc(char_number, line_number, state->last_type, 0, 0);

        lst_append(_line->tokens, (void *) _token);
        if (_line->tokens->count == 1 && state->token_buffer[0] == '#') {
            directive_line = PREP_PEND;
        }

        copy_offset = 0;
        token_length = state->token_length;
    }

    _token->content = (char *) realloc(_token->content, token_length + 1);
    _token->length = token_length;
    _token->content[token_length] = 0;

    for (i = 0; i < state->token_length; i++) {
        _token->content[i + copy_offset] = state->token_buffer[i];
    }

    if (state->token_overflow & 1) {
        state->token_overflow <<= 1;
    }
    else if (state->directive_line == PREP_PEND) {
        uint8_t *new_directive;

        new_directive = hmp_get(state->directives, (void *) _token->content);
        if (new_directive) {
            directive_line = *new_directive;
        }
        else {
            message_out(MESSAGE_ERROR, _line->file, line_number, char_number, "Unrecognized preprocessor directive.");
            return -1;
        }
    }
    else if (!state->directive_line && _token->type == TOT_IDENTIFIER) {
        macro_replacement(state, _token, -1, 0);
    }

    state->directive_line = directive_line;

    return 0;
}

void if_free(void *data) {
    free(data);
}
void if_push(preprocessor_state *state, uint8_t if_state) {
    uint8_t *t;
    t = (uint8_t *) malloc(sizeof(uint8_t));
    *t = if_state;

    lst_append(state->if_stack, t);
}
uint8_t if_pop(preprocessor_state *state) {
    uint8_t t;
    t = if_top(state);
    lst_trim(state->if_stack, state->if_stack->count - 1);

    return t;
}
uint8_t if_top(preprocessor_state *state) {
    uint8_t *t;
    t = (uint8_t *) lst_tail(state->if_stack);
    return *t;
}


int8_t process_directive(preprocessor_state *state) {
    logical_line *_line ;
    token        *_token;
    macro        *_macro;

    uint32_t token_count;
    uint8_t  die;

    int8_t if_result;

    char *_string;

    char *error_text;
    char *warning_text;

    _string = 0;
    die = 0;

    _line = (logical_line *) lst_tail(state->lines);

    token_count = _line->tokens->count;

    if (token_count > 1) {
        switch (state->directive_line) {
            case PREP_INCLUDE:
                if (token_count < 3) {
                    message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, "No file name in #include directive.");
                    return -1;
                }
                else {
                    _token = (token *) lst_index(_line->tokens, 2);

                    if (token_count > 3) {
                        message_out(MESSAGE_WARN, _line->file, _line->start_line, 0, "Additional tokens at end of #include directive (ignored).");
                    }

                    if (_token->type == TOT_SYSFILE) {
                        message_out(MESSAGE_WARN, _line->file, _line->start_line, 0, "System include file ignored.");
                        break;
                    }

                    _string = null_terminate(_token->content, _token->length);
                }
                break;
            case PREP_DEFINE:
                if (token_count < 3) {
                    message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, "No macro name in #define directive.");
                    return -1;
                }
                _macro = (macro *) malloc(sizeof(macro) );
                _macro->type = 0;
                _macro->args = 0;
                _macro->tokens = lst_create(&token_free, token_count - 3);

                _token = (token *) lst_index(_line->tokens, 2);
                _string = _token->content;

                if (_line->tokens->count > 3) {
                    list_iterator *iterator = lst_iterator(_line->tokens);
                    iterator->_index = 3;
                    while ( (_token = (token *) lst_next(iterator) ) ) {
                        lst_append(_macro->tokens, (void *) _token);
                    }
                }

                hmp_put(state->macros, (void *) _string, (void *) _macro);
                break;
            case PREP_IFDEF:
                /* Handled mostly the same way as #ifndef. Fall through. */
            case PREP_IFNDEF:
                /* Prepare warning and error strings */
                if (state->directive_line == PREP_IFDEF) {
                    error_text = "No macro in #ifdef directive.";
                    warning_text = "Additional tokens at end of #ifdef directive (ignored).";
                }
                else {
                    error_text = "No macro in #ifndef directive.";
                    warning_text = "Additional tokens at end of #ifndef directive (ignored).";
                }

                /* If we're missing required tokens, error out. */
                if (token_count < 3) {
                    message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, error_text);
                    return -1;
                }
                /* Warn about additional tokens */
                if (token_count > 3) {
                    message_out(MESSAGE_WARN, _line->file, _line->start_line, 0, warning_text);
                }

                /* Replace this token stream with a #if-friendly token stream using defined(), and update token count. */
                token_count = _retoken_ifdef(state, _line);

                /* Fall through to #if handler */
            case PREP_ELIF:
                /* Handled mostly the same way as #if. Fall through. */
            case PREP_IF:
                message_out(MESSAGE_WARN, _line->file, _line->start_line, 0, "#if implimentation incomplete.");

                /* Error out on missing required tokens. */
                if (token_count < 3) {
                    message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, "Not enough tokens in #if directive.");
                    return -1;
                }

                /* Push a new #if state if we need one. */
                if (state->directive_line == PREP_IF) {
                    if_push(state, PREP_IF_MATCHING);
                }

                /* Check for unmatched directives. In this case, #elif is the only one we're worried about. */
                if (if_top(state) == PREP_IF_BASELINE) {
                    message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, "Unmatched #elif.");
                    return -1;
                }

                {
                    uint8_t if_state;
                    if_state = if_pop(state);

                    if_result = handle_if(state, _line);
                    if (if_result < 0) {
                        message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, "#if evaluation failed.");
                        return -1;
                    }

                    if (if_result > 0 && if_state == PREP_IF_MATCHING) {
                        /* #if evaluated to true (1L), and we haven't matched yet.
                         * Signal a text inclusion. */
                        if_push(state, PREP_IF_INCLUDING);
                    }
                    else if (if_result > 0 && if_state == PREP_IF_INCLUDING) {
                        /* #if evaluated to true (1L), and we have matched.
                         * Signal that we're ready for #endif. */
                        if_push(state, PREP_IF_ENDING);
                    }
                    else {
                        /* #if evaluated to false (0L).
                         * Signal that we're still looking */
                        if_push(state, PREP_IF_MATCHING);
                    }
                }

                /* This line is for evaluating defined()... */
                /*result = hmp_get(state->macros, lst_index(_line->tokens, 2) );*/
                break;
            case PREP_ELSE:
                /* Warn about additional tokens. */
                if (token_count > 2) {
                    message_out(MESSAGE_WARN, _line->file, _line->start_line, 0, "Additional tokens at end of #else directive (ignored).");
                }
                /* Pop #if state. */
                if_result = if_pop(state);
                /* Check for matched directives. */
                if (if_result == PREP_IF_BASELINE) {
                    message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, "Unmatched #else.");
                    return -1;
                }

                /* If we've matched a block in this group already, continue / switch to scanning. */
                if (if_result == PREP_IF_INCLUDING || if_result == PREP_IF_ENDING) {
                    if_push(state, PREP_IF_ENDING);
                }
                /* If we haven't, switch to inclusion. */
                else {
                    if_push(state, PREP_IF_INCLUDING);
                }
                
                break;
            case PREP_ENDIF:
                /* Warn about additional tokens. */
                if (token_count > 2) {
                    message_out(MESSAGE_WARN, _line->file, _line->start_line, 0, "Additional tokens at end of #endif directive (ignored).");
                }

                /* Pop #if state. */
                if_result = if_pop(state);
                /* Check for matched directives. */
                if (if_result == PREP_IF_BASELINE) {
                    message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, "Unmatched #endif.");
                    return -1;
                }

                break;
            case PREP_ERROR:
                message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, "#error");
                return -1;
            case PREP_LINE:
                break;
            default:
                break;
        }
    }

    _line->tokens->count = 0;

    if (state->directive_line == PREP_INCLUDE && _string) {
        state->directive_line = PREP_NONE;
        if (preprocess(state, _string) ) {
            return -1;
        }
        free(_string);
    }

    state->directive_line = PREP_NONE;
    state->token_type     = TOT_NONE;

    return 0;
}

int8_t handle_if(preprocessor_state *state, logical_line *_line) {
    token         *_token;
    list_iterator *tokens;
    int32_t        token_count;
    int16_t        evaluation_depth;

    tokens = lst_iterator(_line->tokens);

    /* Eat preprocessor tokens */
    lst_next(tokens);
    lst_next(tokens);

    /* Keep track of how many tokens we have right now */
    token_count = _line->tokens->count;

    while (!lst_is_end(tokens)) {
        _token = (token *) lst_next(tokens);
        switch (_token->type) {
          case TOT_IDENTIFIER:
            if (!strcmp(_token->content, "defined")) {
                /* check for optional parenthesis surrounding a required identifier */
                token *next;
                uint8_t guarded;

                guarded = 0;
                next = (token *) lst_peek(tokens, 0);
                if (next && next->type == TOT_PUNCTUATOR && strcmp(next->content, "(") == 0) {
                    next = (token *) lst_peek(tokens, 2);
                    if (next && next->type == TOT_PUNCTUATOR && strcmp(next->content, ")") == 0) {
                        next = (token *) lst_peek(tokens, 1);
                        guarded = 1;
                    }
                    else {
                        return -1;
                    }
                }

                if (next && next->type == TOT_IDENTIFIER) {
                    list  *_replacements;
                    token *_replacement_token;
                    macro *_macro;

                    _replacements = lst_create(&token_free, TOKEN_LIST_QUANTUM);
                    _macro = (macro *) hmp_get(state->macros, (void *) next->content);
                    if (_macro) {
                        /* 1L */
                        _replacement_token = _token_alloc(0, 0, TOT_NUMBER, "1L", 2);
                    }
                    else {
                        /* 0L */
                        _replacement_token = _token_alloc(0, 0, TOT_NUMBER, "0L", 2);
                    }

                    lst_append(_replacements, _replacement_token);
                    lst_splice(_line->tokens, _replacements, tokens->_index - 1, guarded ? 4 : 2);
                }
                else {
                    return -1;
                }
            }
            else {
                macro_replacement(state, _token, tokens->_index - 1, 1);
            }
            lst_prev(tokens);
            break;
          case TOT_PUNCTUATOR:
            /* This should filter out any inappropriate punctuators, I guess. */
            
            break;
          case TOT_STRING:
            /* Throw an error. We don't want no strings around these parts. */
            return -1;
            break;
          case TOT_CHARACTER:
            /* This should just be treated as an int, so... replace it with a string representation of its int value? I'm not sure. */
            break;
          case TOT_NUMBER:
            if (_token->content[0] == '0') {
                if (!_token->content[1]) {
                    /* Value is 0 */
                }
                /* Expecting hex or octal constant */
                else if (_token->content[1] && (_token->content[1] == 'x' || _token->content[1] == 'X')) {
                    /* Expecting hex constant. */
                }
                else if (_token->content[1] && (_token->content[1] >= '0' && _token->content[1] <= '9')) {
                    /* Expecting octal constant. */
                }
            }
            else if (_token->content[0] >= '1' && _token->content[0] <= '9') {
                /* Expecting decimal constant. */
            }
            message_out(MESSAGE_WARN, "Got a number!", 0, 0, _token->content);
            break;
          default:
            break;
        }
    }

    return 0;
}

int8_t _retoken_ifdef(preprocessor_state *state, logical_line *_line) {
    int8_t token_count;
    list *tokens;
    token *_token;

    token_count = 2;
    tokens = _line->tokens;

    /* Trim off spare (ignored) tokens */
    if (_line->tokens->count > 3) {
        lst_trim(tokens, 3);
    }

    if (state->directive_line == PREP_IFNDEF) {
        /* Insert `!` token */
        _token = _token_alloc(0, 0, TOT_PUNCTUATOR, "!", 2);
        lst_insert(tokens, _token, token_count++);
    }
    /* Insert `defined` token */
    _token = _token_alloc(0, 0, TOT_IDENTIFIER, "defined", 8);
    lst_insert(tokens, _token, token_count++);
    /*_token = _token_alloc(0, 0, TOT_PUNCTUATOR, "(", 2);
    lst_insert(tokens, _token, token_count++); */
    /* Keep macro name */
    token_count++;
    /* Append `)` token */
    /*_token = _token_alloc(0, 0, TOT_PUNCTUATOR, ")", 2);
    lst_insert(tokens, _token, token_count++);*/

    state->directive_line = PREP_IF;

    return token_count;
}


int8_t _macro_replacement(hash_map *macros, list *tokens, token *_in_token, uint8_t self_reference, uint8_t zero_unmatched) {
    macro *_macro;

    if (_in_token->type == TOT_IDENTIFIER) {
        _macro = (macro *) hmp_get(macros, (void *) _in_token->content);
    }
    else {
        _macro = 0;
    }

    if (_macro && !self_reference) {
        token *_token;
        list_iterator *iterator = lst_iterator(_macro->tokens);

        while ((_token = (token *) lst_next(iterator))) {
            _macro_replacement(macros, tokens, _token, strcmp(_token->content, _in_token->content) ? 0 : 1, zero_unmatched);
        }
    }
    else {
        token *_put_token;
        _put_token = _in_token;
        if (zero_unmatched && _in_token->type == TOT_IDENTIFIER) {
            _put_token = _token_alloc(0, 0, TOT_NUMBER, "0L", 2);
        }
        
        lst_append(tokens, (void *) _put_token);
    }

    return 0;
}

int8_t macro_replacement(preprocessor_state *state, token *_in_token, int32_t offset, uint8_t zero_unmatched) {
    list          *tokens;
    list_iterator *iterator;
    logical_line  *line;

    tokens   = lst_create(&token_free, TOKEN_LIST_QUANTUM);
    iterator = lst_iterator(tokens);

    _macro_replacement(state->macros, tokens, _in_token, 0, zero_unmatched);
    line = (logical_line *) lst_tail(state->lines);

    if (offset < 0) {
        offset = line->tokens->count - 1;
    }

    lst_splice(line->tokens, tokens, offset, 1);

    return 0;
}


void line_free(void *_line) {
    lst_destroy( ( (logical_line *) _line)->tokens);
    free(_line);
}
void token_free(void *_token) {
    free( ( (token *) _token)->content);
    free(_token);
}

void string_free(void *_string) {
    free(_string);
}
int8_t string_compare(void *l, void *r) {
    return strcmp( (char *) l, (char *) r);
}
void macro_free(void *_macro) {
}
uint32_t macro_hash(void *_key, uint32_t max) {
    char *key;
    uint32_t hash;
    uint8_t i;

    key = (char *) _key;
    hash = 0;

    for (i = 0; i < 8 && key[i]; i++) {
        hash += key[i] - 36;
    }

    hash %= 256; /* Ignore the requested hash size */

    return hash;
}
int8_t macro_compare(void *left, void *right) {
    char *l, *r;
    return 0;
}

void directive_free(void *_) {
    /* Do nothing... */
}
uint32_t directive_hash(void *_key, uint32_t size) {
    char *key;
    uint8_t _ret;
    _ret = 0;
    key = (char *) _key;

    if (key[0] == 0 || key[1] == 0) {
    }
    else if (key[0] == 'd') {
        _ret = 1;
    }
    else if (key[0] == 'p') {
        _ret = 2;
    }
    else if (key[0] == 'u') {
        _ret = 3;
    }
    else if (key[0] == 'e') {
        if (key[2] == 's') {
            _ret = 4;
        }
        else if (key[2] == 'd') {
            _ret = 5;
        }
        else if (key[2] == 'i') {
            _ret = 6;
        }
    }
    else if (key[0] == 'i') {
        if (key[2] == 'c') {
            _ret = 7;
        }
        else if (key[2] == 'd') {
            _ret = 8;
        }
        else if (key[2] == 'n') {
            _ret = 9;
        }
    }

    return _ret;
}


char *null_terminate(char *c, unsigned int len) {
    char *str;
    unsigned int i;

    str = 0;

    if (c && len) {
        str = (char *) malloc(len + 1);

        for (i = 0; i < len; i++) {
            str[i] = c[i];
        }
        str[i] = '\0';
    }

    return str;
}

