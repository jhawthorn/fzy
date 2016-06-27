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
	char *search = state->last_search;

	int n = strlen(search);
	size_t positions[n + 1];
	for (int i = 0; i < n + 1; i++)
		positions[i] = -1;

	score_t score = match_positions(search, choice, &positions[0]);

	size_t maxwidth = tty_getwidth(tty);

	if (options->show_scores) {
		if (score == SCORE_MIN) {
			tty_printf(tty, "(     ) ");
		} else {
			tty_printf(tty, "(%5.2f) ", score);
		}
	}

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

static void update_search(tty_interface_t *state) {
	choices_search(state->choices, state->search);
	strcpy(state->last_search, state->search);
}

void update_state(tty_interface_t *state) {
	if (strcmp(state->last_search, state->search)) {
		update_search(state);
		draw(state);
	}
}

static void action_emit(tty_interface_t *state) {
	update_state(state);

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
	update_state(state);
	choices_prev(state->choices);
}

static void action_next(tty_interface_t *state) {
	update_state(state);
	choices_next(state->choices);
}

static void action_pageup(tty_interface_t *state) {
	update_state(state);
	for(size_t i = 0; i < state->options->num_lines && state->choices->selection > 0; i++)
		choices_prev(state->choices);
}

static void action_pagedown(tty_interface_t *state) {
	update_state(state);
	for(size_t i = 0; i < state->options->num_lines && state->choices->selection < state->choices->available-1; i++)
		choices_next(state->choices);
}

static void action_autocomplete(tty_interface_t *state) {
	update_state(state);
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

void tty_interface_init(tty_interface_t *state, tty_t *tty, choices_t *choices, options_t *options) {
	state->tty = tty;
	state->choices = choices;
	state->options = options;

	strcpy(state->input, "");
	strcpy(state->search, "");
	strcpy(state->last_search, "");

	state->exit = -1;

	if (options->init_search)
		strncpy(state->search, options->init_search, SEARCH_SIZE_MAX);

	update_search(state);
}

typedef struct {
	const char *key;
	void (*action)(tty_interface_t *);
} keybinding_t;

#define KEY_CTRL(key) ((const char[]){((key) - ('@')), '\0'})

static const keybinding_t keybindings[] = {{"\x7f", action_del_char},	/* DEL */
					   {KEY_CTRL('H'), action_del_char}, /* Backspace (C-H) */
					   {KEY_CTRL('W'), action_del_word}, /* C-W */
					   {KEY_CTRL('U'), action_del_all},  /* C-U */
					   {KEY_CTRL('I'), action_autocomplete}, /* TAB (C-I ) */
					   {KEY_CTRL('C'), action_exit},	 /* C-C */
					   {KEY_CTRL('D'), action_exit},	 /* C-D */
					   {KEY_CTRL('M'), action_emit},	 /* CR */
					   {KEY_CTRL('P'), action_prev},	 /* C-P */
					   {KEY_CTRL('N'), action_next},	 /* C-N */

					   {"\x1b[A", action_prev}, /* UP */
					   {"\x1bOA", action_prev}, /* UP */
					   {"\x1b[B", action_next}, /* DOWN */
					   {"\x1bOB", action_next}, /* DOWN */
					   {"\x1b[5~", action_pageup},
					   {"\x1b[6~", action_pagedown},
					   {NULL, NULL}};

#undef KEY_CTRL

void handle_input(tty_interface_t *state, const char *s) {
	char *input = state->input;
	strcat(state->input, s);

	/* See if we have matched a keybinding */
	for (int i = 0; keybindings[i].key; i++) {
		if (!strcmp(input, keybindings[i].key)) {
			keybindings[i].action(state);
			strcpy(input, "");
			return;
		}
	}

	/* Check if we are in the middle of a keybinding */
	for (int i = 0; keybindings[i].key; i++)
		if (!strncmp(input, keybindings[i].key, strlen(input)))
			return;

	/* No matching keybinding, add to search */
	for (int i = 0; input[i]; i++)
		if (isprint(input[i]))
			append_search(state, input[i]);

	/* We have processed the input, so clear it */
	strcpy(input, "");
}

int tty_interface_run(tty_interface_t *state) {
	draw(state);

	for (;;) {
		do {
			char s[2] = {tty_getchar(state->tty), '\0'};
			handle_input(state, s);

			if (state->exit >= 0)
				return state->exit;

			draw(state);
		} while (tty_input_ready(state->tty));

		update_state(state);
	}

	return state->exit;
}
