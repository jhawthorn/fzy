#include <stdlib.h>

#include "../config.h"
#include "match.h"

#include "greatest/greatest.h"

/* has_match(char *needle, char *haystack) */
TEST exact_match_should_return_true() {
	ASSERT(has_match("a", "a"));
	PASS();
}

TEST partial_match_should_return_true() {
	ASSERT(has_match("a", "ab"));
	ASSERT(has_match("a", "ba"));
	PASS();
}

TEST match_with_delimiters_in_between() {
	ASSERT(has_match("abc", "a|b|c"));
	PASS();
}

TEST non_match_should_return_false() {
	ASSERT(!has_match("a", ""));
	ASSERT(!has_match("a", "b"));
	ASSERT(!has_match("ass", "tags"));
	PASS();
}

TEST empty_query_should_always_match() {
	/* match when query is empty */
	ASSERT(has_match("", ""));
	ASSERT(has_match("", "a"));
	PASS();
}

/* match(char *needle, char *haystack) */

TEST should_prefer_starts_of_words() {
	/* App/Models/Order is better than App/MOdels/zRder  */
	ASSERT(match("amor", "app/models/order") > match("amor", "app/models/zrder"));
	PASS();
}

TEST should_prefer_consecutive_letters() {
	/* App/MOdels/foo is better than App/M/fOo  */
	ASSERT(match("amo", "app/m/foo") < match("amo", "app/models/foo"));
	PASS();
}

TEST should_prefer_contiguous_over_letter_following_period() {
	/* GEMFIle.Lock < GEMFILe  */
	ASSERT(match("gemfil", "Gemfile.lock") < match("gemfil", "Gemfile"));
	PASS();
}

TEST should_prefer_shorter_matches() {
	ASSERT(match("abce", "abcdef") > match("abce", "abc de"));
	ASSERT(match("abc", "    a b c ") > match("abc", " a  b  c "));
	ASSERT(match("abc", " a b c    ") > match("abc", " a  b  c "));
	PASS();
}

TEST should_prefer_shorter_candidates() {
	ASSERT(match("test", "tests") > match("test", "testing"));
	PASS();
}

TEST should_prefer_start_of_candidate() {
	/* Scores first letter highly */
	ASSERT(match("test", "testing") > match("test", "/testing"));
	PASS();
}

TEST score_exact_match() {
	/* Exact match is SCORE_MAX */
	ASSERT_EQ(SCORE_MAX, match("abc", "abc"));
	ASSERT_EQ(SCORE_MAX, match("aBc", "abC"));
	PASS();
}

TEST score_empty_query() {
	/* Empty query always results in SCORE_MIN */
	ASSERT_EQ(SCORE_MIN, match("", ""));
	ASSERT_EQ(SCORE_MIN, match("", "a"));
	ASSERT_EQ(SCORE_MIN, match("", "bb"));
	PASS();
}

TEST score_gaps() {
	ASSERT_EQ(SCORE_GAP_LEADING, match("a", "*a"));
	ASSERT_EQ(SCORE_GAP_LEADING*2, match("a", "*ba"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_GAP_TRAILING, match("a", "**a*"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_GAP_TRAILING*2, match("a", "**a**"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_CONSECUTIVE + SCORE_GAP_TRAILING*2, match("aa", "**aa**"));
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_GAP_LEADING + SCORE_GAP_INNER + SCORE_GAP_TRAILING + SCORE_GAP_TRAILING, match("aa", "**a*a**"));
	PASS();
}

TEST score_consecutive() {
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_CONSECUTIVE, match("aa", "*aa"));
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_CONSECUTIVE*2, match("aaa", "*aaa"));
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_GAP_INNER + SCORE_MATCH_CONSECUTIVE, match("aaa", "*a*aa"));
	PASS();
}

TEST score_slash() {
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_SLASH, match("a", "/a"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_SLASH, match("a", "*/a"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_SLASH + SCORE_MATCH_CONSECUTIVE, match("aa", "a/aa"));
	PASS();
}

TEST score_capital() {
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_CAPITAL, match("a", "bA"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_CAPITAL, match("a", "baA"));
	ASSERT_EQ(SCORE_GAP_LEADING*2 + SCORE_MATCH_CAPITAL + SCORE_MATCH_CONSECUTIVE, match("aa", "baAa"));
	PASS();
}

TEST score_dot() {
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_MATCH_DOT, match("a", ".a"));
	ASSERT_EQ(SCORE_GAP_LEADING*3 + SCORE_MATCH_DOT, match("a", "*a.a"));
	ASSERT_EQ(SCORE_GAP_LEADING + SCORE_GAP_INNER + SCORE_MATCH_DOT, match("a", "*a.a"));
	PASS();
}

TEST positions_consecutive() {
	size_t positions[3];
	match_positions("amo", "app/models/foo", positions);
	ASSERT_EQ(0, positions[0]);
	ASSERT_EQ(4, positions[1]);
	ASSERT_EQ(5, positions[2]);

	PASS();
}

TEST positions_start_of_word() {
	/*
	 * We should prefer matching the 'o' in order, since it's the beginning
	 * of a word.
	 */
	size_t positions[4];
	match_positions("amor", "app/models/order", positions);
	ASSERT_EQ(0, positions[0]);
	ASSERT_EQ(4, positions[1]);
	ASSERT_EQ(11, positions[2]);
	ASSERT_EQ(12, positions[3]);

	PASS();
}

TEST positions_no_bonuses() {
	size_t positions[2];
	match_positions("as", "tags", positions);
	ASSERT_EQ(1, positions[0]);
	ASSERT_EQ(3, positions[1]);

	match_positions("as", "examples.txt", positions);
	ASSERT_EQ(2, positions[0]);
	ASSERT_EQ(7, positions[1]);

	PASS();
}

TEST positions_multiple_candidates_start_of_words() {
	size_t positions[3];
	match_positions("abc", "a/a/b/c/c", positions);
	ASSERT_EQ(2, positions[0]);
	ASSERT_EQ(4, positions[1]);
	ASSERT_EQ(6, positions[2]);

	PASS();
}

TEST positions_exact_match() {
	size_t positions[3];
	match_positions("foo", "foo", positions);
	ASSERT_EQ(0, positions[0]);
	ASSERT_EQ(1, positions[1]);
	ASSERT_EQ(2, positions[2]);

	PASS();
}

SUITE(match_suite) {
	RUN_TEST(exact_match_should_return_true);
	RUN_TEST(partial_match_should_return_true);
	RUN_TEST(empty_query_should_always_match);
	RUN_TEST(non_match_should_return_false);
	RUN_TEST(match_with_delimiters_in_between);

	RUN_TEST(should_prefer_starts_of_words);
	RUN_TEST(should_prefer_consecutive_letters);
	RUN_TEST(should_prefer_contiguous_over_letter_following_period);
	RUN_TEST(should_prefer_shorter_matches);
	RUN_TEST(should_prefer_shorter_candidates);
	RUN_TEST(should_prefer_start_of_candidate);

	RUN_TEST(score_exact_match);
	RUN_TEST(score_empty_query);
	RUN_TEST(score_gaps);
	RUN_TEST(score_consecutive);
	RUN_TEST(score_slash);
	RUN_TEST(score_capital);
	RUN_TEST(score_dot);

	RUN_TEST(positions_consecutive);
	RUN_TEST(positions_start_of_word);
	RUN_TEST(positions_no_bonuses);
	RUN_TEST(positions_multiple_candidates_start_of_words);
	RUN_TEST(positions_exact_match);
}
