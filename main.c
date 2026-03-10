#include "compiler.h"

int main(int argc, char **argv)
{
	if (argc < 2) {
		fprintf(stderr, "file expected\n");
		return EXIT_FAILURE;
	}
	
	Boot(argc, argv);
	return 0;
}