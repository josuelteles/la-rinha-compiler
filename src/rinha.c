/**
 * @file rinha.c
 *
 * @brief Rinha Language Interpreter - Compiler Development Challenge
 *
 * For more information, please visit the documentation at:
 * [rinha-de-compiler](https://github.com/aripiprazole/rinha-de-compiler)
 *
 * @author Josuel Teles
 * @date September 14, 2023
 */


#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdarg.h>
#include <sys/resource.h>

#include "rinha.h"


/**
 * @brief Just for fun
 *
 * This one you have to find out what it does
 */
static const char special_call[] = {0x63, 0x6F, 0x77, 0x73, 0x61, 0x79, 0x00};

static const char woc[] = {
    0x20, 0x20, 0x20, 0x5C, 0x20, 0x20, 0x20, 0x20, 0x5E, 0x5F, 0x5F, 0x5E,
    0x0A, 0x20, 0x20, 0x20, 0x20, 0x5C, 0x20, 0x20, 0x20, 0x28, 0x6F, 0x6F,
    0x29, 0x5C, 0x5F, 0x5F, 0x5F, 0x5F, 0x5F, 0x5F, 0x5F, 0x0A, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x28, 0x5F, 0x5F, 0x29, 0x5C, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x29, 0x5C, 0x2F, 0x5C, 0x0A, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x7C,
    0x7C, 0x2D, 0x2D, 0x2D, 0x2D, 0x77, 0x20, 0x7C, 0x0A, 0x20, 0x20, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x7C, 0x7C, 0x20,
    0x20, 0x20, 0x20, 0x20, 0x7C, 0x7C, 0x0A,
};

/**
 * @brief Flag to indicate if tests are running.
 */
static bool on_tests = false;

/**
 * @brief Array to store the source name
 * TODO: Config size
 */
static char source_name[128];

/**
 * @brief Flag to indicate whether the cache is enabled.
 *
 * This flag is configured in RINHA_CONFIG_CACHE_ENABLE.
 */
static bool cache_enabled = RINHA_CONFIG_CACHE_ENABLE;

/**
 * @brief Source code pointer.
 *
 * This pointer is used to store the source code of the Rinha program.
 */
static char *source_code = NULL;

/**
 * @brief Temporary buffer.
 *
 * A temporary character buffer with a maximum size defined by RINHA_CONFIG_STRING_VALUE_MAX.
 */
static char tmp[RINHA_CONFIG_STRING_VALUE_MAX] = {0};

/**
 * @brief Global stack.
 *
 * The main stack used in the Rinha interpreter.
 */
//static stack_t global;

/**
 * @brief Secondary stacks.
 *
 * An array of secondary stacks used in the Rinha interpreter.
 */
static stack_t st2[RINHA_CONFIG_STACK_SIZE];

/**
 * @brief Stack pointer.
 *
 * Pointer to the current stack, initially pointing to st2.
 */
static stack_t *stacks = st2;

/**
 * @brief Stack context pointer.
 *
 * Pointer used to manage the stack context.
 */
static stack_t *stack_ctx;

/**
 * @brief Count of function calls.
 *
 * Keeps track of the number of function calls made during execution.
 */
static int calls_count = 0;

/**
 * @brief Rinha stack pointer.
 *
 * Pointer to the current position in the Rinha stack.
 */
static int rinha_sp = 0;

/**
 * @brief Token count.
 *
 * Keeps track of the number of tokens encountered in the Rinha program.
 */
static int rinha_tok_count = 0;

/**
 * @brief Program counter.
 *
 * Keeps track of the program counter for Rinha program execution.
 */
static int rinha_pc = 0;

/**
 * @brief Current token context.
 *
 * Stores the current token context during Rinha program parsing and execution.
 */
static token_t rinha_current_token_ctx;

/**
 * @brief Token array.
 *
 * An array of tokens used in the Rinha interpreter, with a maximum size of 1000.
 */
static token_t tokens[RINHA_CONFIG_TOKENS_SIZE] = {0};

/**
 * @brief Function symbols array.
 *
 * An array that stores function symbols and their details, with a maximum size defined
 * by RINHA_CONFIG_SYMBOLS_SIZE.
 */
static function_t calls[RINHA_CONFIG_CALLS_SIZE];


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
void rinha_print_(rinha_value_t *value, bool lf, bool debug) {

    if(on_tests)
        return;

    if (!value) {
    fprintf(stderr, "Error: value is not defined\n");
    return;
  }

  char end_char = lf ? 0x0a : 0x00;

  switch (value->type) {
    case STRING:
      if (debug)
        fprintf(stdout, "\nSTRING (%ld): ->", strlen(value->string));
      fprintf(stdout, "%s%c", value->string, end_char);
      break;
    case FUNCTION:
      if (debug)
        fprintf(stdout, "\nFUNCTION: ->Hash(%d)",
             ( (function_t *) value->function)->hash);
      fprintf(stdout, "<#closure>%c", end_char);
      break;
    case INTEGER:
      if (debug)
        fprintf(stdout, "\nINTEGER: ->");
      fprintf(stdout, "%ld%c", value->number, end_char);
      break;
    case BOOLEAN:
      if (debug)
        fprintf(stdout, "\nBOOLEAN: ->");
      fprintf(stdout, "%s%c", value->boolean ? "true" : "false", end_char);
      break;
    case TUPLE:
      if (debug)
        fprintf(stdout, "\nTUPLE: ->");
      fprintf(stdout, "(");
      // Recursively print the elements of the tuple
      rinha_print_((rinha_value_t *)&value->tuple.first, false, debug);
      fprintf(stdout, ",");
      rinha_print_((rinha_value_t *)&value->tuple.second, false, debug);
      fprintf(stdout, ")\n");
      break;
    default:
      // Handle unknown value type
      fprintf(stdout, "\nUNKNOWN: ->\n");
      fprintf(stdout, "AS STRING  [%s]\n\n", value->string);
      fprintf(stdout, "AS NUMBER  [%ld]\n", value->number);
      fprintf(stdout, "AS BOOLEAN [%s]\n", BOOL_NAME(value->boolean));
  }
}

