#ifndef MEMORY_H
#define MEMORY_H MEMORY_H

#include <stddef.h>

void *safe_realloc(void *buffer, size_t size);
char *safe_strdup(const char *buffer);

#endif
