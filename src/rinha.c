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
 * @brief Source code pointer.
 *
 * This pointer is used to store the source code of the Rinha program.
 */
static char *source_code = NULL;

/**
 * @brief Temporary buffer.
 *
 * A temporary character buffer with a maximum size defined by RINHA_CONFIG_STRING_VALUE_SIZE.
 */
static char tmp[RINHA_CONFIG_STRING_VALUE_SIZE] = {0};

/**
 * @brief Secondary stacks.
 *
 * An array of secondary stacks used in the Rinha interpreter.
 */
//static stack_t st2[RINHA_CONFIG_STACK_SIZE];

/**
 * @brief Stack pointer.
 *
 * Pointer to the current stack, initially pointing to st2.
 */
static stack_t *stacks = NULL; //st2;

/**
 * @brief Stack context pointer.
 *
 * Pointer used to manage the stack context.
 */
static stack_t *stack_ctx;

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
static token_t *rinha_current_token_ctx;

/**
 * @brief Token array.
 *
 * An array of tokens used in the Rinha interpreter, with a maximum size of 1000.
 */
static token_t *tokens = NULL; //[RINHA_CONFIG_TOKENS_SIZE] = {0};

/**
 * @brief Function symbols array.
 *
 * An array that stores function symbols and their details, with a maximum size defined
 * by RINHA_CONFIG_SYMBOLS_SIZE.
 */
static function_t calls[RINHA_CONFIG_CALLS_SIZE];

static bool cache_enabled = RINHA_CONFIG_CACHE_ENABLE;
static int symref = 0;

static char string_pool[RINHA_CONFIG_STRING_POOL_SIZE][RINHA_CONFIG_STRING_VALUE_SIZE];

