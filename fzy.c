#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

double match(const char *needle, const char *haystack){
	while(*needle){
		if(!*haystack)
			return 0.0;
		while(tolower(*needle) == tolower(*haystack++))
			needle++;
	}
	return 1.0;
}

#define INITIAL_CAPACITY 1
int choices_capacity = 0;
int choices_n = 0;
const char **choices = NULL;

void resize_choices(int new_capacity){
	choices = realloc(choices, new_capacity * sizeof(const char *));
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

void run_search(char *needle){
	int i;
	for(i = 0; i < choices_n; i++){
		double rank = match(needle, choices[i]);
		if(rank > 0){
			printf("%s\n", choices[i]);
		}
	}
}

int ttyin;
FILE *ttyout;
struct termios original_termios;

void reset_tty(){
	tcsetattr(ttyin, TCSANOW, &original_termios);
}

void init_tty(){
	ttyin = open("/dev/tty", O_RDONLY);
	ttyout = fopen("/dev/tty", "w");

	tcgetattr(ttyin, &original_termios);

	struct termios new_termios = original_termios;

	new_termios.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(ttyin, TCSANOW, &new_termios);
}

char ttygetchar(){
	char ch;
	int size = read(ttyin, &ch, 1);
	if(size < 0){
		perror("error reading from tty");
		exit(EXIT_FAILURE);
	}else if(size == 0){
		/* EOF */
		exit(EXIT_FAILURE);
	}else{
		return ch;
	}
}

int search_size;
char search[4096] = {0};

void clear(){
	fprintf(ttyout, "%c%c0G", 0x1b, '[');
	int line = 0;
	while(line++ < 10 + 1){
		fprintf(ttyout, "%c%cK\n", 0x1b, '[');
	}
	fprintf(ttyout, "%c%c%iA", 0x1b, '[', line-1);
	fprintf(ttyout, "%c%c0G", 0x1b, '[');
}

void draw(){
	int line = 0;
	int i;
	const char *prompt = "> ";
	clear();
	fprintf(ttyout, "%s%s\n", prompt, search);
	for(i = 0; line < 10 && i < choices_n; i++){
		double rank = match(search, choices[i]);
		if(rank > 0){
			fprintf(ttyout, "%s\n", choices[i]);
			line++;
		}
	}
	fprintf(ttyout, "%c%c%iA", 0x1b, '[', line + 1);
	fprintf(ttyout, "%c%c%iG", 0x1b, '[', strlen(prompt) + strlen(search) + 1);
	fflush(ttyout);
}

void run(){
	draw();
	char ch;
	do {
		ch = ttygetchar();
		if(isprint(ch)){
			/* FIXME: overflow */
			search[search_size++] = ch;
			search[search_size] = '\0';
			draw();
		}else if(ch == 127){ /* DEL */
			if(search_size)
				search[--search_size] = '\0';
			draw();
		}else if(ch == 23){ /* C-W */
			if(search_size)
				search[--search_size] = '\0';
			while(search_size && !isspace(search[--search_size]))
				search[search_size] = '\0';
			draw();
		}else if(ch == 10){ /* Enter */
			clear();
			exit(0);
		}else{
			printf("'%c' (%i)\n", ch, ch);
		}
	}while(ch != 'q');
}

void usage(const char *argv0){
	fprintf(stderr, "USAGE: %s\n", argv0);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]){
	if(argc != 1){
		usage(argv[0]);
	}
	atexit(reset_tty);
	init_tty(reset_tty);

	resize_choices(INITIAL_CAPACITY);
	read_choices();

	clear();
	run();

	return 0;
}
