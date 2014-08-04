#include <stdio.h>
#include "fzy.h"

int testsrun = 0, testsfailed = 0, assertionsrun = 0;

#define assert(x) if(++assertionsrun && !(x)){fprintf(stderr, "test \"%s\" failed\n   assert(%s) was false\n   at %s:%i\n\n", __func__, #x, __FILE__ ,__LINE__);return -1;}

void runtest(int (*test)()){
	testsrun++;
	if(test())
		testsfailed++;
}

int test_match(){
	assert(has_match("a", "a"));
	assert(has_match("a", "ab"));
	assert(has_match("a", "ba"));
	assert(has_match("abc", "a|b|c"));

	/* non-match */
	assert(!has_match("a", ""));
	assert(!has_match("a", "b"));

	/* match when query is empty */
	assert(has_match("", ""));
	assert(has_match("", "a"));

	return 0;
}

int test_scoring(){
	/* App/Models/Order is better than App/MOdels/foo  */
	assert(match("amo", "app/models/foo") < match("amo", "app/models/order"));

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

	return 0;
}

int test_positions_1(){
	size_t positions[3];
	match_positions("amo", "app/models/foo", positions);
	assert(positions[0] == 0);
	assert(positions[1] == 4);
	assert(positions[2] == 5);

	return 0;
}

int test_positions_2(){
	/*
	 * We should prefer matching the 'o' in order, since it's the beginning
	 * of a word.
	 */
	size_t positions[3];
	match_positions("amo", "app/models/order", positions);
	assert(positions[0] == 0);
	assert(positions[1] == 4);
	assert(positions[2] == 11);

	return 0;
}

int test_positions_3(){
	size_t positions[2];
	match_positions("as", "tags", positions);
	assert(positions[0] == 1);
	assert(positions[1] == 3);

	return 0;
}

int test_positions_4(){
	size_t positions[2];
	match_positions("as", "examples.txt", positions);
	assert(positions[0] == 2);
	assert(positions[1] == 7);

	return 0;
}

int test_positions_exact(){
	size_t positions[3];
	match_positions("foo", "foo", positions);
	assert(positions[0] == 0);
	assert(positions[1] == 1);
	assert(positions[2] == 2);

	return 0;
}

void summary(){
	printf("%i tests, %i assertions, %i failures\n", testsrun, assertionsrun, testsfailed);
}

int main(int argc, char *argv[]){
	(void) argc;
	(void) argv;

	runtest(test_match);
	runtest(test_scoring);
	runtest(test_positions_1);
	runtest(test_positions_2);
	runtest(test_positions_3);
	runtest(test_positions_4);
	runtest(test_positions_exact);

	summary();

	/* exit 0 if all tests pass */
	return !!testsfailed;
}
