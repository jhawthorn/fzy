#include <stdio.h>
#include "fzy.h"

const char *testname;
int testsrun = 0, testspassed = 0;

#define TEST(name) int test_##name(){ testname = #name; testsrun++; do
#define ENDTEST while(0); testspassed++; return 0;}
#define assert(x) if(!(x)){fprintf(stderr, "test \"%s\" failed\n   assert(%s) was false\n   at %s:%i\n\n", testname, #x, __FILE__ ,__LINE__);return -1;}

TEST(match){
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
}ENDTEST

TEST(scoring){
    assert(match("amo", "app/models/foo") < match("amo", "app/models/order"));
}ENDTEST

void summary(){
	printf("%i tests run: %i passed  %i failed\n", testsrun, testspassed, testsrun - testspassed);
}

int main(int argc, char *argv[]){
	test_match();
	test_scoring();

	summary();

	/* exit 0 if all tests pass */
	return testsrun != testspassed;
}
