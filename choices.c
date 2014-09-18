#include <stdlib.h>
#include <stdio.h>

#include "choices.h"
#include "match.h"

#define INITIAL_CAPACITY 1

static int cmpchoice(const void *_idx1, const void *_idx2) {
	const struct scored_position *a = _idx1;
	const struct scored_position *b = _idx2;

	if(a->score == b->score)
		return 0;
	else if(a->score < b->score)
		return 1;
	else
		return -1;
}

static void choices_resize(choices_t *c, int new_capacity){
	c->strings = realloc(c->strings, new_capacity * sizeof(const char *));
	c->results = realloc(c->results, new_capacity * sizeof(struct scored_position));

	if(!c->strings || !c->results){
		fprintf(stderr, "Error: Can't allocate memory\n");
		abort();
	}

	for(int i = c->capacity; i < new_capacity; i++){
		c->strings[i] = NULL;
	}
	c->capacity = new_capacity;
}

void choices_init(choices_t *c){
	c->strings = NULL;
	c->results = NULL;
	c->capacity = c->size = 0;
	c->selection = c->available = 0;
	choices_resize(c, INITIAL_CAPACITY);
}

void choices_free(choices_t *c){
	free(c->strings);
	free(c->results);
}

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
			c->results[c->available].position = i;
			c->results[c->available].score = match(search, c->strings[i]);
			c->available++;
		}
	}

	qsort(c->results, c->available, sizeof(struct scored_position), cmpchoice);
}

const char *choices_get(choices_t *c, size_t n){
	if(n < c->available){
		return c->strings[c->results[n].position];
	}else{
		return NULL;
	}
}
double choices_getscore(choices_t *c, size_t n){
	return c->results[n].score;;
}

void choices_prev(choices_t *c){
	if(c->available)
		c->selection = (c->selection + c->available - 1) % c->available;
}

void choices_next(choices_t *c){
	if(c->available)
		c->selection = (c->selection + 1) % c->available;
}

