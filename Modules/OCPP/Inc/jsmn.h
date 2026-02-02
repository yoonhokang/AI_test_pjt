/*
 * MIT License
 *
 * Copyright (c) 2010 Serge A. Zaitsev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#ifndef JSMN_H
#define JSMN_H

#include <stddef.h>

#define JSMN_PARENT_LINKS // Enable parent links to avoid build error

#ifndef JSMN_STATIC
#define JSMN_STATIC static
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * JSON type identifier. Basic types are:
 * 	o Object
 * 	o Array
 * 	o String
 * 	o Other primitive: number, boolean (true/false) or null
 */
typedef enum {
	JSMN_UNDEFINED = 0,
	JSMN_OBJECT = 1,
	JSMN_ARRAY = 2,
	JSMN_STRING = 3,
	JSMN_PRIMITIVE = 4
} jsmntype_t;

enum jsmnerr {
	/* Not enough tokens were provided */
	JSMN_ERROR_NOMEM = -1,
	/* Invalid character inside JSON string */
	JSMN_ERROR_INVAL = -2,
	/* The string is not a full JSON packet, more bytes expected */
	JSMN_ERROR_PART = -3
};

/**
 * JSON token description.
 * type		type (object, array, string etc.)
 * start	start position in JSON data string
 * end		end position in JSON data string
 */
typedef struct {
	jsmntype_t type;
	int start;
	int end;
	int size;
#ifdef JSMN_PARENT_LINKS
	int parent;
#endif
} jsmntok_t;

/**
 * JSON parser. Contains an array of token blocks available. Also store
 * the string being parsed now and current position in that string.
 */
typedef struct {
	unsigned int pos; /* offset in the JSON string */
	unsigned int toknext; /* next token to allocate */
	int toksuper; /* superior token node, e.g. parent object or array */
} jsmn_parser;

/**
 * Create JSON parser over an array of tokens
 */
JSMN_STATIC void jsmn_init(jsmn_parser *parser);

/**
 * Run JSON parser. It parses a JSON data string into and array of tokens, each
 * describing
 * a single JSON object.
 */
JSMN_STATIC int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
		jsmntok_t *tokens, unsigned int num_tokens);

#ifdef __cplusplus
}
#endif

#endif /* JSMN_H */

