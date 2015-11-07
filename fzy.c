#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>

#include "match.h"
#include "tty.h"
#include "choices.h"

#include "config.h"

int flag_show_scores = 0;

size_t num_lines = 10;
size_t scrolloff = 1;

const char *prompt = "> ";

void read_choices(choices_t *c) {
	const char *line;
	char buf[4096];
	while (fgets(buf, sizeof buf, stdin)) {
		char *nl;
		if ((nl = strchr(buf, '\n')))
			*nl = '\0';

		if (!(line = strdup(buf))) {
			fprintf(stderr, "Cannot allocate memory");
			abort();
		}
		choices_add(c, line);
	}
}

#define SEARCH_SIZE_MAX 4096
int search_size;
char search[SEARCH_SIZE_MAX + 1] = {0};

void clear(tty_t *tty) {
	tty_setcol(tty, 0);
	size_t line = 0;
	while (line++ < num_lines) {
		tty_newline(tty);
	}
	tty_moveup(tty, line - 1);
	tty_flush(tty);
}

void draw_match(tty_t *tty, const char *choice, int selected) {
	int n = strlen(search);
	size_t positions[n + 1];
	for (int i = 0; i < n + 1; i++)
		positions[i] = -1;

	double score = match_positions(search, choice, &positions[0]);

	size_t maxwidth = tty_getwidth(tty);

	if (flag_show_scores)
		tty_printf(tty, "(%5.2f) ", score);

	if (selected)
		tty_setinvert(tty);

	for (size_t i = 0, p = 0; choice[i] != '\0'; i++) {
		if (i + 1 < maxwidth) {
			if (positions[p] == i) {
				tty_setfg(tty, TTY_COLOR_HIGHLIGHT);
				p++;
			} else {
				tty_setfg(tty, TTY_COLOR_NORMAL);
			}
			tty_printf(tty, "%c", choice[i]);
		} else {
			tty_printf(tty, "$");
			break;
		}
	}
	tty_setnormal(tty);
}

void draw(tty_t *tty, choices_t *choices) {
	size_t start = 0;
	size_t current_selection = choices->selection;
	if (current_selection + scrolloff >= num_lines) {
		start = current_selection + scrolloff - num_lines + 1;
		if (start + num_lines >= choices_available(choices)) {
			start = choices_available(choices) - num_lines;
		}
	}
	tty_setcol(tty, 0);
	tty_printf(tty, "%s%s", prompt, search);
	tty_clearline(tty);
	for (size_t i = start; i < start + num_lines; i++) {
		tty_printf(tty, "\n");
		tty_clearline(tty);
		const char *choice = choices_get(choices, i);
		if (choice) {
			draw_match(tty, choice, i == choices->selection);
		}
	}
	tty_moveup(tty, num_lines);
	tty_setcol(tty, strlen(prompt) + strlen(search));
	tty_flush(tty);
}

void emit(choices_t *choices) {
	const char *selection = choices_get(choices, choices->selection);
	if (selection) {
		/* output the selected result */
		printf("%s\n", selection);
	} else {
		/* No match, output the query instead */
		printf("%s\n", search);
	}

	exit(EXIT_SUCCESS);
}

void run(tty_t *tty, choices_t *choices) {
	choices_search(choices, search);
	char ch;
	do {
		draw(tty, choices);
		ch = tty_getchar(tty);
		if (isprint(ch)) {
			if (search_size < SEARCH_SIZE_MAX) {
				search[search_size++] = ch;
				search[search_size] = '\0';
				choices_search(choices, search);
			}
		} else if (ch == 127 || ch == 8) { /* DEL || backspace */
			if (search_size)
				search[--search_size] = '\0';
			choices_search(choices, search);
		} else if (ch == 21) { /* C-U */
			search_size = 0;
			search[0] = '\0';
			choices_search(choices, search);
		} else if (ch == 23) { /* C-W */
			if (search_size)
				search[--search_size] = '\0';
			while (search_size && !isspace(search[--search_size]))
				search[search_size] = '\0';
			choices_search(choices, search);
		} else if (ch == 14) { /* C-N */
			choices_next(choices);
		} else if (ch == 16) { /* C-P */
			choices_prev(choices);
		} else if (ch == 9) { /* TAB */
			strncpy(search, choices_get(choices, choices->selection), SEARCH_SIZE_MAX);
			search_size = strlen(search);
			choices_search(choices, search);
		} else if (ch == 3 || ch == 4) { /* ^C || ^D */
			clear(tty);
			tty_close(tty);
			exit(EXIT_FAILURE);
		} else if (ch == 10) { /* Enter */
			clear(tty);

			/* ttyout should be flushed before outputting on stdout */
			tty_close(tty);

			emit(choices);
		} else if (ch == 27) { /* ESC */
			ch = tty_getchar(tty);
			if (ch == '[' || ch == 'O') {
				ch = tty_getchar(tty);
				if (ch == 'A') { /* UP ARROW */
					choices_prev(choices);
				} else if (ch == 'B') { /* DOWN ARROW */
					choices_next(choices);
				}
			}
		}
	} while (1);
}

