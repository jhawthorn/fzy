#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include "options.h"
#include "choices.h"
#include "match.h"

/* Initial size of buffer for storing input in memory */
#define INITIAL_BUFFER_CAPACITY 4096

/* Initial size of choices array */
#define INITIAL_CHOICE_CAPACITY 128

static int
cmpchoice(const void *_idx1, const void *_idx2)
{
	const struct scored_result *a = _idx1;
	const struct scored_result *b = _idx2;

	if (a->score == b->score) {
		/* To ensure a stable sort, we must also sort by the string
		 * pointers. We can do this since we know all the strings are
		 * from a contiguous memory segment (buffer in choices_t).
		 */
		if (a->str < b->str) {
			return -1;
		} else {
			return 1;
		}
	} else if (a->score < b->score) {
		return 1;
	} else {
		return -1;
	}
}

static void *
safe_realloc(void *buffer, size_t size)
{
	buffer = realloc(buffer, size);
	if (!buffer) {
		fprintf(stderr, "Error: Can't allocate memory (%zu bytes)\n", size);
		abort();
	}

	return buffer;
}

void
choices_fread(choices_t *c, FILE *file, char input_delimiter)
{
	/* Save current position for parsing later */
	size_t buffer_start = c->buffer_size;

	/* Resize buffer to at least one byte more capacity than our current
	 * size. This uses a power of two of INITIAL_BUFFER_CAPACITY.
	 * This must work even when c->buffer is NULL and c->buffer_size is 0
	 */
	size_t capacity = INITIAL_BUFFER_CAPACITY;
	while (capacity <= c->buffer_size)
		capacity *= 2;
	c->buffer = safe_realloc(c->buffer, capacity);

	/* Continue reading until we get a "short" read, indicating EOF */
	while ((c->buffer_size += fread(c->buffer + c->buffer_size, 1, capacity - c->buffer_size, file)) == capacity) {
		capacity *= 2;
		c->buffer = safe_realloc(c->buffer, capacity);
	}
	c->buffer = safe_realloc(c->buffer, c->buffer_size + 1);
	c->buffer[c->buffer_size++] = '\0';

	/* Truncate buffer to used size, (maybe) freeing some memory for
	 * future allocations.
	 */

	/* Tokenize input and add to choices */
	const char *line_end = c->buffer + c->buffer_size;
	char *line = c->buffer + buffer_start;
	do {
		char *nl = strchr(line, input_delimiter);
		if (nl)
			*nl++ = '\0';

		/* Skip empty lines */
		if (*line)
			choices_add(c, line);

		line = nl;
	} while (line && line < line_end);
}

static void
choices_resize(choices_t *c, size_t new_capacity)
{
	c->strings = safe_realloc(c->strings, new_capacity * sizeof(const char *));
	c->capacity = new_capacity;
}

static void
choices_reset_search(choices_t *c)
{
	free(c->results);
	c->selection = c->available = 0;
	c->results = NULL;
}

void
choices_init(choices_t *c, options_t *options)
{
	c->strings = NULL;
	c->results = NULL;

	c->buffer_size = 0;
	c->buffer = NULL;

	c->capacity = c->size = 0;
	choices_resize(c, INITIAL_CHOICE_CAPACITY);

	if (options->workers)
		c->worker_count = options->workers;
	else
		c->worker_count = (int)sysconf(_SC_NPROCESSORS_ONLN);

	choices_reset_search(c);
}

void
choices_destroy(choices_t *c)
{
	free(c->buffer);
	c->buffer = NULL;
	c->buffer_size = 0;

	free(c->strings);
	c->strings = NULL;
	c->capacity = c->size = 0;

	free(c->results);
	c->results = NULL;
	c->available = c->selection = 0;
}

void
choices_add(choices_t *c, const char *choice)
{
	/* Previous search is now invalid */
	choices_reset_search(c);

	if (c->size == c->capacity)
		choices_resize(c, c->capacity * 2);

	c->strings[c->size++] = choice;
}

size_t
choices_available(choices_t *c)
{
	return c->available;
}

#define BATCH_SIZE 512

struct result_list {
	struct scored_result *list;
	size_t size;
};

struct search_job {
	pthread_mutex_t lock;
	choices_t *choices;
	const char *search;
	size_t processed;
	struct worker *workers;
};

struct worker {
	pthread_t thread_id;
	struct search_job *job;
	unsigned int worker_num;
	struct result_list result;
};

