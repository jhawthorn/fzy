#include <stdlib.h>
#include <stdio.h>

#include "fzy.h"
double match(const char *needle, const char *haystack);

void usage(const char *argv0){
	fprintf(stderr, "USAGE: %s QUERY CANDIDATE\n", argv0);
}

int main(int argc, char *argv[]){
	if(argc != 3){
		usage(argv[0]);
	}

	if(has_match(argv[1], argv[2])){
		double result = match(argv[1], argv[2]);
		printf("%f\n", result);
	}

	return 0;
}
