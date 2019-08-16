#ifndef TTY_H
#define TTY_H TTY_H

#include <termios.h>

typedef struct {
	int fdin;
	FILE *fout;
	struct termios original_termios;
	int fgcolor;
	size_t maxwidth;
	size_t maxheight;
} tty_t;

void tty_reset(tty_t *tty);
void tty_close(tty_t *tty);
void tty_init(tty_t *tty, const char *tty_filename);
void tty_getwinsz(tty_t *tty);
char tty_getchar(tty_t *tty);
int tty_input_ready(tty_t *tty, long int timeout, int return_on_signal);

void tty_setfg(tty_t *tty, int fg);
void tty_setinvert(tty_t *tty);
void tty_setunderline(tty_t *tty);
void tty_setnormal(tty_t *tty);
void tty_setnowrap(tty_t *tty);
void tty_setwrap(tty_t *tty);

#define TTY_COLOR_BLACK 0
#define TTY_COLOR_RED 1
#define TTY_COLOR_GREEN 2
#define TTY_COLOR_YELLOW 3
#define TTY_COLOR_BLUE 4
#define TTY_COLOR_MAGENTA 5
#define TTY_COLOR_CYAN 6
#define TTY_COLOR_WHITE 7
#define TTY_COLOR_NORMAL 9

/* tty_newline
 * Move cursor to the beginning of the next line, clearing to the end of the
 * current line
 */
void tty_newline(tty_t *tty);

/* tty_clearline
 * Clear to the end of the current line without advancing the cursor.
 */
void tty_clearline(tty_t *tty);

void tty_moveup(tty_t *tty, int i);
void tty_setcol(tty_t *tty, int col);

void tty_printf(tty_t *tty, const char *fmt, ...);
void tty_putc(tty_t *tty, char c);
void tty_flush(tty_t *tty);

size_t tty_getwidth(tty_t *tty);
size_t tty_getheight(tty_t *tty);

#endif
