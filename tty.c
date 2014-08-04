#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "tty.h"

void tty_reset(tty_t *tty){
	tcsetattr(tty->fdin, TCSANOW, &tty->original_termios);
}

void tty_init(tty_t *tty){
	tty->fdin = open("/dev/tty", O_RDONLY);
	tty->fout = fopen("/dev/tty", "w");

	tcgetattr(tty->fdin, &tty->original_termios);

	struct termios new_termios = tty->original_termios;

	new_termios.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(tty->fdin, TCSANOW, &new_termios);
}

char tty_getchar(tty_t *tty){
	char ch;
	int size = read(tty->fdin, &ch, 1);
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