void rinha_print_debug_(rinha_value_t *value) {
  rinha_print_(value, true, true);
}

_RINHA_CALL_ static rinha_value_t rinha_value_number_set_(int64_t value) {
  rinha_value_t ret = {0};
  ret.type = INTEGER;
  ret.number = value;

  return ret;
}

_RINHA_CALL_ static rinha_value_t rinha_value_bool_set_(bool value) {
  rinha_value_t ret = {0};
  ret.type = BOOLEAN;
  ret.boolean = value;

  return ret;
}

_RINHA_CALL_ static rinha_value_t rinha_value_string_set_(char *value) {
  rinha_value_t ret = {0};
  ret.type = STRING;
  strncpy(ret.string, value, sizeof(ret.string));

  return ret;
}

_RINHA_CALL_ static rinha_value_t rinha_value_caller_set_(function_t *func) {
  rinha_value_t ret = {0};
  ret.type = FUNCTION;
  ret.function = func;

  return ret;
}

_RINHA_CALL_ static rinha_value_t
rinha_value_tuple_set_(rinha_value_t *first, rinha_value_t *second) {
  // tuple_t tuple = {0};
  rinha_value_t ret = {0};
  ret.type = TUPLE;

  rinha_value_t v1 = rinha_value_set_(*first);
  rinha_value_t v2 = rinha_value_set_(*second);

  ret.tuple.first = *((struct __primitive *)&v1);
  ret.tuple.second = *((struct __primitive *)&v2);

  return ret;
}

/**
 * @brief Create a Rinha value with the same type as the input value.
 *
 * This function creates a new Rinha value with the same type as the input value
 * and copies the value accordingly.
 *
 * @param[in] value  The input Rinha value.
 * @return A new Rinha value with the same type and value as the input.
 */
_RINHA_CALL_ rinha_value_t rinha_value_set_(rinha_value_t value) {
  switch (value.type) {
    case STRING:
      return rinha_value_string_set_(value.string);
    case BOOLEAN:
      return rinha_value_bool_set_(value.boolean);
    default:
      return rinha_value_number_set_(value.number);
  }
}

/**
 * @brief Calculate the hash value of a string using the djb2 algorithm.
 *
 * This function calculates the hash value of a string using the djb2 hashing algorithm.
 *
 * @param[in] str  The input string to hash.
 * @return The calculated hash value.
 */
inline static unsigned long rinha_hash_str_(char *str) {
  unsigned long hash = 5381;
  int c;
  char *ptr = str;

  while ((c = *ptr++)) {
    hash = ((hash << 5) + hash) + c;
  }

  return (hash % RINHA_CONFIG_SYMBOLS_SIZE);
}

/**
 * @brief Calculate a hash value for a combination of two integers.
 *
 * This function calculates a hash value for a combination of two integers, 'n' and 'k'.
 * It uses a simple formula: (n * 31 + k) % RINHA_CONFIG_CACHE_SIZE to generate the hash.
 * This particular hash function employs the multiplication by a prime number (31) to help
 * distribute the resulting hash values more evenly.
 *
 * @param[in] n The first integer.
 * @param[in] k The second integer.
 * @return The calculated hash value, which is within the range of [0, RINHA_CONFIG_CACHE_SIZE-1].
 */
inline int rinha_hash_num(int n, int k) {
    return (n * 31 + k) % RINHA_CONFIG_CACHE_SIZE;
}

/**
 * @brief Calculate a hash value for a function's stack context.
 *
 * This function calculates a hash value for a function's stack context based on its contents.
 * It iterates through the elements in the stack and combines their hash values to create
 * a unique hash for the entire stack context. The `rinha_hash_num` function is used to help
 * distribute the hash values more evenly.
 *
 * @param[in] f  The function whose stack context is being hashed.
 * @return The calculated hash value, which is within the range of [0, RINHA_CONFIG_CACHE_SIZE-1].
 */
unsigned int rinha_hash_stack_(function_t *f) {
  unsigned int hash = 0;

  for (register int i = 0; i < f->stack->count; i++) {
    rinha_value_t *v = &f->stack->mem[f->args.hash[i]].value;
    hash ^= (v->type == STRING)
        ? (unsigned int)(rinha_hash_str_(v->string))
        : v->number;

    hash = rinha_hash_num(hash, i);
  }
  return (hash % RINHA_CONFIG_CACHE_SIZE);
}

void rinha_print_statement_(rinha_value_t *value) {
  rinha_token_consume_(TOKEN_PRINT);
  rinha_token_consume_(TOKEN_LPAREN);

  token_t *nt = rinha_next_token();

  if (nt->type == TOKEN_FN) {
       *value = rinha_value_string_set_("<#closure>");
  } else {
      rinha_exec_expression_(value);
  }

  rinha_print_(value, /* line feed */ true, /* debug mode */ false);
}

