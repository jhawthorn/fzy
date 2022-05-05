#ifndef OPTIONS_H
#define OPTIONS_H OPTIONS_H

typedef struct {
	int benchmark;
	const char *filter;
	const char *init_search;
	const char *tty_filename;
	int show_scores;
	unsigned int num_lines;
	unsigned int scrolloff;
	const char *prompt;
	unsigned int workers;
	char input_delimiter;
	int show_info;
	int pad;
	int multi;
	char pointer;
	char marker;
	int cycle;
	int tab_accepts;
	int right_accepts;
	int left_aborts;
	int no_color;
	int reverse;
} options_t;

void options_init(options_t *options);
void options_parse(options_t *options, int argc, char *argv[]);

#endif
