#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>

#include "match.h"
#include "tty.h"
#include "choices.h"
#include "options.h"
#include "tty_interface.h"

#include "config.h"

int
main(int argc, char *argv[])
{
	int ret = 0;

	options_t options;
	options_parse(&options, argc, argv);

	choices_t choices;
	choices_init(&choices, &options);

	if (options.benchmark) {
		if (!options.filter) {
			fprintf(stderr, "Must specify -e/--show-matches with --benchmark\n");
			exit(EXIT_FAILURE);
		}
		choices_fread(&choices, stdin, options.input_delimiter);
		for (int i = 0; i < options.benchmark; i++)
			choices_search(&choices, options.filter);
	} else if (options.filter) {
		choices_fread(&choices, stdin, options.input_delimiter);
		choices_search(&choices, options.filter);
		for (size_t i = 0; i < choices_available(&choices); i++) {
			if (options.show_scores)
				printf("%f\t", choices_getscore(&choices, i));
			printf("%s\n", choices_get(&choices, i));
		}
	} else {
		/* interactive */

		if (isatty(STDIN_FILENO))
			choices_fread(&choices, stdin, options.input_delimiter);

		tty_t tty;
		tty_init(&tty, options.tty_filename);

		if (!isatty(STDIN_FILENO))
			choices_fread(&choices, stdin, options.input_delimiter);

		if (options.num_lines > choices.size)
			options.num_lines = choices.size;

		int num_lines_adjustment = 1;
		if (options.show_info)
			num_lines_adjustment++;

		if (options.num_lines + num_lines_adjustment > tty_getheight(&tty))
			options.num_lines = tty_getheight(&tty) - num_lines_adjustment;

		tty_interface_t tty_interface;
		tty_interface_init(&tty_interface, &tty, &choices, &options);
		ret = tty_interface_run(&tty_interface);
	}

	choices_destroy(&choices);

	return ret;
}
