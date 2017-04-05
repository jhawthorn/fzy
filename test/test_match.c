#include <stdlib.h>

#include "../config.h"
#include "match.h"

#include "greatest/greatest.h"

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
	ASSERT_EQ(SCORE_MAX, match("abc", "abc"));
	ASSERT_EQ(SCORE_MAX, match("aBc", "abC"));

	/* Empty query always results in SCORE_MIN */
	ASSERT_EQ(SCORE_MIN, match("", ""));
	ASSERT_EQ(SCORE_MIN, match("", "a"));
	ASSERT_EQ(SCORE_MIN, match("", "bb"));

	/* Gaps */
	ASSERT_EQ(SCORE_GAP_LEADING, match("a", "*a"));
	ASSERT_EQ(SCORE_GAP_LEADING*2, match("a", "*ba"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_GAP_TRAILING, match("a", "**a*"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_GAP_TRAILING*2, match("a", "**a**"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_CONSECUTIVE + SCORE_GAP_TRAILING*2, match("aa", "**aa**"));
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_GAP_LEADING + SCORE_GAP_INNER + SCORE_GAP_TRAILING + SCORE_GAP_TRAILING, match("aa", "**a*a**"));

	/* Consecutive */
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_CONSECUTIVE, match("aa", "*aa"));
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_CONSECUTIVE*2, match("aaa", "*aaa"));
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_GAP_INNER + SCORE_MATCH_CONSECUTIVE, match("aaa", "*a*aa"));

	/* Slash */
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_SLASH, match("a", "/a"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_SLASH, match("a", "*/a"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_SLASH + SCORE_MATCH_CONSECUTIVE, match("aa", "a/aa"));

	/* Capital */
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_CAPITAL, match("a", "bA"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_CAPITAL, match("a", "baA"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_CAPITAL + SCORE_MATCH_CONSECUTIVE, match("aa", "baAa"));

	/* Dot */
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_DOT, match("a", ".a"));
	ASSERT_EQ(SCORE_GAP_LEADING*3 + SCORE_MATCH_DOT, match("a", "*a.a"));
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_GAP_INNER + SCORE_MATCH_DOT, match("a", "*a.a"));

	PASS();
}

TEST test_positions_1() {
	size_t positions[3];
	match_positions("amo", "app/models/foo", positions);
	ASSERT_EQ(0, positions[0]);
	ASSERT_EQ(4, positions[1]);
	ASSERT_EQ(5, positions[2]);

	PASS();
}

TEST test_positions_2() {
	/*
	 * We should prefer matching the 'o' in order, since it's the beginning
	 * of a word.
	 */
	size_t positions[4];
	match_positions("amor", "app/models/order", positions);
	ASSERT_EQ(0, positions[0]);
	ASSERT_EQ(4, positions[1]);
	ASSERT_EQ(11, positions[2]);

	PASS();
}

TEST test_positions_3() {
	size_t positions[2];
	match_positions("as", "tags", positions);
	ASSERT_EQ(1, positions[0]);
	ASSERT_EQ(3, positions[1]);

	PASS();
}

TEST test_positions_4() {
	size_t positions[2];
	match_positions("as", "examples.txt", positions);
	ASSERT_EQ(2, positions[0]);
	ASSERT_EQ(7, positions[1]);

	PASS();
}

TEST test_positions_5() {
	size_t positions[3];
	match_positions("abc", "a/a/b/c/c", positions);
	ASSERT_EQ(2, positions[0]);
	ASSERT_EQ(4, positions[1]);
	ASSERT_EQ(6, positions[2]);

	PASS();
}

TEST test_positions_exact() {
	size_t positions[3];
	match_positions("foo", "foo", positions);
	ASSERT_EQ(0, positions[0]);
	ASSERT_EQ(1, positions[1]);
	ASSERT_EQ(2, positions[2]);

	PASS();
}

SUITE(match_suite) {
	RUN_TEST(test_match);
	RUN_TEST(test_relative_scores);
	RUN_TEST(test_exact_scores);
	RUN_TEST(test_positions_1);
	RUN_TEST(test_positions_2);
	RUN_TEST(test_positions_3);
	RUN_TEST(test_positions_4);
	RUN_TEST(test_positions_5);
	RUN_TEST(test_positions_exact);
}
