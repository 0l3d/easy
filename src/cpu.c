#include "easy.h"
#include <stdint.h>
#include <unistd.h>

const char     *help = "Usage: easy [-c] [-i] [-h]\n"
"  -h              Show this help\n"
"  -c              Compile easy64 assembly\n"
"  -i              Interpret easy64 binary\n";

int 
main(int argc, char **argv)
{
	char           *output_name = "a.out";
	int 		opt;
	while ((opt = getopt(argc, argv, "hc:i:")) != -1) {
		switch (opt) {
		case 'h':
			printf("%s", help);
			break;
		case 'c':
			parser(optarg, output_name);
			break;
		case 'i':
			interpret_easy64(output_name);
			break;
		}
	}
	return 0;
}