static void
worker_get_next_batch(struct search_job *job, size_t *start, size_t *end)
{
	pthread_mutex_lock(&job->lock);

	*start = job->processed;

	job->processed += BATCH_SIZE;
	if (job->processed > job->choices->size)
		job->processed = job->choices->size;

	*end = job->processed;

	pthread_mutex_unlock(&job->lock);
}

static struct result_list merge2(struct result_list list1, struct result_list list2)
{
	size_t result_index = 0, index1 = 0, index2 = 0;

	struct result_list result;
	result.size = list1.size + list2.size;
	result.list = malloc(result.size * sizeof(struct scored_result));
	if (!result.list) {
		fprintf(stderr, "Error: Can't allocate memory\n");
		abort();
	}

	while(index1 < list1.size && index2 < list2.size) {
		if (cmpchoice(&list1.list[index1], &list2.list[index2]) < 0)
			result.list[result_index++] = list1.list[index1++];
		else
			result.list[result_index++] = list2.list[index2++];
	}

	while(index1 < list1.size)
		result.list[result_index++] = list1.list[index1++];

	while(index2 < list2.size)
		result.list[result_index++] = list2.list[index2++];

	free(list1.list);
	free(list2.list);

	return result;
}

static void *
choices_search_worker(void *data)
{
	struct worker *w = (struct worker *)data;
	struct search_job *job = w->job;
	const choices_t *c = job->choices;
	struct result_list *result = &w->result;

	size_t start, end;

	for(;;) {
		worker_get_next_batch(job, &start, &end);

		if(start == end)
			break;

		for(size_t i = start; i < end; i++) {
			if (has_match(job->search, c->strings[i])) {
				result->list[result->size].str = c->strings[i];
				result->list[result->size].score = match(job->search, c->strings[i]);
				result->size++;
			}
		}
	}

	/* Sort the partial result */
	qsort(result->list, result->size, sizeof(struct scored_result), cmpchoice);

	/* Fan-in, merging results */
	for(unsigned int step = 0;; step++) {
		if (w->worker_num % (2 << step))
			break;

		unsigned int next_worker = w->worker_num | (1 << step);
		if (next_worker >= c->worker_count)
			break;

		if ((errno = pthread_join(job->workers[next_worker].thread_id, NULL))) {
			perror("pthread_join");
			exit(EXIT_FAILURE);
		}

		w->result = merge2(w->result, job->workers[next_worker].result);
	}

	return (char *)NULL;
}

void
choices_search(choices_t *c, const char *search)
{
	choices_reset_search(c);

	struct search_job *job = calloc(1, sizeof(struct search_job));
	if (!job) {
		fprintf(stderr, "Error: Can't allocate memory\n");
		abort();
	}

	job->search = search;
	job->choices = c;
	if (pthread_mutex_init(&job->lock, NULL) != 0) {
		fprintf(stderr, "Error: pthread_mutex_init failed\n");
		abort();
	}

	job->workers = calloc(c->worker_count, sizeof(struct worker));
	if (!job->workers) {
		fprintf(stderr, "Error: Can't allocate memory\n");
		abort();
	}

	struct worker *workers = job->workers;
	for (int i = c->worker_count - 1; i >= 0; i--) {
		workers[i].job = job;
		workers[i].worker_num = i;
		workers[i].result.size = 0;
		workers[i].result.list = malloc(c->size * sizeof(struct scored_result)); /* FIXME: This is overkill */

		/* These must be created last-to-first to avoid a race condition when fanning in */
		if ((errno = pthread_create(&workers[i].thread_id, NULL, &choices_search_worker, &workers[i]))) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	if (pthread_join(workers[0].thread_id, NULL)) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	c->results = workers[0].result.list;
	c->available = workers[0].result.size;

	free(workers);
	pthread_mutex_destroy(&job->lock);
	free(job);
}

const char *
choices_get(choices_t *c, size_t n)
{
	if (n < c->available)
		return c->results[n].str;
	else
		return (char *)NULL;
}

score_t
choices_getscore(choices_t *c, size_t n)
{
	return c->results[n].score;
}

void
choices_prev(choices_t *c)
{
	if (c->available)
		c->selection = (c->selection + c->available - 1) % c->available;
}

void
choices_next(choices_t *c)
{
	if (c->available)
		c->selection = (c->selection + 1) % c->available;
}
