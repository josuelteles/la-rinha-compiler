/**
 * @file rinha.h
 *
 * @brief Rinha Language Interpreter - Compiler Development Challenge
 *
 * For more information, please visit the documentation at:
 * [rinha-de-compiler](https://github.com/aripiprazole/rinha-de-compiler)
 *
 * @author Josuel Teles
 * @date September 14, 2023
 */

#ifndef _LA_RINHA_H
#define _LA_RINHA_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

#define _RINHA_CALL_ //__attribute__((always_inline))

typedef enum {
    TOKEN_UNDEFINED,
    TOKEN_LET,
    TOKEN_FN,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_PRINT,
    TOKEN_TRUE,
    TOKEN_QUOTE,
    TOKEN_APOSTROPHE,
    TOKEN_STRING,
    TOKEN_FALSE,
    TOKEN_FIRST,
    TOKEN_SECOND,
    TOKEN_MOD,
    TOKEN_LT,
    TOKEN_NUMBER,
    TOKEN_IDENTIFIER,
    TOKEN_COMMA,
    TOKEN_WILDCARD,
    TOKEN_SEMICOLON,
    TOKEN_ASSIGN,
    TOKEN_PLUS,
    TOKEN_MINUS,
    TOKEN_MULTIPLY,
    TOKEN_DIVIDE,
    TOKEN_ARROW,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_GT,
    TOKEN_GTE,
    TOKEN_LTE,
    TOKEN_EOF
} token_type;


typedef enum {
    UNDEFINED,
    STRING,
    INTEGER,
    BOOLEAN,
    FLOAT,
    FUNCTION,
    TUPLE
} value_type;

#define RINHA_PRIMITIVES \
    value_type type; \
    int hash; \
    union { \
        int64_t number; \
        bool boolean; \
        char string[RINHA_CONFIG_STRING_VALUE_MAX]; \
   };

struct __primitive {
 RINHA_PRIMITIVES;
};


typedef struct _tuple {
   struct __primitive first;
   struct __primitive second;
} tuple_t;


typedef struct _value {
    RINHA_PRIMITIVES;
    tuple_t tuple;
} rinha_value_t;



typedef struct {
    char name[32];
    rinha_value_t value;
} variable_t;


typedef struct {
    token_type type;
    int hash;
    int line;
    int pos;
    int jmp_pc1;
    int jmp_pc2;
    char lexeme[RINHA_CONFIG_STRING_VALUE_MAX];
    rinha_value_t value;
} token_t;

typedef struct {
    //variable_t mem[RINHA_CONFIG_SYMBOLS_SIZE];
    variable_t mem[64];
    int count;
} stack_t;

typedef struct {
    char name[16][16];
    //int hash[RINHA_CONFIG_SYMBOLS_SIZE];
    int hash[64];
    int count;
} args_t;

typedef struct {
    rinha_value_t value;
    int pc;
    bool cached;
} cache_t;

typedef struct {
    char name[16];
    rinha_value_t ret;
    args_t args;
    stack_t *stack;
    //cache_t cache[RINHA_CONFIG_SYMBOLS_SIZE];
    cache_t cache[RINHA_CONFIG_CACHE_SIZE];
    int pc;
} function_t;

void rinha_token_advance();

void rinha_token_consume_(token_type expected_type);

void rinha_parse_program_(rinha_value_t *result);

void rinha_parse_statement_(rinha_value_t *result);

void rinha_parser_expression_(rinha_value_t *result);

void rinha_parse_block_(rinha_value_t *result);

void rinha_parse_if_statement_(rinha_value_t *result);

void rinha_parser_identifier(void);

bool rinha_parser_logical_or_(rinha_value_t *result);

static void rinha_parse_term_(rinha_value_t *result);

void rinha_parse_calc_(rinha_value_t *result);

void rinha_function_exec_(function_t *f, rinha_value_t *result);

int is_valid_identifier(const char *token);

token_t* rinha_next_token(void);

function_t* rinha_function_get_(int hash);

void rinha_print_(rinha_value_t *value, bool lf, bool debug);

void rinha_print_debug_(rinha_value_t *value);

void rinha_yaswoc( rinha_value_t *value );

void rinha_tokenize_(char **code_ptr, token_t *tokens, int *rinha_tok_count);

bool rinha_script_exec(char *name, char *script, rinha_value_t *response,
                             bool test);


_RINHA_CALL_ static rinha_value_t rinha_value_set_(rinha_value_t value);

#define BOOL_NAME(b) ((b) ? "true" : "false")

#define COLOR_RED     "\x1b[1;91m"
#define COLOR_WHITE   "\x1b[1m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_RESET   "\x1b[0m"

#define TEXT_RED( text )   COLOR_RED   text COLOR_RESET
#define TEXT_WHITE( text ) COLOR_WHITE text COLOR_RESET
#define TEXT_GREEN( text ) COLOR_GREEN text COLOR_RESET

#define DEBUG_ENABLED 1

#if DEBUG_ENABLED == 1

#define DEBUG { \
        printf("\n------------------- TOKEN-DEBUG --------------------\n"); \
        printf("function_t: [\033[31m%s\033[0m] Line: [\033[31m%d\033[0m] \n\n", __func__,  __LINE__ ); \
        printf("Symbol:[\033[32m%s\033[0m], Type: [%d], Hash: [%d], Value: [%d]", \
            rinha_current_token_ctx.lexeme, \
            rinha_current_token_ctx.type, \
            rinha_current_token_ctx.hash, \
            rinha_current_token_ctx.value.number); \
        rinha_print_debug_(&rinha_current_token_ctx.value); \
        printf("\n----------------------------------------------------\n");\
    }

// Define a macro BREAK para pausar a execução para depuração
#define BREAK getchar();

#else

#define DEBUG
#define BREAK

#endif

#endif
