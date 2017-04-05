#define _DEFAULT_SOURCE
#include <string.h>

#include "greatest/greatest.h"
#include "theft/theft.h"

#include "match.h"

static void *string_alloc_cb(struct theft *t, theft_hash seed, void *env) {
	(void)env;
	int limit = 128;

	size_t sz = (size_t)(seed % limit) + 1;
	char *str = malloc(sz + 1);
	if (str == NULL) {
		return THEFT_ERROR;
	}

	for (size_t i = 0; i < sz; i += sizeof(theft_hash)) {
		theft_hash s = theft_random(t);
		for (uint8_t b = 0; b < sizeof(theft_hash); b++) {
			if (i + b >= sz) {
				break;
			}
			str[i + b] = (uint8_t)(s >> (8 * b)) & 0xff;
		}
	}
	str[sz] = 0;

	return str;
}

static void string_free_cb(void *instance, void *env) {
	free(instance);
	(void)env;
}

static void string_print_cb(FILE *f, void *instance, void *env) {
	char *str = (char *)instance;
	(void)env;
	size_t size = strlen(str);
	fprintf(f, "str[%zd]:\n    ", size);
	uint8_t bytes = 0;
	for (size_t i = 0; i < size; i++) {
		fprintf(f, "%02x", str[i]);
		bytes++;
		if (bytes == 16) {
			fprintf(f, "\n    ");
			bytes = 0;
		}
	}
	fprintf(f, "\n");
}

static uint64_t string_hash_cb(void *instance, void *env) {
	(void)env;
	char *str = (char *)instance;
	int size = strlen(str);
	return theft_hash_onepass((uint8_t *)str, size);
}

static void *string_shrink_cb(void *instance, uint32_t tactic, void *env) {
	(void)env;
	char *str = (char *)instance;
	int n = strlen(str);

	if (tactic == 0) { /* first half */
		return strndup(str, n / 2);
	} else if (tactic == 1) { /* second half */
		return strndup(str + (n / 2), n / 2);
	} else {
		return THEFT_NO_MORE_TACTICS;
	}
}

static struct theft_type_info string_info = {
    .alloc = string_alloc_cb,
    .free = string_free_cb,
    .print = string_print_cb,
    .hash = string_hash_cb,
    .shrink = string_shrink_cb,
};

static theft_trial_res prop_should_return_results_if_there_is_a_match(char *needle,
								      char *haystack) {
	int match_exists = has_match(needle, haystack);
	if (!match_exists)
		return THEFT_TRIAL_SKIP;

	score_t score = match(needle, haystack);

	if (needle[0] == '\0')
		return THEFT_TRIAL_SKIP;

	if (score == SCORE_MIN)
		return THEFT_TRIAL_FAIL;

	return THEFT_TRIAL_PASS;
}

TEST should_return_results_if_there_is_a_match() {
	struct theft *t = theft_init(0);
	struct theft_cfg cfg = {
	    .name = __func__,
	    .fun = prop_should_return_results_if_there_is_a_match,
	    .type_info = {&string_info, &string_info},
	    .trials = 100000,
	};

	theft_run_res res = theft_run(t, &cfg);
	theft_free(t);
	GREATEST_ASSERT_EQm("should_return_results_if_there_is_a_match", THEFT_RUN_PASS, res);
	PASS();
}

static theft_trial_res prop_positions_should_match_characters_in_string(char *needle,
									char *haystack) {
	int match_exists = has_match(needle, haystack);
	if (!match_exists)
		return THEFT_TRIAL_SKIP;

	int n = strlen(needle);
	size_t *positions = calloc(n, sizeof(size_t));
	if (!positions)
		return THEFT_TRIAL_ERROR;

	match_positions(needle, haystack, positions);

	/* Must be increasing */
	for (int i = 1; i < n; i++) {
		if (positions[i] <= positions[i - 1]) {
			return THEFT_TRIAL_FAIL;
		}
	}

	/* Matching characters must be in returned positions */
	for (int i = 0; i < n; i++) {
		if (toupper(needle[i]) != toupper(haystack[positions[i]])) {
			return THEFT_TRIAL_FAIL;
		}
	}

	free(positions);
	return THEFT_TRIAL_PASS;
}

TEST positions_should_match_characters_in_string() {
	struct theft *t = theft_init(0);
	struct theft_cfg cfg = {
	    .name = __func__,
	    .fun = prop_positions_should_match_characters_in_string,
	    .type_info = {&string_info, &string_info},
	    .trials = 100000,
	};

	theft_run_res res = theft_run(t, &cfg);
	theft_free(t);
	GREATEST_ASSERT_EQm("should_return_results_if_there_is_a_match", THEFT_RUN_PASS, res);
	PASS();
}

SUITE(properties_suite) {
	RUN_TEST(should_return_results_if_there_is_a_match);
	RUN_TEST(positions_should_match_characters_in_string);
}
