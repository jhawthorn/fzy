#ifndef TTY_H
#define TTY_H TTY_H

#include <termios.h>

typedef struct{
	int fdin;
	FILE *fout;
	struct termios original_termios;
	int fgcolor;
} tty_t;

void tty_reset(tty_t *tty);
void tty_init(tty_t *tty);
char tty_getchar(tty_t *tty);

void tty_setfg(tty_t *tty, int fg);
void tty_setinvert(tty_t *tty);
void tty_setnormal(tty_t *tty);


#endif
