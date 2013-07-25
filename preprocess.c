#include "preprocess.h"
#include "container.h"
#include "message.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
	{ "pragma" , PREP_PRAGMA  }
};

void line_free (void *);
void token_free(void *);

void   string_free   (void *);
int8_t string_compare(void *, void *);
void     macro_free(void *);
uint32_t macro_hash(void *, uint32_t);
void     directive_free(void *);
uint32_t directive_hash(void *, uint32_t);

int8_t macro_replacement(preprocessor_state *, token *, int32_t);

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

	state->directive_line = 0;

	state->lines      = lst_create(&line_free, TOKEN_LIST_QUANTUM);
	state->macros     = hmp_create(&string_free, &macro_free,     &string_compare, &macro_hash,     256);

	// Initialize the preprocessor directives
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
	
		// Proceed with tokenization
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
					// Proceed	
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
				else if (isspace(c) ) {
					generate = 1;
					suppress = 1;
					state->token_type = TOT_NONE;
				}
				else if (ispunct(c) && c != '$' && c != '@') {
					generate = 1;
					state->token_type = TOT_PUNCTUATOR;
				}
				else {
					generate = 1;
					state->token_type = TOT_OTHER;
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
							// continue
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
							// continue
						}
						else {
							generate = 1;
						}
					}
					else if ( (c == '<' ||
					           c == '>' ||
						   c == '+' ||
						   c == '-' ||
						   c == '|' ||
						   c == '&') &&
						  c == tb) {
						// continue
					}
					else if (c == '>' && tb == '-') {
						// continue
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
		
		// Check that we won't overrun our token buffer
		if (state->token_length >= TOKEN_BUFFER_SIZE) {
			state->token_overflow |= 1;
		}
		if (generate || state->token_overflow & 1) {
			// Create a token
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
			// Handle preprocessor directive lines
			
			if (state->directive_line) {
				logical_line *_line ;
				token        *_token;
				macro        *_macro;

				uint32_t token_count;
				uint8_t  die;

				char *_string;

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
						case PREP_IFNDEF:
							char *error_text, *warning_text;
							void *result;

							if (state->directive_line == PREP_IFDEF) {
								error_text = "No macro in #ifdef directive.";
								warning_text = "Additional tokens at end of #ifdef directive (ignored).";
							}
							else {
								error_text = "No macro in #ifndef directive.";
								warning_text = "Additional tokens at end of #ifndef directive (ignored).";
							}

							if (token_count < 3) {
								message_out(MESSAGE_ERROR, _line->file, _line->start_line, 0, error_text);
								return -1;
							}
							if (token_count > 3) {
								message_out(MESSAGE_WARN, _line->file, _line->start_line, 0, warning_text);
							}

							result = hmp_get(state->macros, lst_index(_line->tokens, 2) );

							if (result && state->directive_line == PREP_IFDEF) {
								
							}
							else if (!result && state->directive_line == PREP_IFNDEF) {
								
							}
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
			}
			
			char_number = 0;
			line_number++;
			generate_line(state, line_number, file_name);
		}

		// Shift chars
		//co[2] = co[1];
		co[1] = co[0];
		co[0] = c;

		state->last_type = state->token_type;
	}

	fclose(stream);

	return 0;
}

int8_t generate_line(preprocessor_state *state, unsigned int line, char *file) {
	logical_line *new_line;

	new_line = (logical_line *) lst_tail(state->lines);

	// Short-circuit on empty line to save memory
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
		// Append to the most recent token
		_token = (token *) lst_tail(_line->tokens);
		copy_offset = _token->length;
		token_length = copy_offset + state->token_length;
	}
	else {
		// Create a new token
		_token = (token *) malloc(sizeof(token) );
		_token->char_offset = char_number;
		_token->line_offset = line_number;
		_token->content = 0;
		_token->type = state->last_type;

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
		macro_replacement(state, _token, -1);
	}

	state->directive_line = directive_line;

	return 0;
}

int8_t _macro_replacement(hash_map *macros, list *tokens, token *_in_token, uint8_t self_reference) {
	macro *_macro;
	_macro = (macro *) hmp_get(macros, (void *) _in_token->content);

	if (_macro && !self_reference) {
		token *_token;
		list_iterator *iterator = lst_iterator(_macro->tokens);

		while ( (_token = (token *) lst_next(iterator) ) ) {
			_macro_replacement(macros, tokens, _token, strcmp(_token->content, _in_token->content) ? 0 : 1 );
		}
	}
	else {
		lst_append(tokens, (void *) _in_token);
	}

	return 0;
}

int8_t macro_replacement(preprocessor_state *state, token *_in_token, int32_t offset) {
	list          *tokens;
	list_iterator *iterator;
	logical_line  *line;

	tokens   = lst_create(&token_free, TOKEN_LIST_QUANTUM);
	iterator = lst_iterator(tokens);

	_macro_replacement(state->macros, tokens, _in_token, 0);
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

	hash %= 256; // Ignore the requested hash size

	return hash;
}
int8_t macro_compare(void *left, void *right) {
	char *l, *r;
	return 0;
}

void directive_free(void *_) {
	// Do nothing...
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

