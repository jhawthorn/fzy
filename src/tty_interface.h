#ifndef TTY_INTERFACE_H
#define TTY_INTERFACE_H TTY_INTERFACE_H

#include "choices.h"
#include "options.h"
#include "tty.h"

#define SEARCH_SIZE_MAX 4096

void tty_interface_run(tty_t *tty, choices_t *choices, options_t *options);

#endif
