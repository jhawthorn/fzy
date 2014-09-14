#define _GNU_SOURCE
#include <stdlib.h>

#include "choices.h"
#include "fzy.h"

#define INITIAL_CAPACITY 1

static int cmpchoice(size_t *idx1, size_t *idx2, double *choices_score) {
	double score1 = choices_score[*idx1];
	double score2 = choices_score[*idx2];

	if(score1 == score2)
		return 0;
	else if(score1 < score2)
		return 1;
	else
		return -1;
}

static void choices_resize(choices_t *c, int new_capacity){
	c->strings = realloc(c->strings, new_capacity * sizeof(const char *));
	c->scores  = realloc(c->scores,  new_capacity * sizeof(double));
	c->sorted  = realloc(c->sorted,  new_capacity * sizeof(size_t));

	for(int i = c->capacity; i < new_capacity; i++){
		c->strings[i] = NULL;
	}
	c->capacity = new_capacity;
}

void choices_init(choices_t *c){
	c->strings = NULL;
	c->scores  = NULL;
	c->sorted  = NULL;
	c->capacity = c->size = 0;
	c->selection = c->available = 0;
	choices_resize(c, INITIAL_CAPACITY);
}
void choices_free(choices_t *c){
	free(c->strings);
	free(c->scores);
	free(c->sorted);
};

void choices_add(choices_t *c, const char *choice){
	if(c->size == c->capacity){
		choices_resize(c, c->capacity * 2);
	}
	c->strings[c->size++] = choice;
}

size_t choices_available(choices_t *c){
	return c->available;
}

void choices_search(choices_t *c, const char *search){
	c->selection = 0;
	c->available = 0;

	for(size_t i = 0; i < c->size; i++){
		if(has_match(search, c->strings[i])){
			c->scores[i] = match(search, c->strings[i]);
			c->sorted[c->available++] = i;
		}
	}

	qsort_r(c->sorted, c->available, sizeof(size_t), (int (*)(const void *, const void *, void *))cmpchoice, c->scores);
}

const char *choices_get(choices_t *c, size_t n){
	if(n < c->available){
		return c->strings[c->sorted[n]];
	}else{
		return NULL;
	}
}
double choices_getscore(choices_t *c, size_t n){
	return c->scores[c->sorted[n]];;
}

void choices_prev(choices_t *c){
	c->selection = (c->selection + c->available - 1) % c->available;
}

void choices_next(choices_t *c){
	c->selection = (c->selection + 1) % c->available;
}

