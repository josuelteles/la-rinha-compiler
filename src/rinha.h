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
    TOKEN_YASWOC,
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

/**
 * @brief Represents a value in the Rinha programming language.
 *
 * This structure represents a value in the Rinha programming language, which can be of various types
 * including numbers, booleans, and strings.
 *
 * @var type The type of the value.
 * @var hash A hash value associated with the value.
 * @var number The numeric value if the type is value_type::NUMBER.
 * @var boolean The boolean value if the type is value_type::BOOLEAN.
 * @var string The string value if the type is value_type::STRING.
 */
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

/**
 * @brief Represents a tuple containing two rinha_value_t values.
 *
 * @var first The first value in the tuple.
 * @var second The second value in the tuple.
 */
typedef struct _value {
    RINHA_PRIMITIVES;
    tuple_t tuple;
} rinha_value_t;



/**
 * @brief Represents a variable with a name and a value.
 *
 * @var name The name of the variable.
 * @var value The value associated with the variable.
 */
typedef struct {
    char name[32];
    rinha_value_t value;
} variable_t;

/**
 * @brief Represents a token in the Rinha programming language.
 *
 * @var type The type of the token.
 * @var hash A hash value associated with the token.
 * @var line The line number where the token appears in the source code.
 * @var pos The position of the token in the line.
 * @var jmp_pc1 Jump target PC1 if applicable.
 * @var jmp_pc2 Jump target PC2 if applicable.
 * @var lexeme The lexeme (text) of the token.
 * @var value The value associated with the token if applicable.
 */
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

/**
 * @brief Represents a stack of variables.
 *
 * @var mem An array of variables.
 * @var count The number of variables in the stack.
 */
typedef struct {
    //variable_t mem[RINHA_CONFIG_SYMBOLS_SIZE];
    variable_t mem[64];
    int count;
} stack_t;

/**
 * @brief Represents arguments in a function call.
 *
 * @var name Names of the arguments.
 * @var hash Hash values associated with the arguments.
 * @var count The number of arguments.
 */
typedef struct {
    char name[16][16];
    //int hash[RINHA_CONFIG_SYMBOLS_SIZE];
    int hash[64];
    int count;
} args_t;

/**
 * @brief Represents a cached value in a function.
 *
 * @var value The cached value.
 * @var pc Program counter associated with the cached value.
 * @var cached Flag indicating whether the value is cached.
 */
typedef struct {
    rinha_value_t value;
    int pc;
    bool cached;
} cache_t;

/**
 * @brief Represents a function in the Rinha programming language.
 *
 * @var name The name of the function.
 * @var ret The return value of the function.
 * @var args Arguments of the function.
 * @var stack The stack associated with the function.
 * @var cache Cached values within the function.
 * @var pc Program counter associated with the function.
 */
typedef struct {
    char name[16];
    rinha_value_t ret;
    args_t args;
    stack_t *stack;
    //cache_t cache[RINHA_CONFIG_SYMBOLS_SIZE];
    cache_t cache[RINHA_CONFIG_CACHE_SIZE];
    int pc;
} function_t;

/**
 * @brief Advance the current token.
 *
 * This function advances the current token to the next token in the input stream.
 */
void rinha_token_advance();

/**
 * @brief Consume a token of the specified type.
 *
 * This function consumes a token of the specified type from the input stream.
 *
 * @param expected_type The expected type of the token to be consumed.
 */
void rinha_token_consume_(token_type expected_type);

/**
 * @brief Parse a program.
 *
 * This function parses a program and populates the provided rinha_value_t structure.
 *
 * @param[out] result The rinha_value_t structure to store the parsed program.
 */
void rinha_parse_program_(rinha_value_t *result);

/**
 * @brief Parse a statement.
 *
 * This function parses a statement and populates the provided rinha_value_t structure.
 *
 * @param[out] result The rinha_value_t structure to store the parsed statement.
 */
void rinha_parse_statement_(rinha_value_t *result);

/**
 * @brief Parse an expression.
 *
 * This function parses an expression and populates the provided rinha_value_t structure.
 *
 * @param[out] result The rinha_value_t structure to store the parsed expression.
 */
void rinha_parser_expression_(rinha_value_t *result);

/**
 * @brief Parse a block of code.
 *
 * This function parses a block of code and populates the provided rinha_value_t structure.
 *
 * @param[out] result The rinha_value_t structure to store the parsed block.
 */
void rinha_parse_block_(rinha_value_t *result);

/**
 * @brief Parse an if statement.
 *
 * This function parses an if statement and populates the provided rinha_value_t structure.
 *
 * @param[out] result The rinha_value_t structure to store the parsed if statement.
 */
void rinha_parse_if_statement_(rinha_value_t *result);

/**
 * @brief Parse an identifier.
 */
void rinha_parser_identifier(void);

/**
 * @brief Parse a logical OR expression.
 *
 * This function parses a logical OR expression and populates the provided rinha_value_t structure.
 *
 * @param[out] result The rinha_value_t structure to store the parsed logical OR expression.
 *
 * @return `true` if the parsing is successful, `false` otherwise.
 */
bool rinha_parser_logical_or_(rinha_value_t *result);

/**
 * @brief Parse a term.
 *
 * This function parses a term and is marked as static, indicating it is used internally within the module.
 *
 * @param[out] result The rinha_value_t structure to store the parsed term.
 */
static void rinha_parse_term_(rinha_value_t *result);

/**
 * @brief Parse and calculate an expression.
 *
 * This function parses and calculates an expression and populates the provided rinha_value_t structure.
 *
 * @param[out] result The rinha_value_t structure to store the parsed and calculated expression.
 */
void rinha_parse_calc_(rinha_value_t *result);

/**
 * @brief Execute a function.
 *
 * This function executes a function with the provided arguments and populates the result in the provided rinha_value_t structure.
 *
 * @param[in] f The function to be executed.
 * @param[out] result The rinha_value_t structure to store the function execution result.
 */
void rinha_function_exec_(function_t *f, rinha_value_t *result);

/**
 * @brief Check if a token is a valid identifier.
 *
 * This function checks if a given token is a valid identifier.
 *
 * @param token The token to be checked.
 *
 * @return `1` if the token is a valid identifier, `0` otherwise.
 */
int is_valid_identifier(const char *token);

/**
 * @brief Get the next token in the input stream.
 *
 * This function retrieves the next token in the input stream.
 *
 * @return A pointer to the next token.
 */
token_t* rinha_next_token(void);
/**
 * @brief Get a function by its hash value.
 *
 * This function retrieves a function by its hash value.
 *
 * @param hash The hash value of the function.
 *
 * @return A pointer to the function, or `NULL` if not found.
 */
function_t* rinha_function_get_(int hash);

/**
 * @brief Print a Rinha value with optional line feed and debugging information.
 *
 * This function prints a Rinha value to the standard output stream, with the option
 * to add a line feed character and display debugging information.
 *
 * @param[in] value   The Rinha value to print.
 * @param[in] lf      Set to true to add a line feed character at the end, false otherwise.
 * @param[in] debug   Set to true to enable debugging information, false to skip it.
 */
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
