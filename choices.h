#ifndef CHOICES_H
#define CHOICES_H CHOICES_H

#include <stdio.h>

struct scored_result {
	double score;
	const char *str;
};

typedef struct {
	size_t capacity;
	size_t size;

	const char **strings;
	struct scored_result *results;

	size_t available;
	size_t selection;
} choices_t;

void choices_init(choices_t *c);
void choices_fread(choices_t *c, FILE *file);
void choices_free(choices_t *c);
void choices_add(choices_t *c, const char *choice);
size_t choices_available(choices_t *c);
void choices_search(choices_t *c, const char *search);
const char *choices_get(choices_t *c, size_t n);
double choices_getscore(choices_t *c, size_t n);
void choices_prev(choices_t *c);
void choices_next(choices_t *c);

#endif
