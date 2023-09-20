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

#define RINHA_VERSION "alpha: 0.00001"

/**
 * @details
 * - RINHA_CONFIG_STRING_VALUE_MAX: Maximum length for string values in Rinha.
 */
#define RINHA_CONFIG_STRING_VALUE_MAX 256

/**
 * @details
 * - RINHA_CONFIG_SYMBOLS_SIZE: Size of the symbols table.
 */
#define RINHA_CONFIG_SYMBOLS_SIZE 64

/**
 * @details
 * - RINHA_CONFIG_STACK_SIZE: Size of the execution stack.
 */
#define RINHA_CONFIG_STACK_SIZE 32000

/**
 * @details
 * - RINHA_CONFIG_CALLS_SIZE: Maximum number of function calls.
 */
#define RINHA_CONFIG_CALLS_SIZE 64

/**
 * @details
 * - RINHA_CONFIG_CACHE_ENABLE: Enables or disables the cache in Rinha, set to true (enabled).
 */
#define RINHA_CONFIG_CACHE_ENABLE true

/**
 * @details
 * - RINHA_CONFIG_CACHE_SIZE: Size of the cache when enabled.
 */
#define RINHA_CONFIG_CACHE_SIZE 4096

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
#define RINHA_CONFIG_RLIMIT_STACK 1024 //Mb

#endif