token_type rinha_discover_token_typeype_(char *token) {
  if (strcmp(token, "let") == 0) {
    return TOKEN_LET;
  } else if (strcmp(token, "fn") == 0) {
    return TOKEN_FN;
  } else if (strcmp(token, "(") == 0) {
    return TOKEN_LPAREN;
  } else if (strcmp(token, ")") == 0) {
    return TOKEN_RPAREN;
  } else if (strcmp(token, "{") == 0) {
    return TOKEN_LBRACE;
  } else if (strcmp(token, "}") == 0) {
    return TOKEN_RBRACE;
  } else if (strcmp(token, ",") == 0) {
    return TOKEN_COMMA;
  } else if (strcmp(token, "'") == 0) {
    return TOKEN_APOSTROPHE;
  } else if (strcmp(token, "\"") == 0) {
    return TOKEN_QUOTE;
  } else if (strcmp(token, ";") == 0) {
    return TOKEN_SEMICOLON;
  } else if (strcmp(token, "=") == 0) {
    return TOKEN_ASSIGN;
  } else if (strcmp(token, "if") == 0) {
    return TOKEN_IF;
  } else if (strcmp(token, "else") == 0) {
    tokens[rinha_tok_count].value = rinha_value_bool_set_(false);
    return TOKEN_ELSE;
  } else if (strcmp(token, "true") == 0) {
    tokens[rinha_tok_count].value = rinha_value_bool_set_(true);
    return TOKEN_TRUE;
  } else if (strcmp(token, special_call) == 0) {
    return TOKEN_YASWOC;
  } else if (strcmp(token, "false") == 0) {
    return TOKEN_FALSE;
  } else if (strcmp(token, "print") == 0) {
    return TOKEN_PRINT;
  } else if (strcmp(token, "first") == 0) {
    return TOKEN_FIRST;
  } else if (strcmp(token, "second") == 0) {
    return TOKEN_SECOND;
  } else if (strcmp(token, "<") == 0) {
    return TOKEN_LT;
  } else if (strcmp(token, ">") == 0) {
    return TOKEN_GT;
  } else if (strcmp(token, "+") == 0) {
    return TOKEN_PLUS;
  } else if (strcmp(token, "-") == 0) {
    return TOKEN_MINUS;
  } else if (strcmp(token, "=>") == 0) {
    return TOKEN_ARROW;
  } else if (strcmp(token, "%") == 0) {
    return TOKEN_MOD;
  } else if (strcmp(token, "*") == 0) {
    return TOKEN_MULTIPLY;
  } else if (strcmp(token, "/") == 0) {
    return TOKEN_DIVIDE;
  } else if (strcmp(token, "&&") == 0) {
    return TOKEN_AND;
  } else if (strcmp(token, "||") == 0) {
    return TOKEN_OR;
  } else if (strcmp(token, "==") == 0) {
    return TOKEN_EQ;
  } else if (strcmp(token, "!=") == 0) {
    return TOKEN_NEQ;
  } else if (strcmp(token, ">=") == 0) {
    return TOKEN_GTE;
  } else if (strcmp(token, "_") == 0) {
    return TOKEN_WILDCARD;
  } else if (strcmp(token, "<=") == 0) {
    return TOKEN_LTE;
  } else if (isdigit(token[0])) {
    tokens[rinha_tok_count].value = rinha_value_number_set_(atol(token));
    return TOKEN_NUMBER;
  }

  tokens[rinha_tok_count].hash = rinha_hash_str_(token);

  return TOKEN_IDENTIFIER;
}

/**
 * @brief Set a variable in the current stack context.
 *
 * This function sets a variable in the specified stack context with the given value.
 * It also determines the variable type based on the value if it is not explicitly defined.
 *
 * @param[in] ctx    The stack context in which to set the variable.
 * @param[in] value  The value to set.
 * @param[in] hash   The hash value of the variable name.
 */
_RINHA_CALL_ void rinha_var_set_(stack_t *ctx, rinha_value_t *value, int hash) {
  // Determine the variable type if not explicitly defined
  value->type = (!value->type) ? (isdigit(value->string[0])) ? INTEGER : STRING : value->type;

  // Set the variable in the stack context
  ctx->mem[hash].value = *value;
  ctx->count++;
}

/**
 * @brief Get a variable from the current stack context.
 *
 * This function retrieves a variable from the specified stack context based on its hash value.
 * If the variable is undefined in the current context, it looks in the global context.
 *
 * @param[in] ctx   The stack context from which to retrieve the variable.
 * @param[in] hash  The hash value of the variable name.
 * @return A pointer to the retrieved variable.
 */
_RINHA_CALL_ rinha_value_t *rinha_var_get_(stack_t *ctx, int hash) {

  //local
  rinha_value_t *v = &ctx->mem[hash].value;
  v->hash = hash;

  //local
  if (v && v->type != UNDEFINED) {
    return v;
  }

  //closure local
  //Access the stack of the previous function
  if( rinha_sp > 1 ) {
     v = &stacks[rinha_sp-1].mem[hash].value;
  }

  if (v && v->type != UNDEFINED) {
    return v;
  }

  return &stacks[0].mem[hash].value; //global
}

/**
 * @brief Set function details in the global functions array.
 *
 * This function sets details for a function in the global functions array, such as the program
 * counter (PC). It increments the calls_count to keep track of the number of function calls.
 *
 * @param[in] pc    The program counter value for the function.
 * @param[in] hash  The hash value of the function name.
 * @return A pointer to the function details.
 */
_RINHA_CALL_ function_t *rinha_function_set_(int pc, int hash) {
  function_t *call = &calls[hash];
  call->pc = pc;
  call->hash = hash;
  call->cache_size = 0;
  calls_count++;
  return call;
}

/**
 * @brief Get function details from the global functions array.
 *
 * This function retrieves function details from the global functions array based on the hash value
 * of the function name. If the function is not found, it returns NULL.
 *
 * @param[in] hash  The hash value of the function name.
 * @return A pointer to the function details, or NULL if not found.
 */
_RINHA_CALL_ function_t *rinha_function_get_(int hash) {
  function_t *f = &calls[hash];
  return (f && f->pc) ? f : NULL;
}

/**
 * @brief Add a parameter hash to a function's argument list.
 *
 * This function adds a parameter hash to a function's argument list.
 *
 * @param[in,out] f     The function to which the parameter is added.
 * @param[in]     hash  The hash value of the parameter name.
 */
_RINHA_CALL_ void rinha_call_parameter_add(function_t *f, int hash) {
  f->args.hash[f->args.count++] = hash;
}

/**
 * @brief Initialize a function parameter in the function's stack context.
 *
 * This function initializes a function parameter in the stack context of the given function.
 * It copies the provided value to the specified parameter index in the stack context.
 *
 * @param[in] f       The function whose stack context is used.
 * @param[in] value   The value to set as the parameter's initial value.
 * @param[in] index   The index of the parameter in the argument list.
 */
_RINHA_CALL_ static void rinha_function_param_init_(function_t *f, rinha_value_t *value, int index) {
  // Set the parameter value in the function's stack context
  f->stack->mem[f->args.hash[index]].value = *value;

  // Increment the count of items in the stack context
  f->stack->count++;
}

/**
 * @brief Check if a character is a delimiter.
 *
 * This function checks if a given character is a delimiter, which is a character that
 * separates tokens in the Rinha language.
 *
 * @param[in] c   The character to check.
 * @return true if the character is a delimiter, false otherwise.
 */
