#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "match.h"
#include "tty_interface.h"
#include "../config.h"

static void clear(tty_interface_t *state) {
	tty_t *tty = state->tty;

	tty_setcol(tty, 0);
	size_t line = 0;
	while (line++ < state->options->num_lines) {
		tty_newline(tty);
	}
	tty_clearline(tty);
	tty_moveup(tty, line - 1);
	tty_flush(tty);
}

static void draw_match(tty_interface_t *state, const char *choice, int selected) {
	tty_t *tty = state->tty;
	options_t *options = state->options;
	char *search = state->search;

	int n = strlen(search);
	size_t positions[n + 1];
	for (int i = 0; i < n + 1; i++)
		positions[i] = -1;

	double score = match_positions(search, choice, &positions[0]);

	size_t maxwidth = tty_getwidth(tty);

	if (options->show_scores)
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

static void draw(tty_interface_t *state) {
	tty_t *tty = state->tty;
	choices_t *choices = state->choices;
	options_t *options = state->options;

	unsigned int num_lines = options->num_lines;
	size_t start = 0;
	size_t current_selection = choices->selection;
	if (current_selection + options->scrolloff >= num_lines) {
		start = current_selection + options->scrolloff - num_lines + 1;
		if (start + num_lines >= choices_available(choices)) {
			start = choices_available(choices) - num_lines;
		}
	}
	tty_setcol(tty, 0);
	tty_printf(tty, "%s%s", options->prompt, state->search);
	tty_clearline(tty);
	for (size_t i = start; i < start + num_lines; i++) {
		tty_printf(tty, "\n");
		tty_clearline(tty);
		const char *choice = choices_get(choices, i);
		if (choice) {
			draw_match(state, choice, i == choices->selection);
		}
	}
	tty_moveup(tty, num_lines);
	tty_setcol(tty, strlen(options->prompt) + strlen(state->search));
	tty_flush(tty);
}

static void action_emit(tty_interface_t *state) {
	/* Reset the tty as close as possible to the previous state */
	clear(state);

	/* ttyout should be flushed before outputting on stdout */
	tty_close(state->tty);

	const char *selection = choices_get(state->choices, state->choices->selection);
	if (selection) {
		/* output the selected result */
		printf("%s\n", selection);
	} else {
		/* No match, output the query instead */
		printf("%s\n", state->search);
	}

	state->exit = EXIT_SUCCESS;
}

static void action_del_char(tty_interface_t *state) {
	if (*state->search)
		state->search[strlen(state->search) - 1] = '\0';
}

static void action_del_word(tty_interface_t *state) {
	size_t search_size = strlen(state->search);
	if (search_size)
		state->search[--search_size] = '\0';
	while (search_size && !isspace(state->search[--search_size]))
		state->search[search_size] = '\0';
}

static void action_del_all(tty_interface_t *state) {
	strcpy(state->search, "");
}

static void action_prev(tty_interface_t *state) {
	choices_prev(state->choices);
}

static void action_next(tty_interface_t *state) {
	choices_next(state->choices);
}

static void action_autocomplete(tty_interface_t *state) {
	strncpy(state->search, choices_get(state->choices, state->choices->selection), SEARCH_SIZE_MAX);
}

static void action_exit(tty_interface_t *state) {
	clear(state);
	tty_close(state->tty);

	state->exit = EXIT_FAILURE;
}

static void append_search(tty_interface_t *state, char ch) {
	char *search = state->search;
	size_t search_size = strlen(search);
	if (search_size < SEARCH_SIZE_MAX) {
		search[search_size++] = ch;
		search[search_size] = '\0';
	}
}

#define KEY_CTRL(key) ((key) - ('@'))
#define KEY_DEL 127
#define KEY_ESC 27

static void update_search(tty_interface_t *state) {
	choices_search(state->choices, state->search);
	strcpy(state->last_search, state->search);
}

void update_state(tty_interface_t *state) {
	if (strcmp(state->last_search, state->search))
		update_search(state);
}

void tty_interface_init(tty_interface_t *state, tty_t *tty, choices_t *choices, options_t *options) {
	state->tty = tty;
	state->choices = choices;
	state->options = options;

	strcpy(state->search, "");
	strcpy(state->last_search, "");

	state->exit = -1;

	if (options->init_search)
		strncpy(state->search, options->init_search, SEARCH_SIZE_MAX);

	update_search(state);
}

int tty_interface_run(tty_interface_t *state) {
	tty_t *tty = state->tty;

	char ch;
	while (state->exit < 0) {
		draw(state);
		ch = tty_getchar(tty);
		if (isprint(ch)) {
			append_search(state, ch);
		} else if (ch == KEY_DEL || ch == KEY_CTRL('H')) { /* DEL || Backspace (C-H) */
			action_del_char(state);
		} else if (ch == KEY_CTRL('U')) { /* C-U */
			action_del_all(state);
		} else if (ch == KEY_CTRL('W')) { /* C-W */
			action_del_word(state);
		} else if (ch == KEY_CTRL('N')) { /* C-N */
			action_next(state);
		} else if (ch == KEY_CTRL('P')) { /* C-P */
			action_prev(state);
		} else if (ch == KEY_CTRL('I')) { /* TAB (C-I) */
			action_autocomplete(state);
		} else if (ch == KEY_CTRL('C') || ch == KEY_CTRL('D')) { /* ^C || ^D */
			action_exit(state);
		} else if (ch == KEY_CTRL('M')) { /* CR */
			action_emit(state);
		} else if (ch == KEY_ESC) { /* ESC */
			ch = tty_getchar(tty);
			if (ch == '[' || ch == 'O') {
				ch = tty_getchar(tty);
				if (ch == 'A') { /* UP ARROW */
					action_prev(state);
				} else if (ch == 'B') { /* DOWN ARROW */
					action_next(state);
				}
			}
		}
		update_state(state);
	}

	return state->exit;
}
