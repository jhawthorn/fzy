#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "match.h"
#include "tty_interface.h"
#include "config.h"

#ifndef PATH_MAX
# ifdef __linux__
#  define PATH_MAX 4096
# else
#  define PATH_MAX 1024
# endif /* __linux */
#endif /* PATH_MAX */

#define _ESC 27

/* Array to store selected/marked entries */
static char **selections = (char **)NULL;
/* A buffer big enough to hold decolored entries */
static char buf[PATH_MAX];

/* SEL_N is the current size of the selections array, while SEL_COUNTER
 * is the current amount of actually selected entries */
static size_t seln = 0, sel_counter = 0;

static char colors[COLOR_ITEMS_NUM][MAX_COLOR_LEN];
/* Parse colors taken from FZY_COLORS environment variable
 * Colors are parsed in strict order (see config.h)
 * Colors could be: 0-7 for normal colors, and b0-b7 for bold colors
 * Specific colors could be skipped using a dash ('-').
 * Colors are stored in the COLORS array using the same order defined in
 * config.h
 * These colors are applied in draw() and draw_match() functions in this file
 *
 * For example, "-b1b2-4" is read as follows:
 * -: no PROMPT color
 * b1: bold red POINTER color
 * b2: bold green MARKER color
 * -: no SELECTED ENTRY FOREGROUND color
 * 4: blue SELECTED ENTRY BACKGROUND color
 * */
static void
set_colors(void)
{
	char *p = getenv("NO_COLOR");
	if (p)
		return;

	p = getenv("FZY_COLORS");
	if (!p || !*p)
		p = DEFAULT_COLORS;

	size_t i, b = 0, c = 0;
	for (i = 0; p[i] && c < COLOR_ITEMS_NUM; i++) {
		if (p[i] == 'b') {
			b = 1;
			continue;
		}
		if ((p[i] < '0' || p[i] > '7') || p[i] == '-') {
			*colors[c] = '\0';
			b = 0;
			c++;
			continue;
		}
		/* 16 colors: 0-7 normal; b0-b7 bright */
		snprintf(colors[c], MAX_COLOR_LEN, "\x1b[%s%c%cm",
			b == 1 ? "1;" : "",
			c == SEL_BG_COLOR ? '4' : '3',
			p[i]);
		b = 0;
		c++;
	}
}

/* Search for the string P in the selections array. If found, return 1,
 * otherwise zero */
static int
is_selected(const char *p)
{
	if (!p || !*p || sel_counter == 0)
		return 0;

	size_t i;
	for (i = 0; selections[i]; i++) {
		if (*selections[i] == *p && strcmp(selections[i], p) == 0)
			return 1;
	}

	return 0;
}

/* Remote the entry NAME from the selections array by setting the first
 * byte of the corresponding array entry to NUL */
static void
deselect_entry(char *name)
{
	if (!name || !*name || sel_counter == 0)
		return;

	size_t i;
	for (i = 0; selections[i]; i++) {
		if (*selections[i] != *name || strcmp(selections[i], name) != 0)
			continue;
		*selections[i] = '\0';
		sel_counter--;
		break;
	}
}

static char *
decolor_name(const char *name)
{
	if (!name)
		return (char *)NULL;

	char *p = buf, *q = buf;

	size_t i, j = 0;
	size_t name_len = strlen(name);
	for (i = 0; name[i] && i < name_len; i++) {
		if (name[i] == _ESC && name[i + 1] == '[') {
			for (j = i + 1; name[j]; j++) {
				if (name[j] != 'm')
					continue;
				i = j + (name[j + 1] == _ESC ? 0 : 1);
				break;
			}
		}

		if (i == j) /* We have another escape code */
			continue;
		*p = name[i];
		p++;
	}

	*p = '\0';
	return *q ? q : (char *)NULL;
}

/* Save the string P into the selections array */
static void
save_selection(const char *p)
{
	selections = (char **)realloc(selections, (seln + 2) * sizeof(char *));
	selections[seln] = (char *)malloc((strlen(p) + 1) * sizeof(char));
	strcpy(selections[seln], p);
	seln++;
	sel_counter++;
	selections[seln] = (char *)NULL;
}

/* Select the currently highighted/hovered entry if not already selected.
 * Otherwise, remove it from the selections list */
static int
action_select(tty_interface_t *state)
{
	const char *p = choices_get(state->choices, state->choices->selection);
	if (!p)
		return EXIT_FAILURE;

	if (is_selected(p) == 1) {
		deselect_entry((char *)p);
		return EXIT_FAILURE;
	}

	save_selection(p);
	return EXIT_SUCCESS;
}