/* Implementation */
#ifdef JSMN_IMPLEMENTATION
#ifdef __cplusplus
extern "C" {
#endif

JSMN_STATIC void jsmn_init(jsmn_parser *parser) {
	parser->pos = 0;
	parser->toknext = 0;
	parser->toksuper = -1;
}

JSMN_STATIC int jsmn_parse(jsmn_parser *parser, const char *js, size_t len,
		jsmntok_t *tokens, unsigned int num_tokens) {
	int r;
	int i;
	jsmntok_t *token;
	int count = parser->toknext;

	for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
		char c;
		jsmntype_t type;

		c = js[parser->pos];
		switch (c) {
		case '{': case '[':
			count++;
			if (tokens == NULL) {
				break;
			}
			token = &tokens[parser->toknext];
			if (parser->toknext < num_tokens) {
				token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
				token->start = parser->pos;
				token->parent = parser->toksuper;
			}
			parser->toksuper = parser->toknext;
			parser->toknext++;
			break;
		case '}': case ']':
			if (tokens == NULL) {
				break;
			}
			type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
#ifdef JSMN_PARENT_LINKS
			if (parser->toknext < 1) {
				return JSMN_ERROR_INVAL;
			}
			token = &tokens[parser->toknext - 1];
			for (;;) {
				if (token->start != -1 && token->end == -1) {
					if (token->type != type) {
						return JSMN_ERROR_INVAL;
					}
					token->end = parser->pos + 1;
					parser->toksuper = token->parent;
					break;
				}
				if (token->parent == -1) {
					if(token->type != type || parser->toksuper == -1){
						return JSMN_ERROR_INVAL;
					}
					break;
				}
				token = &tokens[token->parent];
			}
#else
			for (i = parser->toknext - 1; i >= 0; i--) {
				token = &tokens[i];
				if (token->start != -1 && token->end == -1) {
					if (token->type != type) {
						return JSMN_ERROR_INVAL;
					}
					parser->toksuper = -1;
					token->end = parser->pos + 1;
					break;
				}
			}
			/* Error if unmatched closing bracket */
			if (i == -1) return JSMN_ERROR_INVAL;
			for (; i >= 0; i--) {
				token = &tokens[i];
				if (token->start != -1 && token->end == -1) {
					parser->toksuper = i;
					break;
				}
			}
#endif
			break;
		case '\"':
			r = parser->pos; 
            // Correctly parse strings, handling escapes
            parser->pos++;
            for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
                char c = js[parser->pos];
                if (c == '\"') {
                     // End of string
                     break;
                }
                if (c == '\\' && parser->pos + 1 < len) {
                    parser->pos++; // Skip escaped char
                }
            }
			
			count++;
			if (tokens == NULL) {
				break;
			}
			token = &tokens[parser->toknext];
			if (parser->toknext < num_tokens) {
				token->type = JSMN_STRING;
				token->start = r + 1;
				token->end = parser->pos;
				token->size = 0;
#ifdef JSMN_PARENT_LINKS
				token->parent = parser->toksuper;
#endif
			}
			parser->toknext++;
            
			// Add size to parent
			if (parser->toksuper != -1 && tokens != NULL) {
				tokens[parser->toksuper].size++;
			}
            
			break;
		case '\t': case '\r': case '\n': case ' ':
			break;
		case ':':
			parser->toksuper = parser->toknext - 1;
			break;
		case ',':
			if (tokens != NULL && parser->toksuper != -1 &&
					tokens[parser->toksuper].type != JSMN_ARRAY &&
					tokens[parser->toksuper].type != JSMN_OBJECT) {
#ifdef JSMN_PARENT_LINKS
				parser->toksuper = tokens[parser->toksuper].parent;
#else
				for (i = parser->toknext - 1; i >= 0; i--) {
					if (tokens[i].type == JSMN_ARRAY || tokens[i].type == JSMN_OBJECT) {
						if (tokens[i].start != -1 && tokens[i].end == -1) {
							parser->toksuper = i;
							break;
						}
					}
				}
#endif
			}
			break;
#ifdef JSMN_STRICT
		/* In strict mode primitives are: numbers and booleans */
		case '-': case '0': case '1' : case '2': case '3' : case '4':
		case '5': case '6': case '7' : case '8': case '9':
		case 't': case 'f': case 'n' :
			/* And they must not be keys of the object */
			if (tokens != NULL && parser->toksuper != -1) {
				jsmntok_t *t = &tokens[parser->toksuper];
				if (t->type == JSMN_OBJECT ||
						(t->type == JSMN_STRING && t->size != 0)) {
					return JSMN_ERROR_INVAL;
				}
			}
#else
		/* In non-strict mode every unquoted value is a primitive */
		default:
#endif
			r = parser->pos; 
            // Found primitive
            for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++) {
                char c = js[parser->pos];
                if (c == '\t' || c == '\r' || c == '\n' || c == ' ' ||
                    c == ',' || c == ']' || c == '}') {
                    // End of primitive
                    parser->pos--;
                    break;
                }
            }
#ifdef JSMN_STRICT
			/* Found */
			if (tokens == NULL) {
                count++;
                break;
            }
			token = &tokens[parser->toknext];
			if (parser->toknext < num_tokens) {
				token->type = JSMN_PRIMITIVE;
				token->start = r;
				token->end = parser->pos + 1;
				token->size = 0;
#ifdef JSMN_PARENT_LINKS
				token->parent = parser->toksuper;
#endif
			}
			parser->toknext++;
            
            // Add size to parent
            if (parser->toksuper != -1 && tokens != NULL) {
				tokens[parser->toksuper].size++;
			}
			break;
#else
            // In loose mode, same logic for primitive
            count++;
			if (tokens == NULL) {
				break;
			}
			token = &tokens[parser->toknext];
			if (parser->toknext < num_tokens) {
				token->type = JSMN_PRIMITIVE;
				token->start = r;
				token->end = parser->pos + 1;
				token->size = 0;
#ifdef JSMN_PARENT_LINKS
				token->parent = parser->toksuper;
#endif
			}
			parser->toknext++;
			if (parser->toksuper != -1 && tokens != NULL) {
				tokens[parser->toksuper].size++;
			}
			break;
#endif

		}
	}

	if (tokens != NULL) {
		for (i = parser->toknext - 1; i >= 0; i--) {
			/* Unmatched opened object or array */
			if (tokens[i].start != -1 && tokens[i].end == -1) {
				return JSMN_ERROR_PART;
			}
		}
	}

	return count;
}
#ifdef __cplusplus
}
#endif

#endif /* JSMN_IMPLEMENTATION */
