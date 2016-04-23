#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "choices.h"
#include "match.h"

#define INITIAL_CAPACITY 1

static int cmpchoice(const void *_idx1, const void *_idx2) {
	const struct scored_result *a = _idx1;
	const struct scored_result *b = _idx2;

	if (a->score == b->score)
		return 0;
	else if (a->score < b->score)
		return 1;
	else
		return -1;
}

static void *safe_realloc(void *buffer, size_t size) {
	buffer = realloc(buffer, size);
	if (!buffer) {
		fprintf(stderr, "Error: Can't allocate memory (%zu bytes)\n", size);
		abort();
	}

	return buffer;
}

void choices_fread(choices_t *c, FILE *file) {
	size_t bufsize = 65536, pos = 0;
	char *buf = safe_realloc(NULL, bufsize);

	/* Continue reading until we get a "short" read, indicating EOF */
	while ((pos += fread(buf + pos, 1, bufsize - pos, file)) == bufsize) {
		bufsize *= 2;
		buf = safe_realloc(buf, bufsize);
	}
	buf[pos] = 0;

	/* Truncate buffer to used size, (maybe) freeing some memory for
	 * future allocations.
	 */
	buf = safe_realloc(buf, pos + 1);

	/* Tokenize input and add to choices */
	char *line = buf;
	do {
		char *nl = strchr(line, '\n');
		if (nl)
			*nl++ = '\0';

		/* Skip empty lines */
		if (*line)
			choices_add(c, line);

		line = nl;
	} while (line);
}

static void choices_resize(choices_t *c, size_t new_capacity) {
	c->strings = safe_realloc(c->strings, new_capacity * sizeof(const char *));
	c->capacity = new_capacity;
}

static void choices_reset_search(choices_t *c) {
	free(c->results);
	c->selection = c->available = 0;
	c->results = NULL;
}

void choices_init(choices_t *c) {
	c->strings = NULL;
	c->results = NULL;
	c->capacity = c->size = 0;
	choices_reset_search(c);
	choices_resize(c, INITIAL_CAPACITY);
}

void choices_free(choices_t *c) {
	free(c->strings);
	free(c->results);
}

void choices_add(choices_t *c, const char *choice) {
	/* Previous search is now invalid */
	choices_reset_search(c);

	if (c->size == c->capacity) {
		choices_resize(c, c->capacity * 2);
	}
	c->strings[c->size++] = choice;
}

size_t choices_available(choices_t *c) {
	return c->available;
}

void choices_search(choices_t *c, const char *search) {
	choices_reset_search(c);

	c->results = malloc(c->size * sizeof(struct scored_result));
	if (!c->results) {
		fprintf(stderr, "Error: Can't allocate memory\n");
		abort();
	}

	for (size_t i = 0; i < c->size; i++) {
		if (has_match(search, c->strings[i])) {
			c->results[c->available].str = c->strings[i];
			c->results[c->available].score = match(search, c->strings[i]);
			c->available++;
		}
	}

	qsort(c->results, c->available, sizeof(struct scored_result), cmpchoice);
}

const char *choices_get(choices_t *c, size_t n) {
	if (n < c->available) {
		return c->results[n].str;
	} else {
		return NULL;
	}
}

double choices_getscore(choices_t *c, size_t n) {
	return c->results[n].score;
}

void choices_prev(choices_t *c) {
	if (c->available)
		c->selection = (c->selection + c->available - 1) % c->available;
}

void choices_next(choices_t *c) {
	if (c->available)
		c->selection = (c->selection + 1) % c->available;
}
