#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

void *safe_realloc(void *buffer, size_t size) {
	buffer = realloc(buffer, size);
	if (!buffer) {
		fprintf(stderr, "Error: Can't allocate memory (%zu bytes)\n", size);
		abort();
	}

	return buffer;
}

char *safe_strdup(const char *buffer) {
	char *new_buffer = strdup(buffer);
	if (!new_buffer) {
		fprintf(stderr, "Error: Can't allocate memory\n");
		abort();
	}
	return new_buffer;
}
