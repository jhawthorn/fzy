#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

#include "tty.h"

void tty_reset(tty_t *tty) {
	tcsetattr(tty->fdin, TCSANOW, &tty->original_termios);
}

void tty_close(tty_t *tty) {
	tty_reset(tty);
	fclose(tty->fout);
	close(tty->fdin);
}

void tty_init(tty_t *tty, const char *tty_filename) {
	tty->fdin = open(tty_filename, O_RDONLY);
	tty->ready = false;
	if (tty->fdin < 0) {
		perror("Failed to open tty");
		exit(EXIT_FAILURE);
	}

	tty->fout = fopen(tty_filename, "w");
	if (!tty->fout) {
		perror("Failed to open tty");
		exit(EXIT_FAILURE);
	}

	if (setvbuf(tty->fout, NULL, _IOFBF, 4096)) {
		perror("setvbuf");
		exit(EXIT_FAILURE);
	}

	if (tcgetattr(tty->fdin, &tty->original_termios)) {
		perror("tcgetattr");
		exit(EXIT_FAILURE);
	}

	tty_getwinsz(tty);

	tty_setnormal(tty);
}

void tty_init_termios(tty_t *tty) {
	struct termios new_termios = tty->original_termios;

	/*
	 * Disable all of
	 * ICANON  Canonical input (erase and kill processing).
	 * ECHO    Echo.
	 * ISIG    Signals from control characters
	 * ICRNL   Conversion of CR characters into NL
	 */
	new_termios.c_iflag &= ~(ICRNL);
	new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);

	if (tcsetattr(tty->fdin, TCSANOW, &new_termios))
		perror("tcsetattr");

	tty->ready = true;
}

void tty_getwinsz(tty_t *tty) {
	struct winsize ws;
	if (ioctl(fileno(tty->fout), TIOCGWINSZ, &ws) == -1) {
		tty->maxwidth = 80;
		tty->maxheight = 25;
	} else {
		tty->maxwidth = ws.ws_col;
		tty->maxheight = ws.ws_row;
	}
}

char tty_getchar(tty_t *tty) {
	char ch;
	int size = read(tty->fdin, &ch, 1);
	if (size < 0) {
		perror("error reading from tty");
		exit(EXIT_FAILURE);
	} else if (size == 0) {
		/* EOF */
		exit(EXIT_FAILURE);
	} else {
		return ch;
	}
}

int tty_input_ready(tty_t *tty) {
	fd_set readfs;
	struct timeval tv = {0, 0};
	FD_SET(tty->fdin, &readfs);
	select(tty->fdin + 1, &readfs, NULL, NULL, &tv);
	return FD_ISSET(tty->fdin, &readfs);
}

static void tty_sgr(tty_t *tty, int code) {
	tty_printf(tty, "%c%c%im", 0x1b, '[', code);
}

void tty_setfg(tty_t *tty, int fg) {
	if (tty->fgcolor != fg) {
		tty_sgr(tty, 30 + fg);
		tty->fgcolor = fg;
	}
}

void tty_setinvert(tty_t *tty) {
	tty_sgr(tty, 7);
}

void tty_setnormal(tty_t *tty) {
	tty_sgr(tty, 0);
	tty->fgcolor = 9;
}

void tty_newline(tty_t *tty) {
	tty_printf(tty, "%c%cK\n", 0x1b, '[');
}

void tty_clearline(tty_t *tty) {
	tty_printf(tty, "%c%cK", 0x1b, '[');
}

void tty_setcol(tty_t *tty, int col) {
	tty_printf(tty, "%c%c%iG", 0x1b, '[', col + 1);
}

void tty_moveup(tty_t *tty, int i) {
	tty_printf(tty, "%c%c%iA", 0x1b, '[', i);
}

void tty_printf(tty_t *tty, const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vfprintf(tty->fout, fmt, args);
	va_end(args);
}

void tty_flush(tty_t *tty) {
	fflush(tty->fout);
}

size_t tty_getwidth(tty_t *tty) {
	return tty->maxwidth;
}

size_t tty_getheight(tty_t *tty) {
	return tty->maxheight;
}
