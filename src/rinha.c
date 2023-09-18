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



static bool on_tests = false;

static char source_name[128];


static char *source_code = NULL;

static char tmp[RINHA_CONFIG_STRING_VALUE_MAX] = {0};

static stack_t global;

static stack_t st2[16544];

static stack_t *stacks = st2;

static stack_t *stack_ctx;

static int calls_count = 0;

static int rinha_sp = 0;

static int rinha_tok_count = 0;

static int rinha_pc = 0;

static token_t rinha_current_token_ctx;

static token_t tokens[1000] = {0};

static function_t calls[RINHA_CONFIG_SYMBOLS_SIZE];

static function_t *current_function = NULL;


void rinha_print_(rinha_value_t *value, bool lf, bool debug) {

    if(on_tests)
        return;

    if (!value) {
    fprintf(stderr, "Error: rinha_value_t not defined\n");
    return;
  }

  char end_char = lf ? 0x0a : 0x00;

  switch (value->type) {
    case STRING:
      if (debug)
        fprintf(stdout, "\nSTRING (%d): ->", strlen(value->string));
      fprintf(stdout, "%s%c", value->string, end_char);
      break;
    case INTEGER:
      if (debug)
        fprintf(stdout, "\nINTEGER: ->");
      fprintf(stdout, "%lld%c", value->number, end_char);
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
      fprintf(stdout, "AS NUMBER  [%lld]\n", value->number);
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

_RINHA_CALL_ static rinha_value_t rinha_value_set_(rinha_value_t value) {
  switch (value.type) {
    case STRING:
      return rinha_value_string_set_(value.string);
    case BOOLEAN:
      return rinha_value_bool_set_(value.boolean);
    default:
      return rinha_value_number_set_(value.number);
  }
}

inline static unsigned long rinha_hash_str_(char *str) {
  unsigned long hash = 5381;
  int c;
  char *ptr = str;

  while ((c = *ptr++)) {
    hash = ((hash << 5) + hash) + c;
  }

  return (hash % RINHA_CONFIG_SYMBOLS_SIZE);
}

unsigned int rinha_hash_stack_(function_t *f) {
  unsigned int hash = 0;

  for (int i = 0; i < f->stack->count; i++) {
    rinha_value_t *v = &f->stack->mem[f->args.hash[i]].value;
    hash ^= (unsigned int)(v->type != STRING)
        ? v->number : rinha_hash_str_(v->string);
  }

  return (hash % RINHA_CONFIG_CACHE_SIZE);
}

void rinha_print_statement_(rinha_value_t *value) {
  function_t *call = NULL;
  rinha_token_consume_(TOKEN_PRINT);
  rinha_token_consume_(TOKEN_LPAREN);

  token_t *nt = rinha_next_token();

  if (nt->type == TOKEN_FN) {
       //rinha_token_advance();
       *value = rinha_value_string_set_("<#closure>");
  } else {
  //DEBUG
      rinha_parser_expression_(value);
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

_RINHA_CALL_ void rinha_var_set_(stack_t *ctx, rinha_value_t *value, int hash) {
  // Determine the variable type if not explicitly defined
  value->type = (!value->type) ? (isdigit(value->string[0])) ? INTEGER : STRING : value->type;

  // Set the variable in the stack context
  ctx->mem[hash].value = *value;
  ctx->count++;
}

_RINHA_CALL_ rinha_value_t *rinha_var_get_(stack_t *ctx, int hash) {
  rinha_value_t *v = &ctx->mem[hash].value;
  v->hash = hash;

  // If the variable is undefined in the current context, look in the global context
  if (v && v->type == UNDEFINED) {
    v = &stacks[0].mem[hash].value; // global
  }

  return v;
}

_RINHA_CALL_ function_t *rinha_function_set_(int pc, int hash) {
  function_t *call = &calls[hash];
  call->pc = pc;
  calls_count++;
  return call;
}

_RINHA_CALL_ function_t *rinha_function_get_(int hash) {
  function_t *f = &calls[hash];
  return (f && f->pc) ? f : NULL;
}

_RINHA_CALL_ void rinha_call_parameter_add(function_t *f, int hash) {
  f->args.hash[f->args.count++] = hash;
}

_RINHA_CALL_ static void rinha_function_param_init_(function_t *f, rinha_value_t *value, int index) {
  // Set the parameter value in the function's stack context
  f->stack->mem[f->args.hash[index]].value = *value;

  // Increment the count of items in the stack context
  f->stack->count++;
}

inline bool rinha_isdelim(char c) {
  return (strchr("()\"'{},+-*/%;", c) != NULL);
}

void rinha_error(const token_t token, const char *fmt, ...) {

  FILE *RINHA_OUTERR = stderr;

  fprintf(RINHA_OUTERR,
          TEXT_RED("\nError: "),
          token.line, token.pos);

  va_list args;
  va_start(args, fmt);
  vfprintf(RINHA_OUTERR, fmt, args);
  va_end(args);

  fprintf(
      RINHA_OUTERR,
      " ( token_t: " TEXT_GREEN("%s") ", Type: " TEXT_WHITE(
          "%d") ", File: " TEXT_WHITE("%s") ", Line: " TEXT_WHITE("%d") ", "
          "Pos:" " " TEXT_WHITE("%d") ", stack_t: " TEXT_WHITE(
          "%"
       "d") " )\n\n",
      token.lexeme, token.type, source_name, token.line, token.pos,
      rinha_sp);

  const char *code = source_code;
  const char *start = code;
  const char *end = code;

  for (int i = 1; i < token.line; i++) {
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

void rinha_tokenize_(char **code_ptr, token_t *tokens, int *rinha_tok_count) {
  int line_number = 1;
  int token_position = 0;

  while (**code_ptr != '\0') {
    token_type type = UNDEFINED;

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

  for (int i = 1; token[i] != '\0'; i++) {
    if (!isalnum(token[i]) && token[i] != '_') {
      return 0;
    }
  }

  return 1;
}

void rinha_token_advance() {
  rinha_current_token_ctx = tokens[++rinha_pc];
  // DEBUG BREAK;
}

void rinha_token_previous() {
  rinha_current_token_ctx = tokens[--rinha_pc];
}

token_t *rinha_next_token(void) {
  return &tokens[rinha_pc];
}

void rinha_token_consume_(token_type expected_type) {
  if (rinha_current_token_ctx.type == expected_type) {
    rinha_token_advance();
  } else {
    rinha_error(rinha_current_token_ctx,
                "Consume token: Expected an identifier (%d) ", expected_type);
  }
}

void rinha_parse_program_(rinha_value_t *ret) {
  while (rinha_current_token_ctx.type != TOKEN_EOF) {
    rinha_parse_statement_(ret);
  }
}

void rinha_parse_first(rinha_value_t *ret) {
  rinha_token_consume_(TOKEN_FIRST);
  rinha_token_consume_(TOKEN_LPAREN);
  rinha_parser_expression_(ret);

  if (ret->type != TUPLE) {
    rinha_token_previous();
    rinha_error(rinha_current_token_ctx,
                "first: Invalid argument, expected a tuple ");
  }

  *ret = *((rinha_value_t *)&ret->tuple.first);
  rinha_token_consume_(TOKEN_RPAREN);
}

void rinha_parse_second(rinha_value_t *ret) {
  rinha_token_consume_(TOKEN_SECOND);
  rinha_token_consume_(TOKEN_LPAREN);
  rinha_parser_expression_(ret);

  if (ret->type != TUPLE) {
    rinha_token_previous();
    rinha_error(rinha_current_token_ctx,
                "second: Invalid argument, expected a tuple ");
  }

  *ret = *((rinha_value_t *)&ret->tuple.second);
  rinha_token_consume_(TOKEN_RPAREN);
}

void rinha_parse_closure(int hash) {

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
  rinha_token_consume_(TOKEN_RPAREN);
  rinha_token_consume_(TOKEN_ARROW);
  call->pc = rinha_pc;

  //TODO: Refactor this block
  int open_braces = 1;

  while (open_braces && rinha_current_token_ctx.type != TOKEN_EOF) {
    rinha_token_advance();
    if (rinha_current_token_ctx.type == TOKEN_LBRACE)
      open_braces++;
    else if (rinha_current_token_ctx.type == TOKEN_RBRACE)
      open_braces--;
  }
  rinha_token_consume_(TOKEN_RBRACE);
}

void rinha_parse_statement_(rinha_value_t *ret) {
  // DEBUG
  switch (rinha_current_token_ctx.type) {
  case TOKEN_LET: {
    rinha_token_consume_(TOKEN_LET);
    int hash = rinha_current_token_ctx.hash;
    int type = rinha_current_token_ctx.type;

    rinha_parser_identifier();

    rinha_token_consume_(TOKEN_ASSIGN);

    if (type == TOKEN_WILDCARD) {
      return; // that's ok, next instruction
    }

    if (rinha_current_token_ctx.type == TOKEN_FN) {
      rinha_parse_closure(hash);
      return;
    }

    rinha_parser_expression_(ret);
    rinha_var_set_(stack_ctx, ret, hash);
  } break;
  case TOKEN_FN:
    rinha_parse_closure(rinha_current_token_ctx.hash);
    break;
  case TOKEN_FIRST:
    rinha_parse_first(ret);
    break;
  case TOKEN_SECOND:
    rinha_parse_second(ret);
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
  case TOKEN_RPAREN:
    rinha_token_advance();
    break;
  case TOKEN_IDENTIFIER:
    rinha_parser_expression_(ret);
    break;
  case TOKEN_IF:
    rinha_parse_if_statement_(ret);
    break;
  case TOKEN_NUMBER:
    rinha_token_consume_(rinha_current_token_ctx.type);
    *ret = rinha_current_token_ctx.value;
    break;
  case TOKEN_EOF:
    break;
  default:
    rinha_error(rinha_current_token_ctx, "Unexpected token");
    break;
  }
}

bool rinha_parse_comparison_(rinha_value_t *left) {
  rinha_parse_calc_(left);

  while (rinha_current_token_ctx.type == TOKEN_EQ ||
         rinha_current_token_ctx.type == TOKEN_GTE ||
         rinha_current_token_ctx.type == TOKEN_LTE ||
         rinha_current_token_ctx.type == TOKEN_GT ||
         rinha_current_token_ctx.type == TOKEN_LT) {

    token_type operatorType = rinha_current_token_ctx.type;

    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_parse_calc_(&right);

    if (operatorType == TOKEN_EQ) {
      return (left->number == right.number);
    } else if (operatorType == TOKEN_GTE) {
      return (left->number >= right.number);
    } else if (operatorType == TOKEN_LTE) {
      return (left->number <= right.number);
    } else if (operatorType == TOKEN_LT) {
      return (left->number < right.number);
    } else if (operatorType == TOKEN_GT) {
      return (left->number > right.number);
    } else if (operatorType != TOKEN_NEQ) {
      return (left->number != right.number);
    }
  }

  return (left->number != 0);
}

bool rinha_parse_logical_and_(rinha_value_t *left) {
  bool ret = rinha_parse_comparison_(left);

  while (rinha_current_token_ctx.type == TOKEN_AND) {
    rinha_token_advance();
    rinha_value_t right = {0};
    bool cmp = rinha_parse_comparison_(&right);
    ret = (ret && cmp);
  }

  return ret;
}

bool rinha_parser_logical_or_(rinha_value_t *left) {
  bool l = rinha_parse_logical_and_(left); // Parse the left operand and initialize the result

  while (rinha_current_token_ctx.type == TOKEN_OR) {
    rinha_token_advance();
    rinha_value_t right = {0};
    bool cmp = rinha_parse_logical_and_(&right);
    l = (l || cmp);
  }

  return l;
}

bool rinha_parser_assign(rinha_value_t *left) {
  int hash = rinha_current_token_ctx.hash;
  bool l = rinha_parser_logical_or_(left);

  if (rinha_current_token_ctx.type == TOKEN_ASSIGN) {
    rinha_value_t *var = rinha_var_get_(stack_ctx, hash);
    rinha_token_advance();
    rinha_value_t right = {0};
    l = rinha_parser_assign(left);
    *var = *left;
  }

  return l;
}

// Atomic

void rinha_parse_primary_(rinha_value_t *ret) {
  switch (rinha_current_token_ctx.type) {
  case TOKEN_IDENTIFIER:

    rinha_value_t *v = rinha_var_get_(stack_ctx, rinha_current_token_ctx.hash);
    if (v && v->type != UNDEFINED) {
      rinha_token_advance();
      *ret = *v;

      return;
    }
    function_t *call = rinha_function_get_(rinha_current_token_ctx.hash);
    if (call) {
      rinha_function_exec_(call, ret);
      return;
    }

    rinha_error(rinha_current_token_ctx, "Undefined symbol");
    rinha_token_advance();
    break;
  case TOKEN_FN:
    int _pc = rinha_pc;
    int hash = rinha_current_token_ctx.hash;
    rinha_parse_closure(hash);
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
    rinha_parser_expression_(ret);

    if (rinha_current_token_ctx.type == TOKEN_COMMA) {
      rinha_token_consume_(TOKEN_COMMA);

      rinha_value_t second = {0};
      rinha_parser_expression_(&second);
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
    rinha_parse_first(ret);
    break;
  case TOKEN_SECOND:
    rinha_parse_second(ret);
    break;
  case TOKEN_PRINT:
    rinha_print_statement_(ret);
    rinha_token_advance();
    break;
  default:
    rinha_error(rinha_current_token_ctx, "Token undefined");
  }
}

_RINHA_CALL_ static void rinha_parse_term_(rinha_value_t *left) {
  rinha_parse_primary_(left);

  while (rinha_current_token_ctx.type == TOKEN_MULTIPLY ||
         rinha_current_token_ctx.type == TOKEN_DIVIDE ||
         rinha_current_token_ctx.type == TOKEN_MOD) {

    token_type operatorType = rinha_current_token_ctx.type;

    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_parse_primary_(&right);

    switch (operatorType) {
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

void rinha_parse_calc_(rinha_value_t *left) {
  rinha_parse_term_(left);
  while (rinha_current_token_ctx.type == TOKEN_PLUS ||
         rinha_current_token_ctx.type == TOKEN_MINUS) {

    token_type operatorType = rinha_current_token_ctx.type;
    rinha_token_advance();
    rinha_value_t right = {0};
    rinha_parse_term_(&right);

    if (operatorType == TOKEN_PLUS &&
        (left->type != INTEGER || right.type != INTEGER)) {
      rinha_value_concat_(left, &right);
      continue;
    }

    if (operatorType == TOKEN_PLUS) {
      left->number += right.number;
    } else {
      left->number -= right.number;
    }
    left->type = INTEGER;
  }
}


bool rinha_call_memo_cache_get_(function_t *call, rinha_value_t *ret, int hash) {
#if RINHA_CONFIG_CACHE_ENABLE == true
  cache_t *cache = &call->cache[hash];

  if (!cache->cached)
    return false;

  *ret = cache->value;

  return true;
#else
  return false;
#endif
}

void rinha_call_memo_cache_set_(function_t *call, rinha_value_t *ret, int hash) {
#if RINHA_CONFIG_CACHE_ENABLE == true
  cache_t *cache = &call->cache[hash];

  // Check for collisions; if a value is already cached, do nothing
  if (cache->cached)
    return;

  cache->value = *ret;
  cache->cached = true;
#endif
}

void rinha_parser_expression_(rinha_value_t *ret) {
  rinha_parser_assign(ret);
}

void rinha_function_exec_(function_t *call, rinha_value_t *ret) {
  stack_ctx = &stacks[rinha_sp];

  call->stack = &stacks[++rinha_sp];
  rinha_token_advance();
  rinha_token_consume_(TOKEN_LPAREN);

  // Parse function arguments
  for (int i = 0; rinha_current_token_ctx.type != TOKEN_RPAREN; ++i) {
    rinha_parser_expression_(ret);

    if (rinha_current_token_ctx.type == TOKEN_COMMA) {
      rinha_token_consume_(TOKEN_COMMA);
    }

    rinha_function_param_init_(call, ret, i);
  }

  unsigned int hash = rinha_hash_stack_(call);

  int current_pc = rinha_pc;

  if (!rinha_call_memo_cache_get_(call, ret, hash)) {
    // Function not memoized; execute the function's block
    // TODO: Refactor these context flags
    stack_ctx = call->stack;
    rinha_pc = call->pc - 1;
    rinha_token_advance();
    rinha_parse_block_(ret);
    rinha_call_memo_cache_set_(call, ret, hash);
  }

  // Restore the previous context
  // FIXME: Yes, I know... it's ugly, but one day I'll come back here to fix it
  rinha_sp--;
  stack_ctx = call->stack = &stacks[rinha_sp];
  call->stack->count = 0;
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
void rinha_parse_block_(rinha_value_t *ret) {
  rinha_token_consume_(TOKEN_LBRACE);

  while (rinha_current_token_ctx.type != TOKEN_RBRACE) {
    rinha_parse_statement_(ret);
  }

  rinha_token_consume_(TOKEN_RBRACE);
}

/**
 * @brief Jump over a Rinha code block.
 *
 * This function skips over a Rinha code block enclosed in curly braces.
 */
inline static void rinha_block_jump_(void) {
  int open_braces = 1;

  while (open_braces && rinha_current_token_ctx.type != TOKEN_EOF) {
    rinha_token_advance();

    if (rinha_current_token_ctx.type == TOKEN_LBRACE) {
      open_braces++;
    } else if (rinha_current_token_ctx.type == TOKEN_RBRACE) {
      open_braces--;
    }
  }
}


void rinha_parse_if_statement_(rinha_value_t *ret) {
  rinha_token_consume_(TOKEN_IF);
  rinha_token_consume_(TOKEN_LPAREN);

  bool is_true = rinha_parser_logical_or_(ret);

  rinha_token_consume_(TOKEN_RPAREN);

  if (is_true) {

    rinha_parse_block_(ret);

    int tmp_pc = rinha_pc;

    if (rinha_current_token_ctx.jmp_pc1) {
      rinha_pc = rinha_current_token_ctx.jmp_pc1;
      rinha_token_advance();
      return;
    }
    if (rinha_current_token_ctx.type == TOKEN_ELSE) {
      rinha_block_jump_();
    }
    tokens[tmp_pc].jmp_pc1 = (rinha_pc - 1);
  } else {
    int tmp_pc = rinha_pc;

    if (!rinha_current_token_ctx.jmp_pc2) {
      rinha_block_jump_();
      rinha_token_advance();
      tokens[tmp_pc].jmp_pc2 = (rinha_pc - 1);
    } else {
      rinha_pc = rinha_current_token_ctx.jmp_pc2;
      rinha_token_advance();
    }

    if (rinha_current_token_ctx.type == TOKEN_ELSE) {
      rinha_token_consume_(TOKEN_ELSE);
      rinha_parse_block_(ret);
    }
  }
}

void rinha_parser_identifier(void) {
  if (rinha_current_token_ctx.type == TOKEN_IDENTIFIER ||
      rinha_current_token_ctx.type == TOKEN_WILDCARD) {
    rinha_token_advance();
  } else {
    rinha_error(rinha_current_token_ctx, "Expected an identifier ");
  }
}

bool rinha_script_exec(char *name, char *script, rinha_value_t *response, bool test) {

    memset(tokens, 0, sizeof(tokens));
    memset(calls, 0, sizeof(calls));
    memset(stacks, 0, sizeof(*stacks));

    strcpy(source_name, name);

    calls_count = 0;
    rinha_sp = 0;
    rinha_pc = 0;
    rinha_tok_count = 0;

    on_tests = test;

    stack_ctx = stacks;

    char *code_ptr = source_code = script;
    int lines = 0;

    while (*code_ptr != '\0') {
        rinha_tokenize_(&code_ptr, tokens, &rinha_tok_count);
    }

    tokens[rinha_tok_count++].type = TOKEN_EOF;

    // Initialize current token
    rinha_current_token_ctx = tokens[0];
    rinha_value_t ret = {0};

    rinha_parse_program_(&ret);
    *response = ret;

    return true;
}

