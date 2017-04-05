#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "../config.h"
#include "options.h"
#include "choices.h"

#include "greatest/greatest.h"

static options_t default_options;

TEST test_choices_empty() {
	choices_t choices;
	choices_init(&choices, &default_options);
	ASSERT_EQ(0, choices.size);
	ASSERT_EQ(0, choices.available);
	ASSERT_EQ(0, choices.selection);

	choices_prev(&choices);
	ASSERT_EQ(0, choices.selection);

	choices_next(&choices);
	ASSERT_EQ(0, choices.selection);

	choices_destroy(&choices);

	PASS();
}

TEST test_choices_1() {
	choices_t choices;
	choices_init(&choices, &default_options);
	choices_add(&choices, "tags");

	choices_search(&choices, "");
	ASSERT_EQ(1, choices.available);
	ASSERT_EQ(0, choices.selection);

	choices_search(&choices, "t");
	ASSERT_EQ(1, choices.available);
	ASSERT_EQ(0, choices.selection);

	choices_prev(&choices);
	ASSERT_EQ(0, choices.selection);

	choices_next(&choices);
	ASSERT_EQ(0, choices.selection);

	ASSERT(!strcmp(choices_get(&choices, 0), "tags"));
	ASSERT_EQ(NULL, choices_get(&choices, 1));

	choices_destroy(&choices);

	PASS();
}

TEST test_choices_2() {
	choices_t choices;
	choices_init(&choices, &default_options);
	choices_add(&choices, "tags");
	choices_add(&choices, "test");

	/* Empty search */
	choices_search(&choices, "");
	ASSERT_EQ(0, choices.selection);
	ASSERT_EQ(2, choices.available);

	choices_next(&choices);
	ASSERT_EQ(1, choices.selection);
	choices_next(&choices);
	ASSERT_EQ(0, choices.selection);

	choices_prev(&choices);
	ASSERT_EQ(1, choices.selection);
	choices_prev(&choices);
	ASSERT_EQ(0, choices.selection);

	/* Filtered search */
	choices_search(&choices, "te");
	ASSERT_EQ(1, choices.available);
	ASSERT_EQ(0, choices.selection);
	ASSERT_STR_EQ("test", choices_get(&choices, 0));

	choices_next(&choices);
	ASSERT_EQ(0, choices.selection);

	choices_prev(&choices);
	ASSERT_EQ(0, choices.selection);

	/* No results */
	choices_search(&choices, "foobar");
	ASSERT_EQ(0, choices.available);
	ASSERT_EQ(0, choices.selection);

	/* Different order due to scoring */
	choices_search(&choices, "ts");
	ASSERT_EQ(2, choices.available);
	ASSERT_EQ(0, choices.selection);
	ASSERT_STR_EQ("test", choices_get(&choices, 0));
	ASSERT_STR_EQ("tags", choices_get(&choices, 1));

	choices_destroy(&choices);

	PASS();
}

TEST test_choices_without_search() {
	/* Before a search is run, it should return no results */

	choices_t choices;
	choices_init(&choices, &default_options);

	ASSERT_EQ(0, choices.available);
	ASSERT_EQ(0, choices.selection);
	ASSERT_EQ(0, choices.size);
	ASSERT_EQ(NULL, choices_get(&choices, 0));

	choices_add(&choices, "test");

	ASSERT_EQ(0, choices.available);
	ASSERT_EQ(0, choices.selection);
	ASSERT_EQ(1, choices.size);
	ASSERT_EQ(NULL, choices_get(&choices, 0));

	choices_destroy(&choices);

	PASS();
}

/* Regression test for segfault */
TEST test_choices_unicode() {
	choices_t choices;
	choices_init(&choices, &default_options);

	choices_add(&choices, "Edmund Husserl - Méditations cartésiennes - Introduction a la phénoménologie.pdf");
	choices_search(&choices, "e");

	choices_destroy(&choices);
	PASS();
}

TEST test_choices_large_input() {
	choices_t choices;
	choices_init(&choices, &default_options);

	int N = 100000;
	char *strings[N];

	for(int i = 0; i < N; i++) {
		asprintf(&strings[i], "%i", i);
		choices_add(&choices, strings[i]);
	}

	choices_search(&choices, "12");

	/* Must match `seq 0 99999 | grep '.*1.*2.*' | wc -l` */
	ASSERT_EQ(8146, choices.available);

	ASSERT_STR_EQ("12", choices_get(&choices, 0));

	for(int i = 0; i < N; i++) {
		free(strings[i]);
	}

	choices_destroy(&choices);

	PASS();
}

SUITE(choices_suite) {
	options_init(&default_options);

	RUN_TEST(test_choices_empty);
	RUN_TEST(test_choices_1);
	RUN_TEST(test_choices_2);
	RUN_TEST(test_choices_without_search);
	RUN_TEST(test_choices_unicode);
	RUN_TEST(test_choices_large_input);
}
