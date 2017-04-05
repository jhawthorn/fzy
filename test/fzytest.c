#include "greatest/greatest.h"

SUITE(match_suite);
SUITE(choices_suite);
SUITE(properties_suite);

GREATEST_MAIN_DEFS();

int main(int argc, char *argv[]) {
	GREATEST_MAIN_BEGIN();

	RUN_SUITE(match_suite);
	RUN_SUITE(choices_suite);
	RUN_SUITE(properties_suite);

	GREATEST_MAIN_END();
}
