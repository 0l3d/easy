#include "easy.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const char *help =
    "Usage: easy [-c] [-i] [-h]\n"
    "  -h              Show this help\n"
    "  -c              Compile easy64 assembly\n"
    "  -i              Interpret easy64 binary\n"
    "  -o 			   Specify output name for compile\n";

int main(int argc, char **argv) {
  char *output_name = NULL;
  char *compile_file = NULL;
  char *arguments = NULL;
  char *binfile = NULL;
  int opt;
  int compile = 0;
  int interpret = 0;
  while ((opt = getopt(argc, argv, "hc:i:o:a:")) != -1) {
    switch (opt) {
    case 'h':
      printf("%s", help);
      break;
    case 'c':
      compile = 1;
      compile_file = strdup(optarg);
      break;
    case 'i':
      interpret = 1;
      binfile = strdup(optarg);
      break;
    case 'a':
      arguments = strdup(optarg);
      break;
    case 'o':
      output_name = strdup(optarg);
      break;
    default:
      break;
    }
  }

  if (interpret == 1 && compile == 0) {
    interpret_easy64(binfile, arguments);
    free(binfile);
    if (arguments != NULL) {
      free(arguments);
    }
  }

  if (compile == 1 && interpret == 0) {
    if (output_name == NULL) {
      output_name = strdup("a.out");
    }
    if (compile_file == NULL) {
      printf("Need an argument -i: Undefined file.");
      return 1;
    }
    parser(compile_file, output_name);
    free(compile_file);
    free(output_name);
  }
  return 0;
}