static const char *usage_str =
    ""
    "Usage: fzy [OPTION]...\n"
    " -l, --lines=LINES        Specify how many lines of results to show (default 10)\n"
    " -p, --prompt=PROMPT      Input prompt (default '> ')\n"
    " -e, --show-matches=QUERY Output the sorted matches of QUERY\n"
    " -t, --tty=TTY            Specify file to use as TTY device (default /dev/tty)\n"
    " -s, --show-scores        Show the scores of each match\n"
    " -h, --help     Display this help and exit\n"
    " -v, --version  Output version information and exit\n";

void usage(const char *argv0) {
	fprintf(stderr, usage_str, argv0);
}

static struct option longopts[] = {{"show-matches", required_argument, NULL, 'e'},
				   {"lines", required_argument, NULL, 'l'},
				   {"tty", required_argument, NULL, 't'},
				   {"prompt", required_argument, NULL, 'p'},
				   {"show-scores", no_argument, NULL, 's'},
				   {"version", no_argument, NULL, 'v'},
				   {"benchmark", no_argument, NULL, 'b'},
				   {"help", no_argument, NULL, 'h'},
				   {NULL, 0, NULL, 0}};

int main(int argc, char *argv[]) {
	int benchmark = 0;
	const char *initial_query = NULL;
	const char *tty_filename = "/dev/tty";
	char c;
	while ((c = getopt_long(argc, argv, "vhse:l:t:p:", longopts, NULL)) != -1) {
		switch (c) {
			case 'v':
				printf("%s " VERSION " (c) 2014 John Hawthorn\n", argv[0]);
				exit(EXIT_SUCCESS);
			case 's':
				flag_show_scores = 1;
				break;
			case 'e':
				initial_query = optarg;
				break;
			case 'b':
				benchmark = 1;
				break;
			case 't':
				tty_filename = optarg;
				break;
			case 'p':
				prompt = optarg;
				break;
			case 'l': {
				int l;
				if (!strcmp(optarg, "max")) {
					l = INT_MAX;
				} else if (sscanf(optarg, "%d", &l) != 1 || l < 3) {
					fprintf(stderr, "Invalid format for --lines: %s\n", optarg);
					fprintf(stderr, "Must be integer in range 3..\n");
					usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				num_lines = l;
			} break;
			case 'h':
			default:
				usage(argv[0]);
				exit(EXIT_SUCCESS);
		}
	}
	if (optind != argc) {
		usage(argv[0]);
		exit(EXIT_FAILURE);
	}

	choices_t choices;
	choices_init(&choices);
	read_choices(&choices);

	if (num_lines > choices.size)
		num_lines = choices.size;

	if (benchmark) {
		if (!initial_query) {
			fprintf(stderr, "Must specify -e/--show-matches with --benchmark\n");
			exit(EXIT_FAILURE);
		}
		for (int i = 0; i < 100; i++)
			choices_search(&choices, initial_query);
	} else if (initial_query) {
		choices_search(&choices, initial_query);
		for (size_t i = 0; i < choices_available(&choices); i++) {
			if (flag_show_scores)
				printf("%f\t", choices_getscore(&choices, i));
			printf("%s\n", choices_get(&choices, i));
		}
	} else {
		/* interactive */
		tty_t tty;
		tty_init(&tty, tty_filename);

		if (num_lines + 1 > tty_getheight(&tty))
			num_lines = tty_getheight(&tty) - 1;

		run(&tty, &choices);
	}

	return 0;
}