/* Print the list of selected/marked entries to STDOUT */
static void
print_selections(tty_interface_t *state)
{
	if (sel_counter == 0 || state->options->multi == 0)
		return;

	size_t i;
	for (i = 0; selections[i]; i++) {
		if (!*selections[i])
			continue;
		char *p = (char *)NULL;
		if (strchr(selections[i], _ESC))
			p = decolor_name(selections[i]);
		printf("%s\n", p ? p : selections[i]);
	}

}

/* Free the selections array */
static void
free_selections(tty_interface_t *state)
{
	if (state->options->multi == 0 || seln == 0 || !selections)
		return;

	size_t i;
	for (i = 0; selections[i]; i++)
		free(selections[i]);
	free(selections);
	selections = (char **)NULL;
}

static int
isprint_unicode(char c)
{
	return isprint(c) || c & (1 << 7);
}

static int
is_boundary(char c)
{
	return ~c & (1 << 7) || c & (1 << 6);
}

static void
clear(tty_interface_t *state)
{
	tty_t *tty = state->tty;

	tty_setcol(tty, state->options->pad);
	size_t line = 0;
	while (line++ < state->options->num_lines + (state->options->show_info ? 1 : 0))
		tty_newline(tty);

	tty_clearline(tty);
	if (state->options->num_lines > 0)
		tty_moveup(tty, line - 1);

	tty_flush(tty);
}

static void
draw_match(tty_interface_t *state, const char *choice, int selected)
{
	tty_t *tty = state->tty;
	options_t *options = state->options;
	char *search = state->last_search;

	int n = strlen(search);
	size_t positions[MATCH_MAX_LEN];
	for (int i = 0; i < n + 1 && i < MATCH_MAX_LEN; i++)
		positions[i] = -1;

	score_t score = match_positions(search, choice, &positions[0]);

	if (options->show_scores) {
		if (score == SCORE_MIN) {
			tty_printf(tty, "(     ) ");
		} else {
			tty_printf(tty, "(%5.2f) ", score);
		}
	}

	if (selected) {
#ifdef TTY_SELECTION_UNDERLINE
		tty_setunderline(tty);
#else
		/* Let's colorize the selected entry */
		if (*colors[SEL_FG_COLOR] || *colors[SEL_BG_COLOR]) {
			if (*colors[SEL_FG_COLOR])
				tty_printf(tty, "%s", colors[SEL_FG_COLOR]);
			if (*colors[SEL_BG_COLOR])
				tty_printf(tty, "%s", colors[SEL_BG_COLOR]);
		} else {
			tty_setinvert(tty);
		}
#endif
	}

	tty_setnowrap(tty);
	for (size_t i = 0, p = 0; choice[i] != '\0'; i++) {
		if (positions[p] == i) {
			tty_setfg(tty, TTY_COLOR_HIGHLIGHT);
			p++;
		} else {
			tty_setfg(tty, TTY_COLOR_NORMAL);
		}
		if (choice[i] == '\n')
			tty_putc(tty, ' ');
		else
			tty_printf(tty, "%c", choice[i]);
	}
	tty_setwrap(tty);
	tty_setnormal(tty);
}

static void
draw(tty_interface_t *state)
{
	tty_t *tty = state->tty;
	choices_t *choices = state->choices;
	options_t *options = state->options;

	unsigned int num_lines = options->num_lines;
	size_t start = 0;
	size_t current_selection = choices->selection;
	if (current_selection + options->scrolloff >= num_lines) {
		start = current_selection + options->scrolloff - num_lines + 1;
		size_t available = choices_available(choices);
		if (start + num_lines >= available && available > 0) {
			start = available - num_lines;
		}
	}

	if (options->reverse == 0) {
		tty_setcol(tty, options->pad);
		tty_printf(tty, "%s%s", options->prompt, state->search);
		tty_clearline(tty);

		if (options->show_info) {
			tty_printf(tty, "\n[%lu/%lu]", choices->available, choices->size);
			tty_clearline(tty);
		}
	}

	for (size_t i = start; i < start + num_lines; i++) {
		if (options->reverse == 0)
			tty_printf(tty, "\n");
		tty_clearline(tty);
		const char *choice = choices_get(choices, i);
		if (choice) {
			int multi_sel = options->multi == 1 && is_selected((char *)choice);
			tty_printf(tty, "%*s%s%c%s%c%s",
				options->pad, "", colors[POINTER_COLOR],
				i == choices->selection ? options->pointer : ' ',
				colors[MARKER_COLOR],
				multi_sel == 1 ? options->marker : ' ', NC);
			draw_match(state, choice, i == choices->selection);
		}
		if (options->reverse == 1)
			tty_printf(tty, "\n");
	}

	if (options->reverse == 0 && num_lines + options->show_info)
		tty_moveup(tty, num_lines + options->show_info);

	tty_setcol(tty, options->pad);
	tty_printf(tty, "%s%s%s", colors[PROMPT_COLOR], options->prompt, NC);
	for (size_t i = 0; i < state->cursor; i++)
		fputc(state->search[i], tty->fout);

	if (options->reverse == 0) {
		tty_flush(tty);
		return;
	}

	tty_setcol(tty, options->pad);
	tty_printf(tty, "%s%s", options->prompt, state->search);
	tty_clearline(tty);

	if (options->show_info) {
		tty_printf(tty, "\n[%lu/%lu]", choices->available, choices->size);
		tty_clearline(tty);
	}
	tty_flush(tty);
}

