#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "options.h"

static const char *usage_str =
    ""
    "Usage: fzy [OPTION]...\n"
    " -l, --lines=LINES        Specify how many lines of results to show (default 10)\n"
    " -a, --all                Print all matched lines\n"
    " -p, --prompt=PROMPT      Input prompt (default '> ')\n"
    " -q, --query=QUERY        Use QUERY as the initial search string\n"
    " -e, --show-matches=QUERY Output the sorted matches of QUERY\n"
    " -t, --tty=TTY            Specify file to use as TTY device (default /dev/tty)\n"
    " -s, --show-scores        Show the scores of each match\n"
    " -j, --workers NUM        Use NUM workers for searching. (default is # of CPUs)\n"
    " -h, --help     Display this help and exit\n"
    " -v, --version  Output version information and exit\n";

static void usage(const char *argv0) {
	fprintf(stderr, usage_str, argv0);
}

static struct option longopts[] = {{"show-matches", required_argument, NULL, 'e'},
				   {"query", required_argument, NULL, 'q'},
				   {"lines", required_argument, NULL, 'l'},
				   {"all", no_argument, NULL, 'a'},
				   {"tty", required_argument, NULL, 't'},
				   {"prompt", required_argument, NULL, 'p'},
				   {"show-scores", no_argument, NULL, 's'},
				   {"version", no_argument, NULL, 'v'},
				   {"benchmark", optional_argument, NULL, 'b'},
				   {"workers", required_argument, NULL, 'j'},
				   {"help", no_argument, NULL, 'h'},
				   {NULL, 0, NULL, 0}};

void options_init(options_t *options) {
	/* set defaults */
	options->benchmark = 0;
	options->filter = NULL;
	options->init_search = NULL;
	options->tty_filename = "/dev/tty";
	options->show_scores = 0;
	options->num_lines = 10;
	options->output_all = 0;
	options->scrolloff = 1;
	options->prompt = "> ";
	options->workers = 0;
}

void options_parse(options_t *options, int argc, char *argv[]) {
	options_init(options);

	int c;
	while ((c = getopt_long(argc, argv, "vhsae:q:l:t:p:j:", longopts, NULL)) != -1) {
		switch (c) {
			case 'v':
				printf("%s " VERSION " © 2014-2018 John Hawthorn\n", argv[0]);
				exit(EXIT_SUCCESS);
			case 's':
				options->show_scores = 1;
				break;
			case 'q':
				options->init_search = optarg;
				break;
			case 'e':
				options->filter = optarg;
				break;
			case 'b':
				if (optarg) {
					if (sscanf(optarg, "%d", &options->benchmark) != 1) {
						usage(argv[0]);
						exit(EXIT_FAILURE);
					}
				} else {
					options->benchmark = 100;
				}
				break;
			case 't':
				options->tty_filename = optarg;
				break;
			case 'p':
				options->prompt = optarg;
				break;
			case 'j':
				if (sscanf(optarg, "%u", &options->workers) != 1) {
					usage(argv[0]);
					exit(EXIT_FAILURE);
				}
				break;
			case 'a':
				options->output_all = 1;
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
				options->num_lines = l;
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
}