inline bool rinha_isdelim(char c) {
  return (strchr("()\"'{},+-*/%;", c) != NULL);
}

/**
 * @brief Print an error message with context information.
 *
 * This function prints an error message along with context information, such as the error location,
 * the token causing the error, and the relevant source code snippet.
 *
 * @param[in] token  The token associated with the error.
 * @param[in] fmt    The error message format string (supports variable arguments).
 * @param[in] ...    Variable arguments to be formatted according to fmt.
 */
void rinha_error(const token_t token, const char *fmt, ...) {

  FILE *RINHA_OUTERR = stderr;

  fprintf(RINHA_OUTERR,
          TEXT_RED("\nError: "));

  va_list args;
  va_start(args, fmt);
  vfprintf(RINHA_OUTERR, fmt, args);
  va_end(args);

  fprintf(
      RINHA_OUTERR,
      " ( Token: " TEXT_GREEN("%s") ", Type: " TEXT_WHITE(
          "%d") ", File: " TEXT_WHITE("%s") ", Line: " TEXT_WHITE("%d") ", "
          "Pos:" " " TEXT_WHITE("%d") ", stack_t: " TEXT_WHITE(
          "%"
       "d") " )\n\n",
      token.lexeme, token.type, source_name, token.line, token.pos,
      rinha_sp);

  const char *code = source_code;
  const char *start = code;
  const char *end = code;

  for (register int i = 1; i < token.line; i++) {
    while (*end != '\n' && *end != '\0') {
      end++;
    }
    if (*end == '\0') {
      break;
    }
    end++;
    start = end;
  }

  while (*end != '\n' && *end != '\0') {
    end++;
  }

  size_t len = end - start;

  fwrite(start, 1, len, RINHA_OUTERR);
  putchar('\n');

  for (int i = 1; i < token.pos; i++) {
    putchar(' ');
  }
  puts("^"); // Putz...

  exit(EXIT_FAILURE);
}

/**
 * @brief Tokenize a Rinha source code into tokens.
 *
 * This function takes a Rinha source code string and tokenizes it into individual tokens.
 *
 * @param[in,out] code_ptr       A pointer to the source code string (updated as tokens are processed).
 * @param[out]    tokens         An array to store the generated tokens.
 * @param[in,out] rinha_tok_count A pointer to an integer to store the total token count (updated).
 */
void rinha_tokenize_(char **code_ptr, token_t *tokens, int *rinha_tok_count) {
  int line_number = 1;
  int token_position = 0;

  while (**code_ptr != '\0') {
    token_type type = TOKEN_UNDEFINED;

    while (isspace(**code_ptr)) {
      if (**code_ptr == '\n') {
        line_number++;
        token_position = 0;
      }
      (*code_ptr)++;
      token_position++;
    }

    if (**code_ptr == '\0') {
      break;
    }

    char *token = *code_ptr;
    int current_line = line_number;
    int current_position = token_position;

    if (**code_ptr == '\'' || **code_ptr == '"') {
      char quote = **code_ptr;
      token = ++(*code_ptr);

      while (**code_ptr != '\0' && **code_ptr != quote) {
        (*code_ptr)++;
        token_position++;
      }

      type = TOKEN_STRING;
    } else if (rinha_isdelim(**code_ptr)) {
      (*code_ptr)++;
      token_position++;
    } else {
      while (**code_ptr != '\0' && !isspace(**code_ptr) &&
             !rinha_isdelim(**code_ptr)) {
        (*code_ptr)++;
        token_position++;
      }
    }

    size_t tokenLength = *code_ptr - token;
    strncpy(tokens[*rinha_tok_count].lexeme, token, tokenLength);
    tokens[*rinha_tok_count].lexeme[tokenLength] = '\0';

    if (type == TOKEN_STRING) {
      tokens[*rinha_tok_count].type = type;
      tokens[*rinha_tok_count].value =
          rinha_value_string_set_(tokens[*rinha_tok_count].lexeme);
      (*code_ptr)++;
      token_position++;
    } else {
      tokens[*rinha_tok_count].type =
          rinha_discover_token_typeype_(tokens[*rinha_tok_count].lexeme);
    }

    tokens[*rinha_tok_count].line = current_line;
    tokens[*rinha_tok_count].pos = current_position;

    (*rinha_tok_count)++;
  }
}

int rinha_check_valid_identifier(const char *token) {
  if (!isalpha(token[0]) && token[0] != '_') {
    return 0;
  }

  for (register int i = 1; token[i] != '\0'; i++) {
    if (!isalnum(token[i]) && token[i] != '_') {
      return 0;
    }
  }

  return 1;
}

/**
 * @brief Advance the current token to the next token in the token stream.
 */
void rinha_token_advance() {
  //DEBUG BREAK;
  rinha_current_token_ctx = tokens[++rinha_pc];

}

/**
 * @brief Move the current token back to the previous token in the token stream.
 */
void rinha_token_previous() {
  rinha_current_token_ctx = tokens[--rinha_pc];
}

/**
 * @brief Get a pointer to the next token in the token stream.
 *
 * @return A pointer to the next token.
 */
token_t *rinha_next_token(void) {
  return &tokens[rinha_pc];
}

/**
 * @brief Consume the current token if its type matches the expected type.
 *
 * If the current token's type matches the expected type, it is consumed (advanced);
 * otherwise, an error message is generated.
 *
 * @param[in] expected_type  The expected token type.
 */
void rinha_token_consume_(token_type expected_type) {
  if (rinha_current_token_ctx.type == expected_type) {
    rinha_token_advance();
  } else {
    rinha_error(rinha_current_token_ctx,
                "Consume token: Expected an identifier (%d) ", expected_type);
  }
}

/**
 * @brief Parse a Rinha program.
 *
 * This function parses a Rinha program by repeatedly calling the statement parsing function
 * until the end of the token stream (EOF) is reached.
 *
 * @param[in,out] ret  A pointer to the result value of the program (updated during parsing).
 */
