#ifndef CONFIGURATION_H
#define CONFIGURATION_H CONFIGURATION_H

#include "tty_interface.h"

typedef void (*action_t)(tty_interface_t *);

typedef struct {
	char *key;
	action_t action;
} keybinding_t;

typedef struct {
	keybinding_t *keybindings;
} configuration_t;

void configuration_init(configuration_t *configuration);
void configuration_free(configuration_t *configuration);

#endif
