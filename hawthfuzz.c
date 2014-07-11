#include <stdio.h>
#include <string.h>
#include <stdlib.h>

double match(const char *needle, const char *haystack){
	while(*needle){
		while(*needle == *haystack++)
			needle++;
		if(!*haystack)
			return 1;
	}
	return 0;
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
		printf("'%s' =~ '%s'\t%f\n", needle, choices[i], rank);
	}
}

int main(int argc, char *argv[]){
	resize_choices(INITIAL_CAPACITY);
	read_choices();
	run_search(argv[1]);
	return 0;
}
