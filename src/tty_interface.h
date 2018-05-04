#ifndef TTY_INTERFACE_H
#define TTY_INTERFACE_H TTY_INTERFACE_H

#include "choices.h"
#include "options.h"
#include "tty.h"

#define SEARCH_SIZE_MAX 4096

typedef struct {
	tty_t *tty;
	choices_t *choices;
	options_t *options;

	char search[SEARCH_SIZE_MAX + 1];
	char last_search[SEARCH_SIZE_MAX + 1];
	size_t cursor;

	int ambiguous_key_pending;
	char input[32]; /* Pending input buffer */

	int exit;
} tty_interface_t;

void tty_interface_init(tty_interface_t *state, tty_t *tty, choices_t *choices, options_t *options);
int tty_interface_run(tty_interface_t *state);

#endif