void rinha_exec_program_(rinha_value_t *ret) {
  while (rinha_current_token_ctx.type != TOKEN_EOF) {
    rinha_exec_statement_(ret);
  }
}

/**
 * @brief Parse the "first" function to extract the first element of a tuple.
 *
 * This function parses the "first" function, expecting it to be called with a tuple
 * as an argument. It extracts and returns the first element of the tuple.
 *
 * @param[in,out] ret  A pointer to the result value (the first element of the tuple).
 */
void rinha_exec_first(rinha_value_t *ret) {
  rinha_token_consume_(TOKEN_FIRST);
  rinha_token_consume_(TOKEN_LPAREN);
  rinha_exec_expression_(ret);

  if (ret->type != TUPLE) {
    rinha_token_previous();
    rinha_error(rinha_current_token_ctx,
                "first: Invalid argument, expected a tuple ");
  }

  *ret = *((rinha_value_t *)&ret->tuple.first);
  rinha_token_consume_(TOKEN_RPAREN);
}

/**
 * @brief Parse the "second" function to extract the second element of a tuple.
 *
 * This function parses the "second" function, expecting it to be called with a tuple
 * as an argument. It extracts and returns the second element of the tuple.
 *
 * @param[in,out] ret  A pointer to the result value (the second element of the tuple).
 */
void rinha_exec_second(rinha_value_t *ret) {
  rinha_token_consume_(TOKEN_SECOND);
  rinha_token_consume_(TOKEN_LPAREN);
  rinha_exec_expression_(ret);

  if (ret->type != TUPLE) {
    rinha_token_previous();
    rinha_error(rinha_current_token_ctx,
                "second: Invalid argument, expected a tuple ");
  }

  *ret = *((rinha_value_t *)&ret->tuple.second);
  rinha_token_consume_(TOKEN_RPAREN);
}

inline static void rinha_expression_jump(void) {
  while ( rinha_current_token_ctx.type != TOKEN_SEMICOLON
          && rinha_current_token_ctx.type != TOKEN_EOF) {
    rinha_token_advance();
  }
}

/**
 * @brief Jump over a Rinha code block.
 *
 * This function skips over a Rinha code block enclosed in curly braces.
 */
inline static void rinha_block_jump_(void) {
  int open_braces = 1;

  //Gambiarra sintantica...
  if (rinha_current_token_ctx.type != TOKEN_LBRACE) {
      rinha_expression_jump();
      return;
  }

  rinha_token_consume_(TOKEN_LBRACE);

  while (open_braces && rinha_current_token_ctx.type != TOKEN_EOF) {

    rinha_token_advance();
    if (rinha_current_token_ctx.type == TOKEN_LBRACE) {
      open_braces++;
    } else if (rinha_current_token_ctx.type == TOKEN_RBRACE) {
      open_braces--;
    }
  }
  rinha_token_consume_(TOKEN_RBRACE);
}

/**
 * @brief Parse a function closure.
 *
 * This function parses a function closure, which is defined using the "fn" keyword.
 * It extracts the parameters and body of the closure and stores them in a function structure.
 *
 * @param[in] hash  The hash value for the closure's identifier.
 */
void rinha_exec_closure(int hash) {

  function_t *call = rinha_function_set_(rinha_pc, hash);

  rinha_token_consume_(TOKEN_FN);
  rinha_token_consume_(TOKEN_LPAREN);
  while (rinha_current_token_ctx.type != TOKEN_RPAREN) {

    if (rinha_current_token_ctx.type == TOKEN_IDENTIFIER) {

      rinha_call_parameter_add(call, rinha_current_token_ctx.hash);

      rinha_token_consume_(TOKEN_IDENTIFIER);
    } else if (rinha_current_token_ctx.type == TOKEN_COMMA) {
      rinha_token_consume_(TOKEN_COMMA);
    }
  }

  rinha_value_t value = rinha_value_caller_set_(call);
  rinha_var_set_(stack_ctx, &value, hash);

  rinha_token_consume_(TOKEN_RPAREN);
  rinha_token_consume_(TOKEN_ARROW);
  call->pc = rinha_pc;

  rinha_block_jump_();
}

/**
 * @brief Parse a Rinha statement and update the result value.
 *
 * This function parses a Rinha statement based on the current token and updates
 * the result value pointed to by 'ret' accordingly.
 *
 * @param[in,out] ret  A pointer to the result value (updated during parsing).
 */
void rinha_exec_statement_(rinha_value_t *ret) {

  switch (rinha_current_token_ctx.type) {
  case TOKEN_LET: {
    rinha_token_consume_(TOKEN_LET);
    int hash = rinha_current_token_ctx.hash;
    int type = rinha_current_token_ctx.type;

    rinha_exec_identifier();
    rinha_token_consume_(TOKEN_ASSIGN);

    if (type == TOKEN_WILDCARD) {
      return; // that's ok, next instruction
    }

    if (rinha_current_token_ctx.type == TOKEN_FN) {
      rinha_exec_closure(hash);
      return;
    }
    rinha_value_t value = rinha_exec_expression_(ret);
    rinha_var_set_(stack_ctx, &value, hash);
    *ret = value;
  } break;
  case TOKEN_FN:
    rinha_exec_closure(rinha_current_token_ctx.hash);
    break;
  case TOKEN_FIRST:
    rinha_exec_first(ret);
    break;
  case TOKEN_SECOND:
    rinha_exec_second(ret);
    break;
  case TOKEN_PRINT:
    rinha_print_statement_(ret);
    break;
  case TOKEN_RBRACE:
    rinha_token_advance();
    break;
  case TOKEN_LPAREN:
    rinha_token_advance();
    break;
  case TOKEN_SEMICOLON:
    rinha_token_advance();
    break;
  case TOKEN_YASWOC:
    rinha_yaswoc(ret);
    break;
  case TOKEN_RPAREN:
    rinha_token_advance();
    break;
  case TOKEN_IDENTIFIER:
    rinha_exec_expression_(ret);
    break;
  case TOKEN_IF:
    rinha_exec_if_statement_(ret);
    break;
  case TOKEN_NUMBER:
    *ret = rinha_current_token_ctx.value;
    rinha_token_consume_(rinha_current_token_ctx.type);
    break;
  case TOKEN_EOF:
    break;
  default:
    rinha_error(rinha_current_token_ctx, "Unexpected token");
    break;
  }
}

