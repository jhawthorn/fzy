#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include <limits.h>

#include "match.h"
#include "tty.h"
#include "choices.h"

#include "../config.h"

static int flag_show_scores = 0;

static size_t num_lines = 10;
static size_t scrolloff = 1;

static const char *prompt = "> ";

#define SEARCH_SIZE_MAX 4096
static char search[SEARCH_SIZE_MAX + 1] = {0};

static void clear(tty_t *tty) {
	tty_setcol(tty, 0);
	size_t line = 0;
	while (line++ < num_lines) {
		tty_newline(tty);
	}
	tty_clearline(tty);
	tty_moveup(tty, line - 1);
	tty_flush(tty);
}

static void draw_match(tty_t *tty, const char *choice, int selected) {
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

static void draw(tty_t *tty, choices_t *choices) {
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

static void emit(choices_t *choices) {
	const char *selection = choices_get(choices, choices->selection);
	if (selection) {
		/* output the selected result */
		printf("%s\n", selection);
	} else {
		/* No match, output the query instead */
		printf("%s\n", search);
	}
}

#define KEY_CTRL(key) ((key) - ('@'))
#define KEY_DEL 127
#define KEY_ESC 27

static void run(tty_t *tty, choices_t *choices) {
	choices_search(choices, search);
	char ch;
	do {
		draw(tty, choices);
		ch = tty_getchar(tty);
		size_t search_size = strlen(search);
		if (isprint(ch)) {
			if (search_size < SEARCH_SIZE_MAX) {
				search[search_size++] = ch;
				search[search_size] = '\0';
				choices_search(choices, search);
			}
		} else if (ch == KEY_DEL || ch == KEY_CTRL('H')) { /* DEL || Backspace (C-H) */
			if (search_size)
				search[--search_size] = '\0';
			choices_search(choices, search);
		} else if (ch == KEY_CTRL('U')) { /* C-U */
			search_size = 0;
			search[0] = '\0';
			choices_search(choices, search);
		} else if (ch == KEY_CTRL('W')) { /* C-W */
			if (search_size)
				search[--search_size] = '\0';
			while (search_size && !isspace(search[--search_size]))
				search[search_size] = '\0';
			choices_search(choices, search);
		} else if (ch == KEY_CTRL('N')) { /* C-N */
			choices_next(choices);
		} else if (ch == KEY_CTRL('P')) { /* C-P */
			choices_prev(choices);
		} else if (ch == KEY_CTRL('I')) { /* TAB (C-I) */
			strncpy(search, choices_get(choices, choices->selection), SEARCH_SIZE_MAX);
			choices_search(choices, search);
		} else if (ch == KEY_CTRL('C') || ch == KEY_CTRL('D')) { /* ^C || ^D */
			clear(tty);
			tty_close(tty);
			exit(EXIT_FAILURE);
		} else if (ch == KEY_CTRL('M')) { /* CR */
			clear(tty);

			/* ttyout should be flushed before outputting on stdout */
			tty_close(tty);

			emit(choices);

			/* Return to eventually exit successfully */
			return;
		} else if (ch == KEY_ESC) { /* ESC */
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
    " -q, --query=QUERY        Use QUERY as the initial search string\n"
    " -e, --show-matches=QUERY Output the sorted matches of QUERY\n"
    " -t, --tty=TTY            Specify file to use as TTY device (default /dev/tty)\n"
    " -s, --show-scores        Show the scores of each match\n"
    " -h, --help     Display this help and exit\n"
    " -v, --version  Output version information and exit\n";

static void usage(const char *argv0) {
	fprintf(stderr, usage_str, argv0);
}

static struct option longopts[] = {{"show-matches", required_argument, NULL, 'e'},
				   {"query", required_argument, NULL, 'q'},
				   {"lines", required_argument, NULL, 'l'},
				   {"tty", required_argument, NULL, 't'},
				   {"prompt", required_argument, NULL, 'p'},
				   {"show-scores", no_argument, NULL, 's'},
				   {"version", no_argument, NULL, 'v'},
				   {"benchmark", optional_argument, NULL, 'b'},
				   {"help", no_argument, NULL, 'h'},
				   {NULL, 0, NULL, 0}};

int main(int argc, char *argv[]) {
	int benchmark = 0;
	const char *filter = NULL;
	const char *tty_filename = "/dev/tty";
	char c;
	while ((c = getopt_long(argc, argv, "vhse:q:l:t:p:", longopts, NULL)) != -1) {
		switch (c) {
			case 'v':
				printf("%s " VERSION " (c) 2014 John Hawthorn\n", argv[0]);
				exit(EXIT_SUCCESS);
			case 's':
				flag_show_scores = 1;
				break;
			case 'q':
				strncpy(search, optarg, SEARCH_SIZE_MAX);
				break;
			case 'e':
				filter = optarg;
				break;
			case 'b':
				if (optarg) {
					if (sscanf(optarg, "%d", &benchmark) != 1) {
						usage(argv[0]);
						exit(EXIT_FAILURE);
					}
				} else {
					benchmark = 100;
				}
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
	choices_fread(&choices, stdin);

	if (benchmark) {
		if (!filter) {
			fprintf(stderr, "Must specify -e/--show-matches with --benchmark\n");
			exit(EXIT_FAILURE);
		}
		for (int i = 0; i < benchmark; i++)
			choices_search(&choices, filter);
	} else if (filter) {
		choices_search(&choices, filter);
		for (size_t i = 0; i < choices_available(&choices); i++) {
			if (flag_show_scores)
				printf("%f\t", choices_getscore(&choices, i));
			printf("%s\n", choices_get(&choices, i));
		}
	} else {
		/* interactive */
		tty_t tty;
		tty_init(&tty, tty_filename);

		if (num_lines > choices.size)
			num_lines = choices.size;

		if (num_lines + 1 > tty_getheight(&tty))
			num_lines = tty_getheight(&tty) - 1;

		run(&tty, &choices);
	}

	choices_destroy(&choices);

	return 0;
}
