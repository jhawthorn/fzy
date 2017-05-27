#include <stdlib.h>
#ifdef WITH_LIBCONFIG
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <libconfig.h>
#include <basedir_fs.h>
#endif

#include "configuration.h"
#include "memory.h"

#define KEY_CTRL(key) ((const char[]){((key) - ('@')), '\0'})

typedef struct {
	const char *key;
	void (*action)(tty_interface_t *);
} indexed_action_t;

indexed_action_t default_keybindings[] = {
	{"\x7f", action_del_char},	/* DEL */
	{KEY_CTRL('H'), action_del_char}, /* Backspace (C-H) */
	{KEY_CTRL('W'), action_del_word}, /* C-W */
	{KEY_CTRL('U'), action_del_all},  /* C-U */
	{KEY_CTRL('I'), action_autocomplete}, /* TAB (C-I ) */
	{KEY_CTRL('C'), action_exit},	 /* C-C */
	{KEY_CTRL('D'), action_exit},	 /* C-D */
	{KEY_CTRL('M'), action_emit},	 /* CR */
	{KEY_CTRL('P'), action_prev},	 /* C-P */
	{KEY_CTRL('N'), action_next},	 /* C-N */
	{KEY_CTRL('K'), action_prev},	 /* C-J */
	{KEY_CTRL('J'), action_next},	 /* C-K */
	{"\x1b[A", action_prev}, /* UP */
	{"\x1bOA", action_prev}, /* UP */
	{"\x1b[B", action_next}, /* DOWN */
	{"\x1bOB", action_next}, /* DOWN */
	{"\x1b[5~", action_pageup},
	{"\x1b[6~", action_pagedown}
};

#ifdef WITH_LIBCONFIG
indexed_action_t actions[] = {
	{"emit", action_emit},
	{"emit all", action_emit_all},
	{"del char", action_del_char},
	{"del word", action_del_word},
	{"del all", action_del_all},
	{"prev", action_prev},
	{"next", action_next},
	{"page up", action_pageup},
	{"page down", action_pagedown},
	{"autocomplete", action_autocomplete},
	{"exit", action_exit}
};

static action_t action_by_name(char const *name) {
	const size_t action_count = sizeof(actions) / sizeof(indexed_action_t);
	for (size_t i = 0; i < action_count; ++i) {
		if (! strcmp(actions[i].key, name)) {
			return actions[i].action;
		}
	}

	fprintf(stderr, "action named `%s' does not exist\n", name);
	exit(1);
}

static bool insert_keybinding(keybinding_t *keybindings, size_t keybinding_count, const char *key, action_t action) {
	for (size_t i = 0; i < keybinding_count; ++i) {
		if (! strcmp(keybindings[i].key, key)) {
			keybindings[i].action = action;
			return false;
		}
	}

	keybindings[keybinding_count].key = safe_strdup(key);
	keybindings[keybinding_count].action = action;
	return true;
}

static size_t process_new_keybinding(keybinding_t *keybindings, size_t keybinding_count, char const *key, char const *value) {
	action_t action = action_by_name(value);

	if (strlen(key) == 3 && (key[0] == 'c' || key[0] == 'C') && key[1] == '-') {
		const char letter = toupper(key[2]);
		if (letter < 'A' || letter > 'Z') {
			fprintf(stderr, "%s control code invalid, must be a letter from a to z\n", key);
			exit(1);
		}

		char ctrl_code[2];
		ctrl_code[0] = letter - '@';
		ctrl_code[1] = '\0';
		if (insert_keybinding(keybindings, keybinding_count, ctrl_code, action)) {
			++keybinding_count;
		}
	}
	else if (! strcmp(key, "up")) {
		if (insert_keybinding(keybindings, keybinding_count, "\x1b[A", action)) {
			++keybinding_count;
		}
		if (insert_keybinding(keybindings, keybinding_count, "\x1bOA", action)) {
			++keybinding_count;
		}
	}
	else if (! strcmp(key, "down")) {
		if (insert_keybinding(keybindings, keybinding_count, "\x1b[B", action)) {
			++keybinding_count;
		}
		if (insert_keybinding(keybindings, keybinding_count, "\x1bOB", action)) {
			++keybinding_count;
		}
	}
	else if (! strcmp(key, "page-up")) {
		if (insert_keybinding(keybindings, keybinding_count, "\x1b[5~", action)) {
			++keybinding_count;
		}
	}
	else if (! strcmp(key, "page-down")) {
		if (insert_keybinding(keybindings, keybinding_count, "\x1b[6~", action)) {
			++keybinding_count;
		}
	}
	else if (! strcmp(key, "delete")) {
		if (insert_keybinding(keybindings, keybinding_count, "\x7f", action)) {
			++keybinding_count;
		}
	}
	else if (! strcmp(key, "tab")) {
		if (insert_keybinding(keybindings, keybinding_count, KEY_CTRL('I'), action)) {
			++keybinding_count;
		}
	}
	else {
			fprintf(stderr, "%s it not a valid keybinding\n", key);
			exit(1);
	}

	return keybinding_count;
}
#endif

void configuration_init(configuration_t *configuration) {
	size_t keybindings_count = sizeof(default_keybindings) / sizeof(indexed_action_t);

	configuration->keybindings = (keybinding_t *) safe_realloc(NULL, (keybindings_count + 1) * sizeof(keybinding_t));
	for (size_t i = 0; i < keybindings_count; i++) {
		keybinding_t *binding = configuration->keybindings + i;
		binding->key = safe_strdup(default_keybindings[i].key);
		binding->action = default_keybindings[i].action;
	}

#ifdef WITH_LIBCONFIG
	const char *fzy_cfg_path = xdgConfigFind("fzy", NULL);
	if (strlen(fzy_cfg_path) > 0) {
		config_t cfg;
		config_init(&cfg);
		if (!config_read_file(&cfg, fzy_cfg_path)) {
			fprintf(stderr, "could not read config %s:%d - %s\n", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
			exit(1);
		}

		config_setting_t *setting = config_lookup(&cfg, "key-bindings");
		if (setting) {
			int setting_count = config_setting_length(setting);
			configuration->keybindings = (keybinding_t *) safe_realloc(
				configuration->keybindings,
				(keybindings_count + setting_count + 1) * sizeof(keybinding_t)
			);

			for (int i = 0; i < setting_count; ++i) {
				config_setting_t *binding = config_setting_get_elem(setting, i);
				const char *key = config_setting_name(binding);
				const char *setting = config_setting_get_string(binding);
				if (! setting) {
					fprintf(stderr, "key-binding %s should have string value", key);
					exit(1);
				}

				keybindings_count = process_new_keybinding(configuration->keybindings, keybindings_count, key, setting);
			}
		}
	}
#endif

	configuration->keybindings[keybindings_count] = (keybinding_t) {NULL, NULL};
}

void configuration_free(configuration_t *configuration) {
	if (configuration && configuration->keybindings) {
		for (size_t i = 0; configuration->keybindings[i].key; i++) {
			free(configuration->keybindings[i].key);
		}
		free(configuration->keybindings);
	}
}

#undef KEY_CTRL
