#include "easy.h"
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

const char     *help = "Usage: easy [-c] [-i] [-h]\n"
"  -h              Show this help\n"
"  -c              Compile easy64 assembly\n"
"  -i              Interpret easy64 binary\n"
"  -o 			   Specify output name for compile\n";

int
main(int argc, char **argv)
{
	char           *output_name = NULL;
	char           *compile_file = NULL;
	int 		opt;
	int 		compile = 0;
	while ((opt = getopt(argc, argv, "hc:i:o:")) != -1) {
		switch (opt) {
		case 'h':
			printf("%s", help);
			break;
		case 'c':
			compile = 1;
			compile_file = strdup(optarg);
			break;
		case 'i':
			interpret_easy64(optarg);
			break;
		case 'o':
			output_name = strdup(optarg);
			break;
		}
	}

	if (compile == 1) {
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