rinha_value_t rinha_exec_comparison_(rinha_value_t *left) {
  rinha_exec_calc_(left);

  while (rinha_current_token_ctx.type == TOKEN_EQ ||
         rinha_current_token_ctx.type == TOKEN_GTE ||
         rinha_current_token_ctx.type == TOKEN_LTE ||
         rinha_current_token_ctx.type == TOKEN_GT ||
         rinha_current_token_ctx.type == TOKEN_LT) {

    token_type op_type = rinha_current_token_ctx.type;

    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_exec_calc_(&right);

    if (op_type == TOKEN_EQ) {
      return rinha_value_bool_set_(left->number == right.number);
    } else if (op_type == TOKEN_GTE) {
      return rinha_value_bool_set_(left->number >= right.number);
    } else if (op_type == TOKEN_LTE) {
      return rinha_value_bool_set_(left->number <= right.number);
    } else if (op_type == TOKEN_LT) {
      return rinha_value_bool_set_(left->number < right.number);
    } else if (op_type == TOKEN_GT) {
      return rinha_value_bool_set_(left->number > right.number);
    } else if (op_type != TOKEN_NEQ) {
      return rinha_value_bool_set_(left->number != right.number);
    }
  }

  return *left;

  //return rinha_value_bool_set_(left->number != 0);
}

/**
 * @brief Parse a logical AND expression.
 *
 * This function parses a logical AND expression and returns the result.
 *
 * @param[in,out] left  A pointer to the left operand (updated during parsing).
 * @return The result of the logical AND expression.
 */
rinha_value_t rinha_exec_logical_and_(rinha_value_t *left) {
  //bool ret = rinha_exec_comparison_(left);
  rinha_value_t ret = rinha_exec_comparison_(left);

  while (rinha_current_token_ctx.type == TOKEN_AND) {
    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_value_t cmp = rinha_exec_comparison_(&right);
    ret.boolean = (ret.boolean && cmp.boolean);
  }

  return ret;
}

/**
 * @brief Parse a logical OR expression.
 *
 * This function parses a logical OR expression and returns the result.
 *
 * @param[in,out] left  A pointer to the left operand (updated during parsing).
 * @return The result of the logical OR expression.
 */
rinha_value_t rinha_exec_logical_or_(rinha_value_t *left) {
  rinha_value_t ret = rinha_exec_logical_and_(left);

  while (rinha_current_token_ctx.type == TOKEN_OR) {
    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_value_t cmp = rinha_exec_logical_and_(&right);
    ret.boolean = (ret.boolean || cmp.boolean);
  }

  return ret;
}

/**
 * @brief Parse an assignment expression.
 *
 * This function parses an assignment expression and returns the result.
 *
 * @param[in,out] left  A pointer to the left operand (updated during parsing).
 * @return The result of the assignment expression.
 */
rinha_value_t rinha_exec_assign(rinha_value_t *left) {
  int hash = rinha_current_token_ctx.hash;
  rinha_value_t ret = rinha_exec_logical_or_(left);

  if (rinha_current_token_ctx.type == TOKEN_ASSIGN) {
    rinha_value_t *var = rinha_var_get_(stack_ctx, hash);
    rinha_token_advance();
    ret = rinha_exec_assign(left);
    *var = *left;
  }

  return ret;
}

// Atomic

/**
 * @brief Parse a primary expression and update the result value.
 *
 * This function parses a primary expression, which can be an identifier, a function call,
 * a number, or a string. It updates the result value pointed to by 'ret' accordingly.
 *
 * @param[in,out] ret  A pointer to the result value (updated during parsing).
 */
void rinha_exec_primary_(rinha_value_t *ret) {
  switch (rinha_current_token_ctx.type) {
  case TOKEN_IDENTIFIER:
    {
      rinha_value_t *v = rinha_var_get_(stack_ctx, rinha_current_token_ctx.hash);
      if (v && v->type != UNDEFINED) {
        rinha_token_advance();

        if(v->type == FUNCTION) {
           rinha_function_exec_((function_t *) v->function , ret);
           return;
        }
        *ret = *v;
        return;
      }
      rinha_error(rinha_current_token_ctx, "Undefined symbol");
    }
    break;
  case TOKEN_FN:
    rinha_exec_closure(rinha_current_token_ctx.hash);
    /*
    function_t *c = rinha_function_get_(hash);
    if (c) {
      rinha_pc = _pc-1;
      rinha_token_advance();
      rinha_function_exec_(c, ret);
      //return;
    }
    */
    rinha_token_advance();
    break;
  case TOKEN_NUMBER:
    *ret = rinha_current_token_ctx.value;
    (*ret).type = INTEGER;
    rinha_token_advance();
    break;
  case TOKEN_STRING:
    *ret = rinha_current_token_ctx.value;
    (*ret).type = STRING;
    rinha_token_advance();
    break;
  case TOKEN_LPAREN:
    rinha_token_advance();
    rinha_exec_expression_(ret);

    if (rinha_current_token_ctx.type == TOKEN_COMMA) {
      rinha_token_consume_(TOKEN_COMMA);

      rinha_value_t second = {0};
      rinha_exec_expression_(&second);
      *ret = rinha_value_tuple_set_(ret, &second);
    }

    rinha_token_advance();
    break;
  case TOKEN_TRUE:
    *ret = rinha_value_bool_set_(true);
    rinha_token_advance();
    break;
  case TOKEN_FALSE:
    *ret = rinha_value_bool_set_(false);
    rinha_token_advance();
    break;
  case TOKEN_FIRST:
    rinha_exec_first(ret);
    break;
  case TOKEN_SECOND:
    rinha_exec_second(ret);
    break;
  case TOKEN_PRINT:
    rinha_print_statement_(ret);
    rinha_token_advance();
    break;
  case TOKEN_IF:
    rinha_exec_if_statement_(ret);
    break;
  default:
    rinha_error(rinha_current_token_ctx, "Token undefined");
  }
}