/*
static void
draw(tty_interface_t *state)
{
	tty_t *tty = state->tty;
	choices_t *choices = state->choices;
	options_t *options = state->options;

	unsigned int num_lines = options->num_lines;
	size_t start = 0;
	size_t current_selection = choices->selection;
	if (current_selection + options->scrolloff >= num_lines) {
		start = current_selection + options->scrolloff - num_lines + 1;
		size_t available = choices_available(choices);
		if (start + num_lines >= available && available > 0) {
			start = available - num_lines;
		}
	}

	if (options->reverse == 1) // Move to the bottom and print the prompt
		tty_printf(tty, "\x1b[%dB", num_lines);

	tty_setcol(tty, options->pad);
	tty_printf(tty, "%s%s", options->prompt, state->search);
	tty_clearline(tty);

	if (options->show_info) {
		tty_printf(tty, "\n[%lu/%lu]", choices->available, choices->size);
		tty_clearline(tty);
	}

	if (options->reverse == 1) // Go back to the top to print the files list
		tty_printf(tty, "\x1b[M\x1b[%dA", num_lines + 1);

	for (size_t i = start; i < start + num_lines; i++) {
		tty_printf(tty, "\n");
		tty_clearline(tty);
		const char *choice = choices_get(choices, i);
		if (choice) {
			int multi_sel = options->multi == 1 && is_selected((char *)choice);
			tty_printf(tty, "%*s%s%c%s%c%s",
				options->pad, "", colors[POINTER_COLOR],
				i == choices->selection ? options->pointer : ' ',
				colors[MARKER_COLOR],
				multi_sel == 1 ? options->marker : ' ', NC);
			draw_match(state, choice, i == choices->selection);
		}
	}

	if (options->reverse == 0 && num_lines + options->show_info)
		tty_moveup(tty, num_lines + options->show_info);

	if (options->reverse == 1)
		tty_printf(tty, "%c", '\n');

	tty_setcol(tty, options->pad);
	tty_printf(tty, "%s%s%s", colors[PROMPT_COLOR], options->prompt, NC);
	for (size_t i = 0; i < state->cursor; i++)
		fputc(state->search[i], tty->fout);
	tty_flush(tty);
} */

static void
update_search(tty_interface_t *state)
{
	choices_search(state->choices, state->search);
	strcpy(state->last_search, state->search);
}

static void
update_state(tty_interface_t *state)
{
	if (strcmp(state->last_search, state->search)) {
		update_search(state);
		if (state->options->reverse == 1)
			tty_printf(state->tty, "\x1b[%dA\n", state->options->num_lines + 1);
		draw(state);
	}
}

static void
action_emit(tty_interface_t *state)
{
	update_state(state);

	if (state->options->reverse == 1)
		tty_printf(state->tty, "\x1b[%dA\x1b[J", state->options->num_lines);

	if (state->options->multi == 1 && seln > 0) {
		clear(state);
		tty_close(state->tty);

		print_selections(state);
		free_selections(state);
		state->exit = EXIT_SUCCESS;
		return;
	}

	/* Reset the tty as close as possible to the previous state */
	clear(state);

	/* ttyout should be flushed before outputting on stdout */
	tty_close(state->tty);

	const char *selection = choices_get(state->choices, state->choices->selection);
	if (selection) { /* output the selected result */
		char *p = (char *)NULL;
		if (strchr(selection, _ESC))
			p = decolor_name(selection);
		printf("%s\n", p ? p : selection);
	} else { /* No match, output the query instead */
		printf("%s\n", state->search);
	}

	state->exit = EXIT_SUCCESS;
}

