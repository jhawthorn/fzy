#define _GNU_SOURCE
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

#include "../config.h"
#include "match.h"
#include "choices.h"
#include "options.h"

#include "greatest/greatest.h"

static options_t default_options;

TEST test_match() {
	ASSERT(has_match("a", "a"));
	ASSERT(has_match("a", "ab"));
	ASSERT(has_match("a", "ba"));
	ASSERT(has_match("abc", "a|b|c"));

	/* non-match */
	ASSERT(!has_match("a", ""));
	ASSERT(!has_match("a", "b"));
	ASSERT(!has_match("ass", "tags"));

	/* match when query is empty */
	ASSERT(has_match("", ""));
	ASSERT(has_match("", "a"));

	PASS();
}

TEST test_relative_scores() {
	/* App/Models/Order is better than App/MOdels/zRder  */
	ASSERT(match("amor", "app/models/order") > match("amor", "app/models/zrder"));

	/* App/MOdels/foo is better than App/M/fOo  */
	ASSERT(match("amo", "app/m/foo") < match("amo", "app/models/foo"));

	/* GEMFIle.Lock < GEMFILe  */
	ASSERT(match("gemfil", "Gemfile.lock") < match("gemfil", "Gemfile"));

	/* GEMFIle.Lock < GEMFILe  */
	ASSERT(match("gemfil", "Gemfile.lock") < match("gemfil", "Gemfile"));

	/* Prefer shorter matches */
	ASSERT(match("abce", "abcdef") > match("abce", "abc de"));

	/* Prefer shorter candidates */
	ASSERT(match("test", "tests") > match("test", "testing"));

	/* Scores first letter highly */
	ASSERT(match("test", "testing") > match("test", "/testing"));

	/* Prefer shorter matches */
	ASSERT(match("abc", "    a b c ") > match("abc", " a  b  c "));
	ASSERT(match("abc", " a b c    ") > match("abc", " a  b  c "));

	PASS();
}

TEST test_exact_scores() {
	/* Exact match is SCORE_MAX */
	ASSERT(match("abc", "abc") == SCORE_MAX);
	ASSERT(match("aBc", "abC") == SCORE_MAX);

	/* Empty query always results in SCORE_MIN */
	ASSERT(match("", "") == SCORE_MIN);
	ASSERT(match("", "a") == SCORE_MIN);
	ASSERT(match("", "bb") == SCORE_MIN);

	/* Gaps */
	ASSERT(match("a", "*a") == SCORE_GAP_LEADING);
	ASSERT(match("a", "*ba") == SCORE_GAP_LEADING*2);
	ASSERT(match("a", "**a*") == SCORE_GAP_LEADING*2 + SCORE_GAP_TRAILING);
	ASSERT(match("a", "**a**") == SCORE_GAP_LEADING*2 + SCORE_GAP_TRAILING*2);
	ASSERT(match("aa", "**aa**") == SCORE_GAP_LEADING*2 + SCORE_MATCH_CONSECUTIVE + SCORE_GAP_TRAILING*2);
	ASSERT(match("aa", "**a*a**") == SCORE_GAP_LEADING + SCORE_GAP_LEADING + SCORE_GAP_INNER + SCORE_GAP_TRAILING + SCORE_GAP_TRAILING);

	/* Consecutive */
	ASSERT(match("aa", "*aa") == SCORE_GAP_LEADING + SCORE_MATCH_CONSECUTIVE);
	ASSERT(match("aaa", "*aaa") == SCORE_GAP_LEADING + SCORE_MATCH_CONSECUTIVE*2);
	ASSERT(match("aaa", "*a*aa") == SCORE_GAP_LEADING + SCORE_GAP_INNER + SCORE_MATCH_CONSECUTIVE);

	/* Slash */
	ASSERT(match("a", "/a") == SCORE_GAP_LEADING + SCORE_MATCH_SLASH);
	ASSERT(match("a", "*/a") == SCORE_GAP_LEADING*2 + SCORE_MATCH_SLASH);
	ASSERT(match("aa", "a/aa") == SCORE_GAP_LEADING*2 + SCORE_MATCH_SLASH + SCORE_MATCH_CONSECUTIVE);

	/* Capital */
	ASSERT(match("a", "bA") == SCORE_GAP_LEADING + SCORE_MATCH_CAPITAL);
	ASSERT(match("a", "baA") == SCORE_GAP_LEADING*2 + SCORE_MATCH_CAPITAL);
	ASSERT(match("aa", "baAa") == SCORE_GAP_LEADING*2 + SCORE_MATCH_CAPITAL + SCORE_MATCH_CONSECUTIVE);

	/* Dot */
	ASSERT(match("a", ".a") == SCORE_GAP_LEADING + SCORE_MATCH_DOT);
	ASSERT(match("a", "*a.a") == SCORE_GAP_LEADING*3 + SCORE_MATCH_DOT);
	ASSERT(match("a", "*a.a") == SCORE_GAP_LEADING + SCORE_GAP_INNER + SCORE_MATCH_DOT);

	PASS();
}

TEST test_positions_1() {
	size_t positions[3];
	match_positions("amo", "app/models/foo", positions);
	ASSERT(positions[0] == 0);
	ASSERT(positions[1] == 4);
	ASSERT(positions[2] == 5);

	PASS();
}