_RINHA_CALL_ void rinha_exec_term_(rinha_value_t *left) {
  rinha_exec_primary_(left);

  while (rinha_current_token_ctx.type == TOKEN_MULTIPLY ||
         rinha_current_token_ctx.type == TOKEN_DIVIDE ||
         rinha_current_token_ctx.type == TOKEN_MOD) {

    token_type op_type = rinha_current_token_ctx.type;

    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_exec_primary_(&right);

    switch (op_type) {
    case TOKEN_MULTIPLY:
      left->number *= right.number;
      break;
    case TOKEN_DIVIDE:
      left->number /= right.number;
      break;
    case TOKEN_MOD:
      left->number %= right.number;
      break;
    }
  }
}

/**
 * @brief Concatenate two Rinha values and store the result in the left value.
 *
 * This function concatenates two Rinha values (left and right) and stores the result in
 * the left value. The concatenation is performed based on the types of the values.
 *
 * @param[in,out] left   A pointer to the left operand and the destination for the result.
 * @param[in]     right  A pointer to the right operand.
 */
_RINHA_CALL_ static void rinha_value_concat_(rinha_value_t *left, rinha_value_t *right) {

    // Check the types of the left and right values for concatenation
  if (left->type == INTEGER && right->type == STRING) {
    // Concatenate an integer and a string
    sprintf(tmp, "%ld%s", left->number, right->string);
  } else if (left->type == STRING && right->type == INTEGER) {
    // Concatenate a string and an integer
    sprintf(tmp, "%s%ld", left->string, right->number);
  } else if (left->type == STRING && right->type == BOOLEAN) {
    // Concatenate a string and a boolean
    sprintf(tmp, "%s%s", left->string, BOOL_NAME(right->boolean));
  } else if (left->type == BOOLEAN && right->type == STRING) {
    // Concatenate a boolean and a string
    sprintf(tmp, "%s%s", BOOL_NAME(left->boolean), right->string);
  } else {
    // Concatenate two strings or unsupported types
    sprintf(tmp, "%s%s", left->string, right->string);
  }

  strncpy(left->string, tmp, RINHA_CONFIG_STRING_VALUE_MAX);
  left->type = STRING;
}

void rinha_exec_calc_(rinha_value_t *left) {
  rinha_exec_term_(left);
  while (rinha_current_token_ctx.type == TOKEN_PLUS ||
         rinha_current_token_ctx.type == TOKEN_MINUS) {

    token_type op_type = rinha_current_token_ctx.type;
    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_exec_term_(&right);

    if (op_type == TOKEN_PLUS &&
        (left->type != INTEGER || right.type != INTEGER)) {
      rinha_value_concat_(left, &right);
      continue;
    }

    if (op_type == TOKEN_PLUS) {
      left->number += right.number;
    } else {
      left->number -= right.number;
    }
    left->type = INTEGER;
  }
}


/**
 * @brief Get a cached value from the memoization cache.
 *
 * This function retrieves a cached value from the memoization cache associated with a function call.
 * It checks if the cache is enabled and if the value is cached for the given hash.
 *
 * @param[in]  call  A pointer to the function call structure.
 * @param[out] ret   A pointer to store the cached value (if found).
 * @param[in]  hash  The hash value used as the cache key.
 *
 * @return 'true' if the value is cached and retrieved, 'false' otherwise.
 */
bool rinha_call_memo_cache_get_(function_t *call, rinha_value_t *ret, int hash) {
#if RINHA_CONFIG_CACHE_ENABLE == true

  if(!cache_enabled)
    return false;

  cache_t *cache = &call->cache[hash];

  if (!cache->cached)
    return false;

  *ret = cache->value;

  return true;
#else
  return false;
#endif
}

/**
 * @brief Set a value in the memoization cache.
 *
 * This function stores a value in the memoization cache associated with a function call.
 * It checks if the cache is enabled and whether a collision occurred before caching.
 *
 * @param[in,out] call  A pointer to the function call structure.
 * @param[in]     ret   A pointer to the value to be cached.
 * @param[in]     hash  The hash value used as the cache key.
 */
void rinha_call_memo_cache_set_(function_t *call, rinha_value_t *value, int hash) {
#if RINHA_CONFIG_CACHE_ENABLE == true
  if(!cache_enabled)
    return;

  cache_t *cache = &call->cache[hash];

  // Check for collisions; if a value is already cached, do nothing
  if (cache->cached)
  {
    cache_enabled = false;
    return;
  }

  cache->value = *value;
  cache->cached = true;
#endif
}


/**
 * @brief Parse an expression and update the result value.
 *
 * This function parses an expression and updates the result value pointed to by 'ret'.
 * The specific expression parsing logic is delegated to 'rinha_exec_assign'.
 *
 * @param[in,out] ret  A pointer to the result value (updated during parsing).
 */
rinha_value_t rinha_exec_expression_(rinha_value_t *ret) {
  return rinha_exec_assign(ret);
}


/**
 * @brief Execute a Rinha function call.
 *
 * This function executes a Rinha function call, including parsing its arguments, checking memoization
 * cache, and executing the function's code block.
 *
 * @param[in]  call  A pointer to the function call structure.
 * @param[out] ret   A pointer to store the return value (updated during execution).
 */