static void
action_del_char(tty_interface_t *state)
{
	if (state->cursor == 0)
		return;
	size_t length = strlen(state->search);
	size_t original_cursor = state->cursor;

	do {
		state->cursor--;
	} while (!is_boundary(state->search[state->cursor]) && state->cursor);

	memmove(&state->search[state->cursor], &state->search[original_cursor],
		length - original_cursor + 1);
}

static void
action_del_word(tty_interface_t *state)
{
	size_t original_cursor = state->cursor;
	size_t cursor = state->cursor;

	while (cursor && isspace(state->search[cursor - 1]))
		cursor--;

	while (cursor && !isspace(state->search[cursor - 1]))
		cursor--;

	memmove(&state->search[cursor], &state->search[original_cursor],
		strlen(state->search) - original_cursor + 1);
	state->cursor = cursor;
}

static void
action_del_all(tty_interface_t *state)
{
	memmove(state->search, &state->search[state->cursor],
		strlen(state->search) - state->cursor + 1);
	state->cursor = 0;
}

static void
action_prev(tty_interface_t *state)
{
	if (state->options->cycle == 0 && state->choices->selection == 0)
		return;
	update_state(state);
	choices_prev(state->choices);
}

static void
action_ignore(tty_interface_t *state)
{
	(void)state;
}

static void
action_next(tty_interface_t *state)
{
	if (state->options->cycle == 0
	&& state->choices->selection + 1 >= state->choices->available)
		return;
	update_state(state);
	choices_next(state->choices);
}

static void
action_exit(tty_interface_t *state)
{
	if (state->options->reverse == 1)
		tty_printf(state->tty, "\x1b[%dA\x1b[J", state->options->num_lines);

	clear(state);
	tty_close(state->tty);

	state->exit = EXIT_FAILURE;
}

static void
action_left(tty_interface_t *state)
{
	if (state->options->left_aborts == 1) {
		action_exit(state);
		return;
	}

	if (state->cursor > 0) {
		state->cursor--;
		while (!is_boundary(state->search[state->cursor]) && state->cursor)
			state->cursor--;
	}
}

static void
action_right(tty_interface_t *state)
{
	if (state->options->right_accepts == 1) {
		action_emit(state);
		return;
	}

	if (state->cursor < strlen(state->search)) {
		state->cursor++;
		while (!is_boundary(state->search[state->cursor]))
			state->cursor++;
	}
}

static void
action_beginning(tty_interface_t *state)
{
	state->cursor = 0;
}

static void
action_end(tty_interface_t *state)
{
	state->cursor = strlen(state->search);
}

static void
action_pageup(tty_interface_t *state)
{
	update_state(state);
	for (size_t i = 0; i < state->options->num_lines
	&& state->choices->selection > 0; i++)
		choices_prev(state->choices);
}

static void
action_pagedown(tty_interface_t *state)
{
	update_state(state);
	for (size_t i = 0; i < state->options->num_lines
	&& state->choices->selection < state->choices->available - 1; i++)
		choices_next(state->choices);
}

static void
action_tab(tty_interface_t *state)
{
	if (state->options->multi == 1) {
		action_select(state);
		action_next(state);
		return;
	}

	if (state->options->tab_accepts == 1) {
		action_emit(state);
		return;
	}

	/* Autocomplete */
	update_state(state);
	const char *current_selection = choices_get(state->choices,
		state->choices->selection);
	if (current_selection) {
		strncpy(state->search, choices_get(state->choices,
			state->choices->selection), SEARCH_SIZE_MAX);
		state->cursor = strlen(state->search);
	}
}

static void
append_search(tty_interface_t *state, char ch)
{
	char *search = state->search;
	size_t search_size = strlen(search);
	if (search_size < SEARCH_SIZE_MAX) {
		memmove(&search[state->cursor+1], &search[state->cursor],
			search_size - state->cursor + 1);
		search[state->cursor] = ch;

		state->cursor++;
	}
}

void
tty_interface_init(tty_interface_t *state, tty_t *tty, choices_t *choices, options_t *options)
{
	state->tty = tty;
	state->choices = choices;
	state->options = options;
	state->ambiguous_key_pending = 0;

	strcpy(state->input, "");
	strcpy(state->search, "");
	strcpy(state->last_search, "");

	state->exit = -1;

	if (options->init_search)
		strncpy(state->search, options->init_search, SEARCH_SIZE_MAX);

	state->cursor = strlen(state->search);

	update_search(state);
}

typedef struct {
	const char *key;
	void (*action)(tty_interface_t *);
} keybinding_t;

#define KEY_CTRL(key) ((const char[]){((key) - ('@')), '\0'})

