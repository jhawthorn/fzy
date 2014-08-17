#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "fzy.h"
#include "tty.h"

#define INITIAL_CAPACITY 1
int choices_capacity = 0;
int choices_n = 0;
const char **choices = NULL;
double *choices_score = NULL;
size_t *choices_sorted = NULL;
size_t current_selection = 0;

void resize_choices(int new_capacity){
	choices = realloc(choices, new_capacity * sizeof(const char *));
	choices_score = realloc(choices_score, new_capacity * sizeof(double));
	choices_sorted = realloc(choices_sorted, new_capacity * sizeof(size_t));

	int i = choices_capacity;
	for(; i < new_capacity; i++){
		choices[i] = NULL;
	}
	choices_capacity = new_capacity;
}

void add_choice(const char *choice){
	if(choices_n == choices_capacity){
		resize_choices(choices_capacity * 2);
	}
	choices[choices_n++] = choice;
}

void read_choices(){
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	while ((read = getline(&line, &len, stdin)) != -1) {
		char *nl;
		if((nl = strchr(line, '\n')))
			*nl = '\0';

		add_choice(line);

		line = NULL;
	}
	free(line);
}

size_t choices_available = 0;

static int cmpchoice(const void *p1, const void *p2) {
	size_t idx1 = *(size_t *)p1;
	size_t idx2 = *(size_t *)p2;

	double score1 = choices_score[idx1];
	double score2 = choices_score[idx2];

	if(score1 == score2)
		return 0;
	else if(score1 < score2)
		return 1;
	else
		return -1;
}

void run_search(char *needle){
	current_selection = 0;
	choices_available = 0;
	int i;
	for(i = 0; i < choices_n; i++){
		if(has_match(needle, choices[i])){
			choices_score[i] = match(needle, choices[i]);
			choices_sorted[choices_available++] = i;
		}
	}

	qsort(choices_sorted, choices_available, sizeof(size_t), cmpchoice);
}

#define NUMLINES 10

#define SEARCH_SIZE_MAX 4096
int search_size;
char search[SEARCH_SIZE_MAX + 1] = {0};

void clear(tty_t *tty){
	tty_setcol(tty, 0);
	int line = 0;
	while(line++ < NUMLINES + 1){
		tty_newline(tty);
	}
	fprintf(tty->fout, "%c%c%iA", 0x1b, '[', line-1);
	tty_setcol(tty, 0);
}

#define TTY_COLOR_HIGHLIGHT TTY_COLOR_YELLOW

void draw_match(tty_t *tty, const char *choice, int selected){
	int n = strlen(search);
	size_t positions[n + 1];
	for(int i = 0; i < n + 1; i++)
		positions[i] = -1;

	match_positions(search, choice, &positions[0]);

	if(selected)
		tty_setinvert(tty);

	for(size_t i = 0, p = 0; choice[i] != '\0'; i++){
		if(positions[p] == i){
			tty_setfg(tty, TTY_COLOR_HIGHLIGHT);
			p++;
		}else{
			tty_setfg(tty, TTY_COLOR_NORMAL);
		}
		tty_printf(tty, "%c", choice[i]);
	}
	tty_printf(tty, "\n");
	tty_setnormal(tty);
}

void draw(tty_t *tty){
	int start = 0;
	if(current_selection >= NUMLINES){
		start = current_selection - NUMLINES + 1;
	}
	int line = 0;
	const char *prompt = "> ";
	clear(tty);
	tty_printf(tty, "%s%s\n", prompt, search);
	for(size_t i = start; line < NUMLINES && i < choices_available; i++){
		draw_match(tty, choices[choices_sorted[i]], i == current_selection);
		line++;
	}
	fprintf(tty->fout, "%c%c%iA", 0x1b, '[', line + 1);
	tty_setcol(tty, strlen(prompt) + strlen(search) + 1);
	fflush(tty->fout);
}

void emit(tty_t *tty){
	/* ttyout should be flushed before outputting on stdout */
	fclose(tty->fout);

	if(choices_available){
		/* output the selected result */
		printf("%s\n", choices[choices_sorted[current_selection]]);
	}else{
		/* No match, output the query instead */
		printf("%s\n", search);
	}

	exit(EXIT_SUCCESS);
}

void action_prev(){
	current_selection = (current_selection + choices_available - 1) % choices_available;
}

void action_next(){
	current_selection = (current_selection + 1) % choices_available;
}

void run(tty_t *tty){
	run_search(search);
	char ch;
	do {
		draw(tty);
		ch = tty_getchar(tty);
		if(isprint(ch)){
			if(search_size < SEARCH_SIZE_MAX){
				search[search_size++] = ch;
				search[search_size] = '\0';
				run_search(search);
			}
		}else if(ch == 127 || ch == 8){ /* DEL || backspace */
			if(search_size)
				search[--search_size] = '\0';
			run_search(search);
		}else if(ch == 21){ /* C-U */
			search_size = 0;
			search[0] = '\0';
			run_search(search);
		}else if(ch == 23){ /* C-W */
			if(search_size)
				search[--search_size] = '\0';
			while(search_size && !isspace(search[--search_size]))
				search[search_size] = '\0';
			run_search(search);
		}else if(ch == 14){ /* C-N */
			action_next();
		}else if(ch == 16){ /* C-P */
			action_prev();
		}else if(ch == 10){ /* Enter */
			clear(tty);
			emit(tty);
		}else if(ch == 27){ /* ESC */
			ch = tty_getchar(tty);
			if(ch == '['){
				ch = tty_getchar(tty);
				if(ch == 'A'){ /* UP ARROW */
					action_prev();
				}else if(ch == 'B'){ /* DOWN ARROW */
					action_next();
				}
			}
		}
	}while(1);
}

void usage(const char *argv0){
	fprintf(stderr, "USAGE: %s\n", argv0);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]){
	if(argc == 2 && !strcmp(argv[1], "-v")){
		printf("%s " VERSION  " (c) 2014 John Hawthorn\n", argv[0]);
		exit(EXIT_SUCCESS);
	}else if(argc != 1){
		usage(argv[0]);
	}
	tty_t tty;
	tty_init(&tty);

	resize_choices(INITIAL_CAPACITY);
	read_choices();

	clear(&tty);
	run(&tty);

	return 0;
}