void rinha_function_exec_(function_t *call, rinha_value_t *ret) {

  token_t *nt = rinha_next_token();

  if (nt->type != TOKEN_LPAREN) {
      *ret = rinha_value_caller_set_(call);
      //rinha_token_advance();
      return;
  }

  rinha_token_previous();

  stack_ctx = &stacks[rinha_sp];

  if(rinha_sp+1 >= RINHA_CONFIG_STACK_SIZE) {
    rinha_error(rinha_current_token_ctx, "Stack overflow!");
  }

  call->stack = &stacks[++rinha_sp];
  rinha_token_advance();

  rinha_token_consume_(TOKEN_LPAREN);

  // Parse function arguments
  for (register int i = 0; rinha_current_token_ctx.type != TOKEN_RPAREN; ++i) {
    rinha_exec_expression_(ret);

    if (rinha_current_token_ctx.type == TOKEN_COMMA) {
      rinha_token_consume_(TOKEN_COMMA);
    }

    rinha_function_param_init_(call, ret, i);
  }

  unsigned int hash = rinha_hash_stack_(call);

  int current_pc = rinha_pc;

  if ( !rinha_call_memo_cache_get_(call, ret, hash)) {
    // Function not memoized; execute the function's block
    // TODO: Refactor these context flags
    stack_ctx = call->stack;
    rinha_pc = call->pc - 1;
    rinha_token_advance();
    rinha_exec_block_(ret);
    rinha_call_memo_cache_set_(call, ret, hash);
  }

  rinha_sp--;
  call->stack->count = 0;
  stack_ctx = call->stack = &stacks[rinha_sp];
  rinha_pc = current_pc;
  rinha_token_advance();
}


/**
 * @brief Parse a Rinha block of code.
 *
 * This function parses a block of Rinha code enclosed in curly braces, executing
 * each statement within the block.
 *
 * @param[out] ret  A pointer to store the return value (updated during execution).
 */
void rinha_exec_block_(rinha_value_t *ret) {

  //Gambiarra sintantica...
  if (rinha_current_token_ctx.type != TOKEN_LBRACE) {
      rinha_exec_statement_(ret);
      return;
  }

  rinha_token_consume_(TOKEN_LBRACE);

  while (rinha_current_token_ctx.type != TOKEN_RBRACE) {
    rinha_exec_statement_(ret);
  }

  rinha_token_consume_(TOKEN_RBRACE);
}

void rinha_exec_if_statement_(rinha_value_t *ret) {
  rinha_token_consume_(TOKEN_IF);
  rinha_token_consume_(TOKEN_LPAREN);

  rinha_value_t cond = rinha_exec_logical_or_(ret);

  rinha_token_consume_(TOKEN_RPAREN);

  if (cond.boolean) {

    rinha_exec_block_(ret);
    int tmp_pc = rinha_pc;

    if (rinha_current_token_ctx.jmp_pc1) {
      rinha_pc = rinha_current_token_ctx.jmp_pc1;
      rinha_token_advance();
      return;
    }
    if (rinha_current_token_ctx.type == TOKEN_ELSE) {
      rinha_token_consume_(TOKEN_ELSE);
      rinha_block_jump_();
    }
    tokens[tmp_pc].jmp_pc1 = (rinha_pc - 1);
  } else {
    int tmp_pc = rinha_pc;

    if (!rinha_current_token_ctx.jmp_pc2) {
      rinha_block_jump_();
      //rinha_token_advance();
      tokens[tmp_pc].jmp_pc2 = (rinha_pc - 1);
    } else {
      rinha_pc = rinha_current_token_ctx.jmp_pc2;
      rinha_token_advance();
    }

    if (rinha_current_token_ctx.type == TOKEN_ELSE) {
      rinha_token_consume_(TOKEN_ELSE);
      rinha_exec_block_(ret);
    }
  }
}

void rinha_exec_identifier(void) {
  if (rinha_current_token_ctx.type == TOKEN_IDENTIFIER ||
      rinha_current_token_ctx.type == TOKEN_WILDCARD) {
    rinha_token_advance();
  } else {
    rinha_error(rinha_current_token_ctx, "Expected an identifier ");
  }
}

//Surprise...
void rinha_yaswoc(rinha_value_t *value) {

  RINHA_UNUSED_PARAM(value);

  rinha_token_consume_(TOKEN_YASWOC);
  rinha_token_consume_(TOKEN_LPAREN);

  char *dialog = rinha_current_token_ctx.lexeme;
  rinha_value_t v;
  v.type = STRING;
  int w = sizeof(woc) / sizeof(woc[0]);
  int l = strlen(dialog);
  int i = 1;

  v.string[0] = 0x20;
  for (; i < l; ++i)
    v.string[i] = 0x5F;
  v.string[i++] = 0x0A;
  v.string[i++] = 0x3C;
  v.string[i++] = 0x20;
  for (int j = 0; j < l; ++j)
    v.string[i++] = dialog[j];
  v.string[i++] = 0x20;
  v.string[i++] = 0x3E;
  v.string[i++] = 0x0A;
  v.string[i++] = 0x20;
  for (int j = 0; j < l; ++j)
    v.string[i++] = 0x2D;
  v.string[i++] = 0x0A;
  for (int j = 0; j < w; ++j)
    v.string[i++] = woc[j];
  v.string[i++] = 0x0A;
  v.string[i++] = 0x00;

  rinha_print_(&v, true, false);
  rinha_token_advance();
}

void rinha_clear_stack(void) {

  memset(tokens, 0, sizeof(tokens));
  memset(calls, 0, sizeof(calls));
  memset(tmp, 0, sizeof(tmp));
  memset(st2, 0, sizeof(st2));
}

inline static void rinha_clear_context(void) {
  stacks          = st2;
  stack_ctx       = NULL;
  calls_count     = 0;
  rinha_sp        = 0;
  rinha_pc        = 0;
  rinha_tok_count = 0;
  on_tests        = false;
}

/**
 * @brief Execute a Rinha script.
 *
 * This function executes a Rinha script, parsing and interpreting the provided script code.
 *
 * @param name Script name.
 * @param script The Rinha script code to execute.
 * @param[out] response The result of script execution.
 * @param test Set to true if running in test mode, false otherwise.
 *
 * @return `true` if the script executed successfully, `false` on failure.
 */
bool rinha_script_exec(char *name, char *script, rinha_value_t *response, bool test) {

    rinha_clear_context();

    strcpy(source_name, name);
    on_tests = test;

    stack_ctx = stacks;

    char *code_ptr = source_code = script;

    while (*code_ptr != '\0') {
      rinha_tokenize_(&code_ptr, tokens, &rinha_tok_count);
    }

    tokens[rinha_tok_count++].type = TOKEN_EOF;

    // Initialize current token
    rinha_current_token_ctx = tokens[0];
    rinha_value_t ret = {0};

    rinha_exec_program_(&ret);
    *response = ret;

    return true;
}

