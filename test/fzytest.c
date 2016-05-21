#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "match.h"
#include "choices.h"

int testsrun = 0, testsfailed = 0, assertionsrun = 0;

#define assert(x)                                                                                  \
	if (++assertionsrun && !(x)) {                                                             \
		fprintf(stderr, "test \"%s\" failed\n   assert(%s) was false\n   at %s:%i\n\n",    \
			__func__, #x, __FILE__, __LINE__);                                         \
		raise(SIGTRAP);                                                                    \
		testsfailed++;                                                                     \
		return;                                                                            \
	}

#define assert_streq(a, b) assert(!strcmp(a, b))

void runtest(void (*test)()) {
	testsrun++;
	test();
}

void test_match() {
	assert(has_match("a", "a"));
	assert(has_match("a", "ab"));
	assert(has_match("a", "ba"));
	assert(has_match("abc", "a|b|c"));

	/* non-match */
	assert(!has_match("a", ""));
	assert(!has_match("a", "b"));
	assert(!has_match("ass", "tags"));

	/* match when query is empty */
	assert(has_match("", ""));
	assert(has_match("", "a"));
}

void test_scoring() {
	/* App/Models/Order is better than App/MOdels/zRder  */
	assert(match("amor", "app/models/order") > match("amor", "app/models/zrder"));

	/* App/MOdels/foo is better than App/M/fOo  */
	assert(match("amo", "app/m/foo") < match("amo", "app/models/foo"));

	/* GEMFIle.Lock < GEMFILe  */
	assert(match("gemfil", "Gemfile.lock") < match("gemfil", "Gemfile"));

	/* GEMFIle.Lock < GEMFILe  */
	assert(match("gemfil", "Gemfile.lock") < match("gemfil", "Gemfile"));

	/* Prefer shorter matches */
	assert(match("abce", "abcdef") > match("abce", "abc de"));

	/* Prefer shorter candidates */
	assert(match("test", "tests") > match("test", "testing"));

	/* Scores first letter highly */
	assert(match("test", "testing") > match("test", "/testing"));

	/* Prefer shorter matches */
	assert(match("abc", "    a b c ") > match("abc", " a  b  c "));
	assert(match("abc", " a b c    ") > match("abc", " a  b  c "));
}

void test_positions_1() {
	size_t positions[3];
	match_positions("amo", "app/models/foo", positions);
	assert(positions[0] == 0);
	assert(positions[1] == 4);
	assert(positions[2] == 5);
}

void test_positions_2() {
	/*
	 * We should prefer matching the 'o' in order, since it's the beginning
	 * of a word.
	 */
	size_t positions[4];
	match_positions("amor", "app/models/order", positions);
	assert(positions[0] == 0);
	assert(positions[1] == 4);
	assert(positions[2] == 11);
}

void test_positions_3() {
	size_t positions[2];
	match_positions("as", "tags", positions);
	assert(positions[0] == 1);
	assert(positions[1] == 3);
}

void test_positions_4() {
	size_t positions[2];
	match_positions("as", "examples.txt", positions);
	assert(positions[0] == 2);
	assert(positions[1] == 7);
}

void test_positions_5() {
	size_t positions[3];
	match_positions("abc", "a/a/b/c/c", positions);
	assert(positions[0] == 2);
	assert(positions[1] == 4);
	assert(positions[2] == 6);
}

void test_positions_exact() {
	size_t positions[3];
	match_positions("foo", "foo", positions);
	assert(positions[0] == 0);
	assert(positions[1] == 1);
	assert(positions[2] == 2);
}

void test_choices_empty() {
	choices_t choices;
	choices_init(&choices);
	assert(choices.size == 0);
	assert(choices.available == 0);
	assert(choices.selection == 0);

	choices_prev(&choices);
	assert(choices.selection == 0);

	choices_next(&choices);
	assert(choices.selection == 0);

	choices_destroy(&choices);
}

void test_choices_1() {
	choices_t choices;
	choices_init(&choices);
	choices_add(&choices, "tags");

	choices_search(&choices, "");
	assert(choices.available == 1);
	assert(choices.selection == 0);

	choices_search(&choices, "t");
	assert(choices.available == 1);
	assert(choices.selection == 0);

	choices_prev(&choices);
	assert(choices.selection == 0);

	choices_next(&choices);
	assert(choices.selection == 0);

	assert(!strcmp(choices_get(&choices, 0), "tags"));
	assert(choices_get(&choices, 1) == NULL);

	choices_destroy(&choices);
}

void test_choices_2() {
	choices_t choices;
	choices_init(&choices);
	choices_add(&choices, "tags");
	choices_add(&choices, "test");

	/* Empty search */
	choices_search(&choices, "");
	assert(choices.selection == 0);
	assert(choices.available == 2);
	assert_streq(choices_get(&choices, 0), "tags");
	assert_streq(choices_get(&choices, 1), "test");

	choices_next(&choices);
	assert(choices.selection == 1);
	choices_next(&choices);
	assert(choices.selection == 0);

	choices_prev(&choices);
	assert(choices.selection == 1);
	choices_prev(&choices);
	assert(choices.selection == 0);

	/* Filtered search */
	choices_search(&choices, "te");
	assert(choices.available == 1);
	assert(choices.selection == 0);
	assert_streq(choices_get(&choices, 0), "test");

	choices_next(&choices);
	assert(choices.selection == 0);

	choices_prev(&choices);
	assert(choices.selection == 0);

	/* No results */
	choices_search(&choices, "foobar");
	assert(choices.available == 0);
	assert(choices.selection == 0);

	/* Different order due to scoring */
	choices_search(&choices, "ts");
	assert(choices.available == 2);
	assert(choices.selection == 0);
	assert_streq(choices_get(&choices, 0), "test");
	assert_streq(choices_get(&choices, 1), "tags");

	choices_destroy(&choices);
}

void test_choices_without_search() {
	/* Before a search is run, it should return no results */

	choices_t choices;
	choices_init(&choices);

	assert(choices.available == 0);
	assert(choices.selection == 0);
	assert(choices.size == 0);
	assert(choices_get(&choices, 0) == NULL);

	choices_add(&choices, "test");

	assert(choices.available == 0);
	assert(choices.selection == 0);
	assert(choices.size == 1);
	assert(choices_get(&choices, 0) == NULL);

	choices_destroy(&choices);
}

void summary() {
	printf("%i tests, %i assertions, %i failures\n", testsrun, assertionsrun, testsfailed);
}

static void ignore_signal(int signum) {
	(void)signum;
}

int main(int argc, char *argv[]) {
	(void)argc;
	(void)argv;

	/* We raise sigtrap on all assertion failures.
	 * If we have no debugger running, we should ignore it */
	signal(SIGTRAP, ignore_signal);

	runtest(test_match);
	runtest(test_scoring);
	runtest(test_positions_1);
	runtest(test_positions_2);
	runtest(test_positions_3);
	runtest(test_positions_4);
	runtest(test_positions_5);
	runtest(test_positions_exact);

	runtest(test_choices_empty);
	runtest(test_choices_1);
	runtest(test_choices_2);
	runtest(test_choices_without_search);

	summary();

	/* exit 0 if all tests pass */
	return !!testsfailed;
}