inline static char *rinha_alloc_static_string(void) {
   static uint32_t index = 0;
   return string_pool[++index % RINHA_CONFIG_STRING_POOL_SIZE];
}

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

  if(on_tests && !debug)
    return;

  if (!value) {
    fprintf(stderr, "Error: value is not defined\n");
    return;
  }

  char end_char = lf ? 0x0a : 0x00;

  switch (value->type) {
    case STRING:
      if (debug)
        fprintf(stdout, "\nSTRING (%ld): ->", value->string
                ? strlen(value->string) : 0);
      fprintf(stdout, "%s%c", value->string ? value->string : "NULL", end_char);
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
      fprintf(stdout, "%s%c", BOOL_NAME(value->boolean), end_char);
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

_RINHA_CALL_ static rinha_value_t rinha_value_number_set_(RINHA_WORD value) {
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
  ret.string = rinha_alloc_static_string();
  strncpy(ret.string, value, RINHA_CONFIG_STRING_VALUE_SIZE);

  return ret;
}

_RINHA_CALL_ static void  rinha_value_caller_set_(rinha_value_t *value, function_t *func) {
  value->type = FUNCTION;
  value->function = func;
}

_RINHA_CALL_ static rinha_value_t
rinha_value_tuple_set_(rinha_value_t *first, rinha_value_t *second) {
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

unsigned int rinha_create_anonymous_hash(char *token) {
  return ++symref;
}

token_t *rinha_find_token(char *lexname) {
  for(register int i=0; i < rinha_tok_count+1; ++i) {

      token_t *t = &tokens[i];

      switch(t->type) {
        case TOKEN_IDENTIFIER:
        case TOKEN_FN:
          if (strcmp(lexname, t->lexname) == 0) {
            return t;
          }
      }
  }

  return NULL;
}

//I'am just trying to avoid more colisions here
int rinha_create_sym_ref(char *lexname) {

  token_t *t = rinha_find_token(lexname);

  if (t) {
    return (!t->hash) ?
        ++symref : t->hash;
  }

  return rinha_hash_str_(lexname);
}


void rinha_print_statement_(rinha_value_t *value) {
  rinha_token_consume_(TOKEN_PRINT);
  rinha_token_consume_(TOKEN_LPAREN);

  token_t *nt = rinha_next_token();
  rinha_exec_expression_(value);
  rinha_print_(value, /* line feed */ true, /* debug mode */ false);
  cache_enabled = false;
}

token_type rinha_discover_token_typeype_(char *token) {

  if (strcmp(token, "let") == 0) {
    return TOKEN_LET;
  } else if (strcmp(token, "fn") == 0) {
    tokens[rinha_tok_count].hash = rinha_create_anonymous_hash(token);
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
    return TOKEN_ELSE;
  } else if (strcmp(token, "true") == 0) {
    tokens[rinha_tok_count].value = rinha_value_bool_set_(true);
    return TOKEN_TRUE;
  } else if (strcmp(token, "false") == 0) {
    tokens[rinha_tok_count].value = rinha_value_bool_set_(false);
    return TOKEN_FALSE;
  } else if (strcmp(token, special_call) == 0) {
    return TOKEN_YASWOC;
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
  // FIXME:
  // Determine the variable type if not explicitly defined
  value->type = (!value->type) ? (isdigit(value->string[0]))
	  ? INTEGER : STRING : value->type;

  // Set the variable in the stack context
  rinha_var_copy( &ctx->mem[hash].value, value);

  ++ctx->count;
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
_RINHA_CALL_ function_t *rinha_function_set_(token_t *pc, int hash) {
  function_t *call = &calls[hash];
  call->pc = pc;
  call->hash = hash;
  call->args.count = 0;
  call->cache_size = 0;
  call->cache_enabled = RINHA_CONFIG_CACHE_ENABLE;
  call->cache_checked = false;
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
_RINHA_CALL_ static void rinha_function_param_init_(function_t *call, rinha_value_t *value, int index) {
  // Set the parameter value in the function's stack context
  rinha_var_copy(&call->stack->mem[call->args.hash[index]].value , value);

  // Increment the count of items in the stack context
  ++call->stack->count;
}

inline static rinha_value_t *rinha_function_get_arg(function_t *call, int index) {
    return &call->stack->mem[call->args.hash[index]].value;
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
void rinha_error(const token_t *token, const char *fmt, ...) {

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
          "Pos:" " " TEXT_WHITE("%d") ", Stack: " TEXT_WHITE(
          "%"
       "d") " )\n\n",
      token->lexname, token->type, source_name, token->line, token->pos,
      rinha_sp);

  const char *code = source_code;
  const char *start = code;
  const char *end = code;

  for (register int i = 1; i < token->line; i++) {
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

  for (int i = 1; i < token->pos; i++) {
    putchar(' ');
  }
  puts("^"); // Putz...

  exit(EXIT_FAILURE);
}

bool rinha_test_is_comment(char **code_ptr, int *line_number, int *token_position) {

    if (**code_ptr == '/' && *(*code_ptr +1 ) == '/') {
        (*code_ptr) += 2;
        while (**code_ptr != '\0' && **code_ptr != '\n') {
            (*code_ptr)++;
        }
        return true;

     } else if ( **code_ptr == '/' &&  *(*code_ptr + 1) == '*') {
            (*code_ptr) += 2;
            while (**code_ptr != '\0') {
                if (**code_ptr == '*' && *(*code_ptr + 1) == '/') {
                    (*code_ptr) += 2;
                    return true;
                }
              if (**code_ptr == '\n') {
                (*line_number)++;
                (*token_position) = 0;
              }
              (*code_ptr)++;
              (*token_position)++;
            }
     }
    return false;
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
void rinha_tokenize_(char **code_ptr, int *rinha_tok_count) {
  int line_number = 1;
  int token_position = 0;
  int token_capacity = RINHA_CONFIG_TOKENS_SIZE;

  tokens = (token_t *)calloc(token_capacity, sizeof(token_t));

  if( !tokens )
    rinha_error(rinha_current_token_ctx, "Memory allocation failed");

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

    if (rinha_test_is_comment(code_ptr, &line_number, &token_position )) {
      continue;
    }

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

    if (rinha_test_is_comment(code_ptr, &line_number, &token_position )) {
      continue;
    }

    size_t tokenLength = *code_ptr - token;

    if (*rinha_tok_count >= token_capacity) {
        token_capacity *= 2;
        tokens = (token_t *)realloc(tokens, token_capacity * sizeof(token_t));

        if( !tokens )
          rinha_error(rinha_current_token_ctx, "Memory allocation failed");
    }

    strncpy(tokens[*rinha_tok_count].lexname, token, tokenLength);
    tokens[*rinha_tok_count].lexname[tokenLength] = '\0';

    if (type == TOKEN_STRING) {
      tokens[*rinha_tok_count].type = type;
      tokens[*rinha_tok_count].value =
          rinha_value_string_set_(tokens[*rinha_tok_count].lexname);
      (*code_ptr)++;
      token_position++;
    } else {
      tokens[*rinha_tok_count].type =
          rinha_discover_token_typeype_(tokens[*rinha_tok_count].lexname);
    }

    tokens[*rinha_tok_count].line = current_line;
    tokens[*rinha_tok_count].pos = current_position;

    (*rinha_tok_count)++;

    if (tokens[*rinha_tok_count-1].type == TOKEN_IDENTIFIER) {
      tokens[*rinha_tok_count-1].hash =
        rinha_create_sym_ref(tokens[*rinha_tok_count-1].lexname);
    }
  }
/*
  for(int i=0; i < *rinha_tok_count; i++) {
    //printf("\n TOKEN [%s] \n", tokens[i].lexname );
    //BREAK
  }

  printf("\n TOKENS [%d] \n", *rinha_tok_count);
  */
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
  //WARNING TO NAVIGATORS, IF YOU NEED TO DEBUG, START HERE
  //Uncomment the line below.
  //DEBUG BREAK
  ++rinha_current_token_ctx; // = tokens[++rinha_pc];

}

/**
 * @brief Move the current token back to the previous token in the token stream.
 */
void rinha_token_previous() {
  --rinha_current_token_ctx; // = tokens[--rinha_pc];
}

/**
 * @brief Get a pointer to the next token in the token stream.
 *
 * @return A pointer to the next token.
 */
token_t *rinha_next_token(void) {
  return (rinha_current_token_ctx+1); //&tokens[rinha_pc+1];
}

token_t *rinha_prev_token(void) {
  return (rinha_current_token_ctx-1); //&tokens[rinha_pc-1];
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
  if (rinha_current_token_ctx->type == expected_type) {
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
  while (rinha_current_token_ctx->type != TOKEN_EOF) {
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

#define rinha_check_call(call) (cache_enabled && call && !call->cache_checked && call->cache_enabled )

/**
 * @brief Checks the availability of cache optimization for the function.
 *
 * This function examines the current token context and determines whether
 * cache optimization can be used for the given function call.
 *
 * @param call Pointer to the function call.
 */
inline static bool rinha_check_cache_availability(function_t *call) {

  if (call->args.count > 3 || call->args.count == 0 ) {
    return false;
  }

  switch (rinha_current_token_ctx->type) {
    case TOKEN_PRINT:
      return false;
      break;
    case TOKEN_IDENTIFIER:

      // If writing to a global variable disables the cache as well
      token_t *nt = rinha_next_token();
      token_t *pt = rinha_prev_token();

      if (pt->type == TOKEN_LET) {
        call->vars++;
      }

      if (call->hash != rinha_current_token_ctx->hash) {
        rinha_value_t *v = rinha_var_get_(stack_ctx, rinha_current_token_ctx->hash);
        if (v && v->type == FUNCTION)  {
          function_t *caller = (function_t *) v->function;
          if (caller != NULL && !caller->cache_enabled) {
              return false;
          }
        }
      }

      if (pt->type != TOKEN_LET && nt->type == TOKEN_ASSIGN) {
        return false;
      }

      break;
  }

  return true;
}

inline static void rinha_expression_jump(function_t *call) {
  int open_paren = 0;
  while ( rinha_current_token_ctx->type != TOKEN_SEMICOLON
        && rinha_current_token_ctx->type != TOKEN_EOF) {

    if (rinha_current_token_ctx->type == TOKEN_LPAREN) {
      open_paren++;
    } else if ( rinha_current_token_ctx->type == TOKEN_RPAREN ) {
     if(!open_paren) {
         break;
     }
      open_paren--;
    }

    if ( !open_paren && rinha_current_token_ctx->type == TOKEN_COMMA) {
       break;
    }

    if (rinha_check_call(call)) {
      call->cache_enabled = rinha_check_cache_availability(call);
    }
    rinha_token_advance();
  }
  if(call)
    call->cache_checked = true;
}

/**
 * @brief Jump over a Rinha code block.
 *
 * This function skips over a Rinha code block enclosed in curly braces.
 */
inline static void rinha_block_jump_(function_t *call) {
  int open_braces = 1;

  //Gambiarra sintantica...
  if (rinha_current_token_ctx->type != TOKEN_LBRACE) {
    rinha_expression_jump(call);
    return;
  }

  rinha_token_consume_(TOKEN_LBRACE);

  while (open_braces && rinha_current_token_ctx->type != TOKEN_EOF) {

    if (rinha_check_call(call)) {
      call->cache_enabled = rinha_check_cache_availability(call);
    }

    rinha_token_advance();
    switch (rinha_current_token_ctx->type) {
      case TOKEN_LBRACE:
        open_braces++;
        break;
      case TOKEN_RBRACE:
        open_braces--;
        break;
    }
  }

  if(call)
    call->cache_checked = true;

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
function_t *rinha_prepare_closure(rinha_value_t *ret, int hash) {

  function_t *call = rinha_function_set_(rinha_current_token_ctx, hash);

  token_t *token_ctx = rinha_current_token_ctx;

  rinha_token_consume_(TOKEN_FN);
  rinha_token_consume_(TOKEN_LPAREN);

  call->vars = 0;

  while (rinha_current_token_ctx->type != TOKEN_RPAREN) {

    if (rinha_current_token_ctx->type == TOKEN_IDENTIFIER) {
      rinha_call_parameter_add(call, rinha_current_token_ctx->hash);
    }
    rinha_token_advance();
    call->vars++;
  }

  call->parent = stack_ctx;
  rinha_value_caller_set_(ret, call);
  rinha_var_set_(stack_ctx, ret, hash);

  if (rinha_sp > 0) {
    for (register int i = 0; i < RINHA_CONFIG_SYMBOLS_SIZE; ++i) {
      if (stack_ctx->mem[i].value.type != UNDEFINED ) {

         rinha_var_copy(&call->env[i], &stack_ctx->mem[i].value);
         call->vars++;
      }
    }
  }

  rinha_token_consume_(TOKEN_RPAREN);
  rinha_token_consume_(TOKEN_ARROW);
  call->pc = rinha_current_token_ctx; // rinha_pc;
  rinha_block_jump_(call);

  if (rinha_current_token_ctx->type == TOKEN_RPAREN) {
    token_t *next = rinha_next_token();
    if (next->type == TOKEN_LPAREN) {
      rinha_token_advance();
      int index = 0;
      while (rinha_current_token_ctx->type != TOKEN_RPAREN) {
         rinha_token_advance();
         rinha_exec_expression_(ret);
         rinha_var_copy(&call->args.values[index++], ret);
      }
      token_t *end = rinha_current_token_ctx;
      rinha_current_token_ctx = token_ctx+1;
      rinha_call_function_(call, ret);
      rinha_current_token_ctx = end;
      rinha_token_advance();
      return NULL;
    }
  }

  return call;
}

/**
 * @brief Parse a Rinha statement and update the result value.
 *
 * This function parses a Rinha statement based on the current token and updates
 * the result value pointed to by 'ret' accordingly.
 *
 * @param[in,out] ret  A pointer to the result value (updated during parsing).
 */
inline void rinha_exec_statement_(rinha_value_t *ret) {

  switch (rinha_current_token_ctx->type) {
  case TOKEN_LET: {
    rinha_token_consume_(TOKEN_LET);
    int hash = rinha_current_token_ctx->hash;
    int type = rinha_current_token_ctx->type;

    rinha_exec_identifier();
    rinha_token_consume_(TOKEN_ASSIGN);

    if (type == TOKEN_WILDCARD) {
      return; // that's ok, next instruction
    }
    if (rinha_current_token_ctx->type == TOKEN_FN) {
      rinha_prepare_closure(ret, hash);
      return;
    }
    rinha_exec_expression_(ret);
    rinha_var_set_(stack_ctx, ret, hash);
  } break;
  case TOKEN_FN:
    rinha_prepare_closure(ret, rinha_current_token_ctx->hash);
    break;
  case TOKEN_PRINT:
    rinha_print_statement_(ret);
    break;
  case TOKEN_RBRACE:
    rinha_token_advance();
    break;
  case TOKEN_LPAREN:
    rinha_token_advance();
    rinha_exec_expression_(ret);
    if (rinha_current_token_ctx->type == TOKEN_COMMA) {
      rinha_token_advance();
      rinha_value_t second = {0};
      rinha_exec_expression_(&second);
      *ret = rinha_value_tuple_set_(ret, &second);
      rinha_token_advance();
    }
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
  case TOKEN_LBRACE:
    rinha_exec_block_(ret);
  case TOKEN_IDENTIFIER:
    rinha_exec_expression_(ret);
    break;
  case TOKEN_IF:
    rinha_exec_if_statement_(ret);
    break;
  case TOKEN_SECOND:
  case TOKEN_FIRST:
  case TOKEN_NUMBER:
  case TOKEN_STRING:
  case TOKEN_TRUE:
  case TOKEN_FALSE:
    rinha_exec_expression_(ret);
    break;
  default:
    rinha_error(rinha_current_token_ctx, "Unexpected token");
    break;
  }
}


static bool rinha_cmp_eq(rinha_value_t *left, rinha_value_t *right);
static bool rinha_cmp_neq(rinha_value_t *left, rinha_value_t *right);

inline static bool rinha_cmp_tuple_eq(tuple_t *left, tuple_t *right)
{
  return rinha_cmp_eq((rinha_value_t *) &left->first,
          (rinha_value_t *) &right->first) &&
    rinha_cmp_eq((rinha_value_t *) &left->second,
           (rinha_value_t *)&right->second);
}

inline static bool rinha_cmp_tuple_neq(tuple_t *left, tuple_t *right)
{
  return rinha_cmp_neq((rinha_value_t *) &left->first,
          (rinha_value_t *) &right->first) ||
    rinha_cmp_neq((rinha_value_t *) &left->second,
          (rinha_value_t *)&right->second);
}

inline static bool rinha_cmp_eq(rinha_value_t *left, rinha_value_t *right) {

  if (left->type != right->type) {
    rinha_token_previous();
    rinha_error(rinha_current_token_ctx, "Comparison of different types");
  }

  switch(left->type) {
    case INTEGER:
      return (left->number == right->number);
    case STRING:
      return (strcmp(left->string, right->string) == 0);
    case TUPLE:
      return rinha_cmp_tuple_eq(&left->tuple, &right->tuple);
    default:
       return left->boolean == right->boolean;
  }
}

inline static bool rinha_cmp_neq(rinha_value_t *left, rinha_value_t *right) {

  if (left->type != right->type) {
    rinha_token_previous();
    rinha_error(rinha_current_token_ctx, "Comparison of different types");
  }

  switch(left->type) {
    case INTEGER:
      return (left->number != right->number);
    case STRING:
      return (strcmp(left->string, right->string) != 0);
    case TUPLE:
      return rinha_cmp_tuple_neq(&left->tuple, &right->tuple);
    default:
       return left->boolean != right->boolean;
  }
}


inline static void rinha_exec_comparison_(rinha_value_t *ret) {

  rinha_value_t left = {0};
  rinha_exec_calc_(&left);

  while (rinha_current_token_ctx->type == TOKEN_EQ ||
         rinha_current_token_ctx->type == TOKEN_GTE ||
         rinha_current_token_ctx->type == TOKEN_LTE ||
         rinha_current_token_ctx->type == TOKEN_GT ||
         rinha_current_token_ctx->type == TOKEN_NEQ ||
         rinha_current_token_ctx->type == TOKEN_LT) {

    token_type op_type = rinha_current_token_ctx->type;

    rinha_token_advance();

    rinha_value_t right = {0};
    rinha_exec_calc_(&right);

    switch(op_type) {
      case TOKEN_EQ:
        left.boolean = rinha_cmp_eq(&left, &right);
      break;
      case TOKEN_GTE:
        left.boolean = (left.number >= right.number);
        break;
      case TOKEN_LTE:
        left.boolean = (left.number <= right.number);
        break;
      case TOKEN_LT:
        left.boolean = (left.number < right.number);
        break;
      case TOKEN_GT:
        left.boolean = (left.number > right.number);
        break;
      case TOKEN_NEQ:
        left.boolean = rinha_cmp_neq(&left, &right);
        break;
    }
    left.type = BOOLEAN;
  }
  rinha_var_copy(ret, &left);
}

/**
 * @brief Parse a logical AND expression.
 *
 * This function parses a logical AND expression and returns the result.
 *
 * @param[in,out] left  A pointer to the left operand (updated during parsing).
 * @return The result of the logical AND expression.
 */
inline static void rinha_exec_logical_and_(rinha_value_t *ret) {

  rinha_value_t left = {0};
  rinha_exec_comparison_(&left);

  while (rinha_current_token_ctx->type == TOKEN_AND) {
    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_exec_comparison_(&right);
    left.boolean = left.boolean && right.boolean;
    left.type = BOOLEAN;
  }
  rinha_var_copy(ret, &left);
}

/**
 * @brief Parse a logical OR expression.
 *
 * This function parses a logical OR expression and returns the result.
 *
 * @param[in,out] left  A pointer to the left operand (updated during parsing).
 * @return The result of the logical OR expression.
 */
inline static void rinha_exec_logical_or_(rinha_value_t *ret) {
  rinha_value_t left = {0};
  rinha_exec_logical_and_(&left);

  while (rinha_current_token_ctx->type == TOKEN_OR) {
    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_exec_logical_and_(&right);
    left.boolean = (left.boolean || right.boolean);
    left.type = BOOLEAN;
  }
  rinha_var_copy(ret, &left);
}


inline void rinha_var_copy(rinha_value_t *var1, rinha_value_t *var2) {

  var1->type = var2->type;
  switch(var2->type)
  {
    case INTEGER:
      var1->number = var2->number;
      break;
    case BOOLEAN:
      var1->boolean = var2->boolean;
      break;
    case STRING:
      var1->string = rinha_alloc_static_string();
      strcpy(var1->string, var2->string);
      break;
    case FUNCTION:
      var1->function = var2->function;
      break;
    case TUPLE:
      rinha_var_copy( (rinha_value_t *) &var1->tuple.first,
          (rinha_value_t *) &var2->tuple.first );
      rinha_var_copy( (rinha_value_t *) &var1->tuple.second,
          (rinha_value_t *) &var2->tuple.second );
      break;
  }
}

/**
 * @brief Parse an assignment expression.
 *
 * This function parses an assignment expression and returns the result.
 *
 * @param[in,out] left  A pointer to the left operand (updated during parsing).
 * @return The result of the assignment expression.
 */
inline void rinha_exec_assign(rinha_value_t *left) {
  int hash = rinha_current_token_ctx->hash;
  rinha_exec_logical_or_(left);

  if (rinha_current_token_ctx->type == TOKEN_ASSIGN) {
    rinha_value_t *var = rinha_var_get_(stack_ctx, hash);
    rinha_token_advance();
    rinha_exec_assign(left);
    rinha_var_copy(var, left);
  }
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
inline static void rinha_exec_primary_(rinha_value_t *ret) {

  switch (rinha_current_token_ctx->type) {
  case TOKEN_IDENTIFIER:
    {
      int hash = rinha_current_token_ctx->hash;
      rinha_value_t *v = rinha_var_get_(stack_ctx, rinha_current_token_ctx->hash);
      if (v && v->type != UNDEFINED) {

        rinha_token_advance();

        if(v->type == FUNCTION) {
           rinha_call_function_((function_t *) v->function , ret);
           return;
        }
        rinha_var_copy(ret, v);
        return;
      }
      rinha_error(rinha_current_token_ctx, "Undefined symbol (Hash: %d) ",
          rinha_current_token_ctx->hash );
    }
    break;
  case TOKEN_FN:
    rinha_prepare_closure(ret, rinha_current_token_ctx->hash);
    break;
  case TOKEN_NUMBER:
    rinha_current_token_ctx->value = rinha_value_number_set_(atol(rinha_current_token_ctx->lexname));
    rinha_var_copy(ret, &rinha_current_token_ctx->value);
    rinha_token_advance();
    break;
  case TOKEN_STRING:
    rinha_value_t string = rinha_value_string_set_(
       rinha_current_token_ctx->lexname);
    rinha_var_copy(ret, &string);
    rinha_token_advance();
    break;
  case TOKEN_LPAREN:
    rinha_token_advance();
    //Weird, but this is to support things like: (let a = 2; a) + (let b = 3; b)
    if (rinha_current_token_ctx->type == TOKEN_LET) {
      rinha_exec_statement_(ret);

      switch (rinha_current_token_ctx->type) {
        case TOKEN_SEMICOLON:
          rinha_token_advance();
          rinha_exec_expression_(ret);
          break;
        case TOKEN_RPAREN:
          rinha_token_advance();
          break;
      }
    } else {
      rinha_exec_expression_(ret);
    }
    if (rinha_current_token_ctx->type == TOKEN_COMMA) {
      rinha_token_consume_(TOKEN_COMMA);

      rinha_value_t second = {0};
      rinha_exec_expression_(&second);
      *ret = rinha_value_tuple_set_(ret, &second);
    }
    rinha_token_advance();
    break;
  case TOKEN_TRUE:
    rinha_var_copy(ret, &rinha_current_token_ctx->value);
    rinha_token_advance();
    break;
  case TOKEN_FALSE:
    rinha_var_copy(ret, &rinha_current_token_ctx->value);
    rinha_token_advance();
    break;
  case TOKEN_FIRST:
    rinha_exec_first(ret);
    break;
  case TOKEN_SECOND:
    rinha_exec_second(ret);
    break;
  case TOKEN_LET:
    rinha_exec_statement_(ret);
      switch (rinha_current_token_ctx->type) {
        case TOKEN_SEMICOLON:
          rinha_token_advance();
          rinha_exec_expression_(ret);
          break;
        case TOKEN_RPAREN:
          rinha_token_advance();
          break;
      }
    rinha_token_advance();
    break;
  case TOKEN_PRINT:
    rinha_print_statement_(ret);
    rinha_token_advance();
    break;
  case TOKEN_IF:
    rinha_exec_if_statement_(ret);
    break;
  case TOKEN_RBRACE:
  case TOKEN_SEMICOLON:
  case TOKEN_RPAREN:
    rinha_token_advance();
    break;
  case TOKEN_EOF:
    break;
  default:
    rinha_error(rinha_current_token_ctx, "Token undefined");
  }
}

_RINHA_CALL_ void rinha_exec_term_(rinha_value_t *left) {
  rinha_exec_primary_(left);

  while (rinha_current_token_ctx->type == TOKEN_MULTIPLY ||
         rinha_current_token_ctx->type == TOKEN_DIVIDE ||
         rinha_current_token_ctx->type == TOKEN_MOD) {

    token_type op_type = rinha_current_token_ctx->type;

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

  // Concatenate an integer and a string
  if (left->type == INTEGER && right->type == STRING) {
    sprintf(tmp, "%d%s", left->number, right->string);
    left->string = rinha_alloc_static_string();

  // Concatenate a string and an integer
  } else if (left->type == STRING && right->type == INTEGER) {

    sprintf(tmp, "%s%d", left->string, right->number);

  // Concatenate a string and a boolean
  } else if (left->type == STRING && right->type == BOOLEAN) {

    sprintf(tmp, "%s%d", left->string, BOOL_NAME(right->boolean));

  // Concatenate a boolean and a string
  } else if (left->type == BOOLEAN && right->type == STRING) {

    sprintf(tmp, "%d%s", BOOL_NAME(left->boolean), right->string);
    left->string = rinha_alloc_static_string();

  // Concatenate two strings or unsupported types
  } else {
    sprintf(tmp, "%s%s", left->string, right->string);
    left->string = rinha_alloc_static_string();
  }

  strncpy(left->string, tmp, RINHA_CONFIG_STRING_VALUE_SIZE);
  left->type = STRING;
}

void rinha_exec_calc_(rinha_value_t *left) {
  rinha_exec_term_(left);

  while (rinha_current_token_ctx->type == TOKEN_PLUS ||
         rinha_current_token_ctx->type == TOKEN_MINUS) {

    token_type op_type = rinha_current_token_ctx->type;
    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_exec_term_(&right);

    if (op_type == TOKEN_PLUS &&
        (left->type != INTEGER || right.type != INTEGER) ) {
      rinha_value_concat_(left, &right);
      continue;
    }

    if (op_type == TOKEN_PLUS) {
      left->number += (RINHA_WORD) right.number;

    } else {
      left->number -= (RINHA_WORD) right.number;
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
inline static bool rinha_call_memo_cache_get_(function_t *call, rinha_value_t *ret, int hash) {
#if RINHA_CONFIG_CACHE_ENABLE == true

  if (!cache_enabled || !call->cache_enabled) {
    return false;
  }

  cache_t *cache = &call->cache[hash];

  if (!cache->cached)
    return false;

  rinha_value_t *arg0 = rinha_function_get_arg(call, 0);

  if (arg0->type != UNDEFINED && arg0->type != INTEGER) {
    call->cache_enabled = false;
    return false;
  }

  rinha_value_t *arg1 = rinha_function_get_arg(call, 1);

  if (arg1->type != UNDEFINED && arg1->type != INTEGER) {
    call->cache_enabled = false;
    return false;
  }

  rinha_value_t *arg2 = rinha_function_get_arg(call, 2);

  if (arg2->type != UNDEFINED && arg2->type != INTEGER) {
    call->cache_enabled = false;
    return false;
  }

  if (cache->input0.number != arg0->number ||
      cache->input1.number != arg1->number ||
      cache->input2.number != arg2->number) {
    return false;
  }

  rinha_var_copy(ret, &cache->value);

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
inline static void rinha_call_memo_cache_set_(function_t *call, rinha_value_t *value, int hash) {
#if RINHA_CONFIG_CACHE_ENABLE == true
  if (!cache_enabled || !call->cache_enabled) {
    return;
  }

  cache_t *cache = &call->cache[hash];

  // Check for collisions; if a value is already cached, do nothing
  if (cache->cached) {
    return;
  }

  if (++call->cache_size >= RINHA_CONFIG_CACHE_SIZE) {
    return;
  }

  rinha_value_t *arg0 = rinha_function_get_arg(call, 0);
  rinha_value_t *arg1 = rinha_function_get_arg(call, 1);
  rinha_value_t *arg2 = rinha_function_get_arg(call, 2);

  rinha_var_copy(&cache->input0, arg0);
  rinha_var_copy(&cache->input1, arg1);
  rinha_var_copy(&cache->input2, arg2);
  rinha_var_copy(&cache->value, value);
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
inline void rinha_exec_expression_(rinha_value_t *ret) {
  rinha_exec_assign(ret);
}


inline static void rinha_exec_function_(function_t *call, rinha_value_t *ret, rinha_value_t *args) {

  if ( rinha_sp == 0 ) {
    cache_enabled = RINHA_CONFIG_CACHE_ENABLE;
  }

  stack_ctx = &stacks[rinha_sp];
  call->stack = &stacks[++rinha_sp];

  for (register int i = 0; i < RINHA_CONFIG_SYMBOLS_SIZE; ++i) {
    if (call->env[i].type != UNDEFINED ) {
      rinha_var_copy(&call->stack->mem[i].value, &call->env[i]);
    }
  }

  for (register int i = 0; i < call->args.count; ++i) {
    rinha_function_param_init_(call, (rinha_value_t *) &args[i], i);
  }
  unsigned int hash = 0;

  if (cache_enabled && call->cache_enabled)
    hash = rinha_hash_stack_(call);

  token_t *current_pc = rinha_current_token_ctx;

  // Function not memoized; execute the function's block
  if (!rinha_call_memo_cache_get_(call, ret, hash)) {
    // TODO: Refactor these context flags
    stack_ctx = call->stack;
    rinha_current_token_ctx = call->pc;
    rinha_exec_block_(ret);
    rinha_call_memo_cache_set_(call, ret, hash);
  }

  --rinha_sp;
  call->stack->count = 0;
  stack_ctx = call->stack = &stacks[rinha_sp];
  rinha_current_token_ctx = current_pc;
  rinha_token_advance();
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
inline static void rinha_call_function_(function_t *call, rinha_value_t *ret)
{
  if (rinha_current_token_ctx->type != TOKEN_LPAREN) {
    rinha_value_caller_set_(ret, call);
    return;
  }

  if (rinha_sp+1 >= RINHA_CONFIG_STACK_SIZE) {
    rinha_error(rinha_current_token_ctx, "Stack overflow!");
  }

  rinha_token_consume_(TOKEN_LPAREN);

  rinha_value_t args[RINHA_CONFIG_FUNCTION_ARGS_SIZE];

  // Parse function arguments
  for (register int i = 0; i < call->args.count; ++i) {

    if (call->args.values[i].type != UNDEFINED) {
        rinha_var_copy((rinha_value_t *) &args[i], &call->args.values[i]);
    } else {
      rinha_exec_expression_((rinha_value_t *) &args[i]);
      if (rinha_current_token_ctx->type == TOKEN_COMMA) {
        rinha_token_advance();
      }
    }
  }

  rinha_exec_function_(call, ret, args);
}

/**
 * @brief Parse a Rinha block of code.
 *
 * This function parses a block of Rinha code enclosed in curly braces, executing
 * each statement within the block.
 *
 * @param[out] ret  A pointer to store the return value (updated during execution).
 */
inline void rinha_exec_block_(rinha_value_t *ret) {

  //Gambiarra sintantica...
  if (rinha_current_token_ctx->type != TOKEN_LBRACE) {
    rinha_exec_statement_(ret);
    return;
  }
  rinha_token_consume_(TOKEN_LBRACE);

  while (rinha_current_token_ctx->type != TOKEN_RBRACE) {
    rinha_exec_statement_(ret);
  }
  rinha_token_consume_(TOKEN_RBRACE);
}

inline void rinha_exec_if_statement_(rinha_value_t *ret) {
  rinha_token_consume_(TOKEN_IF);
  rinha_token_consume_(TOKEN_LPAREN);
  rinha_exec_logical_or_(ret);
  rinha_token_consume_(TOKEN_RPAREN);

  if (ret->boolean) {

    rinha_exec_block_(ret);
    token_t *tmp_pc = rinha_current_token_ctx;

    if (rinha_current_token_ctx->jmp_pc1) {
      rinha_current_token_ctx = rinha_current_token_ctx->jmp_pc1;
      rinha_token_advance();
      return;
    }
    if (rinha_current_token_ctx->type == TOKEN_ELSE) {
      rinha_token_consume_(TOKEN_ELSE);
      rinha_block_jump_(NULL);
    }
    rinha_current_token_ctx->jmp_pc1 = rinha_current_token_ctx;
  } else {
    int tmp_pc = rinha_pc;

    if (!rinha_current_token_ctx->jmp_pc2) {
      rinha_block_jump_(NULL);
      rinha_current_token_ctx->jmp_pc2 = rinha_current_token_ctx;
    } else {
      rinha_current_token_ctx = rinha_current_token_ctx->jmp_pc2;
      rinha_token_advance();
    }

    if (rinha_current_token_ctx->type == TOKEN_ELSE) {
      rinha_token_consume_(TOKEN_ELSE);
      rinha_exec_block_(ret);
    }
  }
}

void rinha_exec_identifier(void) {
  if (rinha_current_token_ctx->type == TOKEN_IDENTIFIER ||
      rinha_current_token_ctx->type == TOKEN_WILDCARD) {
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

  char *dialog = rinha_current_token_ctx->lexname;
  rinha_value_t v;
  v.type = STRING;
  int w = sizeof(woc) / sizeof(woc[0]);
  int l = strlen(dialog);
  int i = 1;

  v.string = rinha_alloc_static_string();

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

  tokens = NULL;
  //memset(tokens, 0, sizeof(tokens));
  memset(calls, 0, sizeof(calls));
  memset(tmp, 0, sizeof(tmp));
  memset(string_pool, 0, sizeof(string_pool));
}

inline static void rinha_clear_context(void) {
  stack_ctx       = NULL;
  rinha_sp        = 0;
  rinha_pc        = 0;
  rinha_tok_count = 0;
  on_tests        = false;
  symref          = 0;
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

    stacks = calloc(RINHA_CONFIG_STACK_SIZE, sizeof(stack_t));

    if (stacks == NULL) {
      fprintf(stderr, "Memory allocation failed (Size:%d Mb)",
      (( RINHA_CONFIG_STACK_SIZE * sizeof(stack_t) ) / 1024) / 1024 );
       return false;
    }

    rinha_clear_context();

    strcpy(source_name, name);
    on_tests = test;

    stack_ctx = stacks;
    char *code_ptr = source_code = script;

    while (*code_ptr != '\0') {
      rinha_tokenize_(&code_ptr, &rinha_tok_count);
    }

    tokens[rinha_tok_count++].type = TOKEN_EOF;

    // Initialize current token
    rinha_current_token_ctx = tokens;
    rinha_value_t ret = {0};

    rinha_exec_program_(&ret);
    *response = ret;

    free(stacks); stacks = NULL;

    return true;
}

