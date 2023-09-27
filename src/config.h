/**
 * @file config.h
 *
 * @brief Rinha Configuration - Compiler Settings
 *
 * This header file contains configuration settings for the Rinha compiler.
 *
 * @details
 * - RINHA_VERSION: The current version of Rinha, set to "alpha: 0.00001".
 *
 * @author Josuel Teles
 * @date September 16, 2023
 */

#ifndef _RINHA_CONFIG_
#define _RINHA_CONFIG_

#include <stdint.h>
#include <sys/resource.h>

#define RINHA_VERSION "alpha: 0.00001"

#define WORD32 int32_t //Max 2.147.483.647
#define WORD64 int64_t //Max 9.223.372.036.854.775.807

#define RINHA_WORD WORD64

/**
 * @details
 * - RINHA_CONFIG_STRING_VALUE_MAX: Maximum length for string values in Rinha.
 */
#define RINHA_CONFIG_STRING_VALUE_SIZE 1024
#define RINHA_CONFIG_STRING_POOL_SIZE 32

/**
 * @details
 * - RINHA_CONFIG_SYMBOLS_SIZE: Size of the symbols table.
 */
#define RINHA_CONFIG_SYMBOLS_SIZE 32

/**
 * @details
 * - RINHA_CONFIG_STACK_SIZE: Size of the execution stack.
 */
#define RINHA_CONFIG_STACK_SIZE 1000100

/**
 * @details
 * - RINHA_CONFIG_CALLS_SIZE: Maximum number of function calls.
 */
#define RINHA_CONFIG_CALLS_SIZE 32

/**
 * @details
 * - RINHA_CONFIG_CACHE_ENABLE: Enables or disables the cache in Rinha, set to true (enabled).
 */
#define RINHA_CONFIG_CACHE_ENABLE true

/**
 * @details
 * - RINHA_CONFIG_CACHE_SIZE: Size of the cache when enabled.
 */
#define RINHA_CONFIG_CACHE_SIZE 4099

/**
 * @details
 * - RINHA_CONFIG_TOKENS_SIZE: Maximum number of tokens that can be stored in the token array
 */
#define RINHA_CONFIG_TOKENS_SIZE 1000

/**
 * @details
 * - RINHA_CONFIG_SYMBOL_NAME_SIZE: Maximum size of symbol (identifier) names in characters.
 */
#define RINHA_CONFIG_SYMBOL_NAME_SIZE 16

/**
 * @brief Maximum stack size limit (RLIMIT_STACK) for the Rinha program.
 *
 * @note Adjust this value as needed based on the memory requirements of your environment.
 */
//#define RINHA_CONFIG_RLIMIT_STACK (1024 * 1024 * 800) // 800Mb
#define RINHA_CONFIG_RLIMIT_STACK RLIM_INFINITY

///
#define RINHA_CONFIG_FUNCTION_ARGS_SIZE 6

#endif
