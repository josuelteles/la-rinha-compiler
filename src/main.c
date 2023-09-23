/**
 * @file main.c
 *
 * @brief Rinha Language Interpreter - Compiler Development Challenge
 *
 * For more information, please visit the documentation at:
 * [rinha-de-compiler](https://github.com/aripiprazole/rinha-de-compiler)
 *
 * @author Josuel Teles
 * @date September 16, 2023
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <stdarg.h>
#include <sys/resource.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>

#include "rinha.h"

char *rinha_load_file(const char *file) {
  struct stat s;

  if (stat(file, &s) == -1) {
    fprintf(stderr, "Error getting file info (file:%s, err: %s)",
      file, strerror(errno));
    return NULL;
  }

  FILE *fp = fopen(file, "r");
  if (!fp) {
    fprintf(stderr, "Error opening file (file:%s, err: %s)", file,
     strerror(errno));
    return NULL;
  }

  char *buffer = calloc(s.st_size + 1, sizeof(char));
  if (!buffer) {
    fprintf(stderr, "Memory allocation failed (file:%s)", file);
    fclose(fp);
    return NULL;
  }

  size_t r = fread(buffer, sizeof(char), s.st_size, fp);
  if (r != (size_t)s.st_size) {
    fprintf(stderr, "Error reading file (file:%s, err: %s)", file,
       strerror(errno));
    free(buffer);
    fclose(fp);
    return NULL;
  }

  buffer[s.st_size] = '\0';

  fclose(fp);
  return buffer;
}

#define _BYTES 1048576

void rinha_sysinfo(void) {

  struct sysinfo sys_info;

  if (sysinfo(&sys_info) != 0) {
      perror("sysinfo");
      return;
  }

  uint64_t total_ram =
    sys_info.totalram * sys_info.mem_unit / _BYTES;
  uint64_t free_ram =
    sys_info.freeram * sys_info.mem_unit / _BYTES;
  uint64_t available_ram =
    (sys_info.freeram + sys_info.bufferram) * sys_info.mem_unit / _BYTES;

  printf("\nTotal RAM: %llu Mb\n", total_ram);
  printf("Free RAM: %llu Mb\n", free_ram);
  printf("Available RAM: %llu Mb\n\n", available_ram);
}

bool rinha_stack_config(void) {
  struct rlimit rl;

  rl.rlim_cur = RINHA_CONFIG_RLIMIT_STACK;
  rl.rlim_max = RINHA_CONFIG_RLIMIT_STACK;

  if (setrlimit(RLIMIT_STACK, &rl) == -1) {
      fprintf(stderr, "Error configuring stack size limit (err: %s)",
              strerror(errno));
      return false;
  }

  return true;
}


void rinha_banner(void) {

  const char *banner =
      "\033[31m\n\n"
      " ██▓    ▄▄▄          ██▀███   ██▓ ███▄    █  ██░ ██  ▄▄▄        \n"
      "▓██▒   ▒████▄       ▓██ ▒ ██▒▓██▒ ██ ▀█   █ ▓██░ ██▒▒████▄      \n"
      "▒██░   ▒██  ▀█▄     ▓██ ░▄█ ▒▒██▒▓██  ▀█ ██▒▒██▀▀██░▒██  ▀█▄    \n"
      "▒██░   ░██▄▄▄▄██    ▒██▀▀█▄  ░██░▓██▒  ▐▌██▒░▓█ ░██ ░██▄▄▄▄██   \n"
      "░██████▒▓█   ▓██▒   ░██▓ ▒██▒░██░▒██░   ▓██░░▓█▒░██▓ ▓█   ▓██▒  \n"
      "░ ▒░▓  ░▒▒   ▓▒█░   ░ ▒▓ ░▒▓░░▓  ░ ▒░   ▒ ▒  ▒ ░░▒░▒ ▒▒   ▓▒█░  \n"
      "░ ░ ▒  ░ ▒   ▒▒ ░     ░▒ ░ ▒░ ▒ ░░ ░░   ░ ▒░ ▒ ░▒░ ░  ▒   ▒▒ ░  \n"
      " ░ ░    ░   ▒        ░░   ░  ▒ ░   ░   ░ ░  ░  ░░ ░  ░   ▒      \n"
      "   ░  ░     ░  ░      ░      ░           ░  ░  ░  ░      ░  ░   \n"
      "\n\033[0;37mVersion " RINHA_VERSION "\033[0m\n";

  fprintf( stdout, "\n%s\n\n", banner);

  rinha_sysinfo();
}

int usage(const char *prog) {
    rinha_banner();
    printf("Usage: %s <script_file>\n", prog);
    printf("  <script_file>: Path to the Rinha script file to execute.\n");
    printf("  Ex: %s /usr/src/source.rinha\n", prog);
    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {

  //TODO: Set options config here
  rinha_stack_config();
  //rinha_banner();

  if (argc < 2) {
      return usage(argv[0]);
  }

  char *code = rinha_load_file(argv[1]);

  if (!code)
      return EXIT_FAILURE;

  rinha_value_t response = {0};

  rinha_script_exec(argv[1], code, &response, false);

  return EXIT_SUCCESS;
}