static const keybinding_t keybindings[] = {
					   {"\x1b", action_exit},             /* ESC */
					   {"\x7f", action_del_char},	      /* DEL */
					   {KEY_CTRL('H'), action_del_char}, /* Backspace (C-H) */
					   {KEY_CTRL('W'), action_del_word}, /* C-W */
					   {KEY_CTRL('U'), action_del_all},  /* C-U */
					   {KEY_CTRL('I'), action_tab},      /* TAB (C-I ) */
					   {KEY_CTRL('C'), action_exit},	 /* C-C */
					   {KEY_CTRL('D'), action_exit},	 /* C-D */
					   {KEY_CTRL('G'), action_exit},	 /* C-G */
					   {KEY_CTRL('M'), action_emit},	 /* CR */
					   {KEY_CTRL('P'), action_prev},	 /* C-P */
					   {KEY_CTRL('N'), action_next},	 /* C-N */
					   {KEY_CTRL('K'), action_prev},	 /* C-K */
					   {KEY_CTRL('J'), action_next},	 /* C-J */
					   {KEY_CTRL('A'), action_beginning},    /* C-A */
					   {KEY_CTRL('E'), action_end},		 /* C-E */

					   {"\x1bOD", action_left}, /* LEFT */
					   {"\x1b[D", action_left}, /* LEFT */
					   {"\x1bOC", action_right}, /* RIGHT */
					   {"\x1b[C", action_right}, /* RIGHT */
					   {"\x1b[1~", action_beginning}, /* HOME */
					   {"\x1b[H", action_beginning}, /* HOME */
					   {"\x1b[4~", action_end}, /* END */
					   {"\x1b[F", action_end}, /* END */
					   {"\x1b[A", action_prev}, /* UP */
					   {"\x1bOA", action_prev}, /* UP */
					   {"\x1b[B", action_next}, /* DOWN */
					   {"\x1bOB", action_next}, /* DOWN */
					   {"\x1b[5~", action_pageup},
					   {"\x1b[6~", action_pagedown},
					   {"\x1b[200~", action_ignore},
					   {"\x1b[201~", action_ignore},
					   {NULL, NULL}};

#undef KEY_CTRL

static void
handle_input(tty_interface_t *state, const char *s, int handle_ambiguous_key)
{
	state->ambiguous_key_pending = 0;

	char *input = state->input;
	strcat(state->input, s);

	/* Figure out if we have completed a keybinding and whether we're in the
	 * middle of one (both can happen, because of Esc). */
	int found_keybinding = -1;
	int in_middle = 0;
	for (int i = 0; keybindings[i].key; i++) {
		if (!strcmp(input, keybindings[i].key))
			found_keybinding = i;
		else if (!strncmp(input, keybindings[i].key, strlen(state->input)))
			in_middle = 1;
	}

	/* If we have an unambiguous keybinding, run it.  */
	if (found_keybinding != -1 && (!in_middle || handle_ambiguous_key)) {
		keybindings[found_keybinding].action(state);
		strcpy(input, "");
		return;
	}

	/* We could have a complete keybinding, or could be in the middle of one.
	 * We'll need to wait a few milliseconds to find out. */
	if (found_keybinding != -1 && in_middle) {
		state->ambiguous_key_pending = 1;
		return;
	}

	/* Wait for more if we are in the middle of a keybinding */
	if (in_middle)
		return;

	/* No matching keybinding, decolorize and add to search */
	char *p = input, *q = (char *)NULL;
	if (strchr(input, _ESC) && (q = decolor_name(input)))
		p = q;

	for (int i = 0; p[i]; i++) {
		if (isprint_unicode(p[i]))
			append_search(state, p[i]);
	}

	/* We have processed the input, so clear it */
	strcpy(input, "");
}

int
tty_interface_run(tty_interface_t *state)
{
	if (state->options->no_color == 0)
		set_colors();
	draw(state);

	for (;;) {
		do {
			while(!tty_input_ready(state->tty, -1, 1)) {
				/* We received a signal (probably WINCH) */
				draw(state);
			}

			char s[2] = {tty_getchar(state->tty), '\0'};
			handle_input(state, s, 0);

			if (state->exit >= 0) {
				free_selections(state);
				return state->exit;
			}

			if (state->options->reverse == 1)
				tty_printf(state->tty, "\x1b[%dA\n", state->options->num_lines + 1);
			draw(state);
		} while (tty_input_ready(state->tty,
			state->ambiguous_key_pending ? KEYTIMEOUT : 0, 0));

		if (state->ambiguous_key_pending) {
			char s[1] = "";
			handle_input(state, s, 1);

			if (state->exit >= 0) {
				free_selections(state);
				return state->exit;
			}
		}

		update_state(state);
	}

	return state->exit;
}