TEST test_positions_2() {
	/*
	 * We should prefer matching the 'o' in order, since it's the beginning
	 * of a word.
	 */
	size_t positions[4];
	match_positions("amor", "app/models/order", positions);
	ASSERT(positions[0] == 0);
	ASSERT(positions[1] == 4);
	ASSERT(positions[2] == 11);

	PASS();
}

TEST test_positions_3() {
	size_t positions[2];
	match_positions("as", "tags", positions);
	ASSERT(positions[0] == 1);
	ASSERT(positions[1] == 3);

	PASS();
}

TEST test_positions_4() {
	size_t positions[2];
	match_positions("as", "examples.txt", positions);
	ASSERT(positions[0] == 2);
	ASSERT(positions[1] == 7);

	PASS();
}

TEST test_positions_5() {
	size_t positions[3];
	match_positions("abc", "a/a/b/c/c", positions);
	ASSERT(positions[0] == 2);
	ASSERT(positions[1] == 4);
	ASSERT(positions[2] == 6);

	PASS();
}

TEST test_positions_exact() {
	size_t positions[3];
	match_positions("foo", "foo", positions);
	ASSERT(positions[0] == 0);
	ASSERT(positions[1] == 1);
	ASSERT(positions[2] == 2);

	PASS();
}

TEST test_choices_empty() {
	choices_t choices;
	choices_init(&choices, &default_options);
	ASSERT(choices.size == 0);
	ASSERT(choices.available == 0);
	ASSERT(choices.selection == 0);

	choices_prev(&choices);
	ASSERT(choices.selection == 0);

	choices_next(&choices);
	ASSERT(choices.selection == 0);

	choices_destroy(&choices);

	PASS();
}

TEST test_choices_1() {
	choices_t choices;
	choices_init(&choices, &default_options);
	choices_add(&choices, "tags");

	choices_search(&choices, "");
	ASSERT(choices.available == 1);
	ASSERT(choices.selection == 0);

	choices_search(&choices, "t");
	ASSERT(choices.available == 1);
	ASSERT(choices.selection == 0);

	choices_prev(&choices);
	ASSERT(choices.selection == 0);

	choices_next(&choices);
	ASSERT(choices.selection == 0);

	ASSERT(!strcmp(choices_get(&choices, 0), "tags"));
	ASSERT(choices_get(&choices, 1) == NULL);

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
	ASSERT(choices.selection == 0);
	ASSERT(choices.available == 2);

	choices_next(&choices);
	ASSERT(choices.selection == 1);
	choices_next(&choices);
	ASSERT(choices.selection == 0);

	choices_prev(&choices);
	ASSERT(choices.selection == 1);
	choices_prev(&choices);
	ASSERT(choices.selection == 0);

	/* Filtered search */
	choices_search(&choices, "te");
	ASSERT(choices.available == 1);
	ASSERT(choices.selection == 0);
	ASSERT_STR_EQ(choices_get(&choices, 0), "test");

	choices_next(&choices);
	ASSERT(choices.selection == 0);

	choices_prev(&choices);
	ASSERT(choices.selection == 0);

	/* No results */
	choices_search(&choices, "foobar");
	ASSERT(choices.available == 0);
	ASSERT(choices.selection == 0);

	/* Different order due to scoring */
	choices_search(&choices, "ts");
	ASSERT(choices.available == 2);
	ASSERT(choices.selection == 0);
	ASSERT_STR_EQ(choices_get(&choices, 0), "test");
	ASSERT_STR_EQ(choices_get(&choices, 1), "tags");

	choices_destroy(&choices);

	PASS();
}

TEST test_choices_without_search() {
	/* Before a search is run, it should return no results */

	choices_t choices;
	choices_init(&choices, &default_options);

	ASSERT(choices.available == 0);
	ASSERT(choices.selection == 0);
	ASSERT(choices.size == 0);
	ASSERT(choices_get(&choices, 0) == NULL);

	choices_add(&choices, "test");

	ASSERT(choices.available == 0);
	ASSERT(choices.selection == 0);
	ASSERT(choices.size == 1);
	ASSERT(choices_get(&choices, 0) == NULL);

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
	ASSERT(choices.available == 8146);

	ASSERT_STR_EQ(choices_get(&choices, 0), "12");

	for(int i = 0; i < N; i++) {
		free(strings[i]);
	}

	choices_destroy(&choices);

	PASS();
}

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) {
	GREATEST_MAIN_BEGIN();

	options_init(&default_options);

	RUN_TEST(test_match);
	RUN_TEST(test_relative_scores);
	RUN_TEST(test_exact_scores);
	RUN_TEST(test_positions_1);
	RUN_TEST(test_positions_2);
	RUN_TEST(test_positions_3);
	RUN_TEST(test_positions_4);
	RUN_TEST(test_positions_5);
	RUN_TEST(test_positions_exact);

	RUN_TEST(test_choices_empty);
	RUN_TEST(test_choices_1);
	RUN_TEST(test_choices_2);
	RUN_TEST(test_choices_without_search);
	RUN_TEST(test_choices_unicode);
	RUN_TEST(test_choices_large_input);

	GREATEST_MAIN_END();
}
