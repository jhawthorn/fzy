VERSION=0.2pre

CPPFLAGS=-DVERSION=\"${VERSION}\" -D_GNU_SOURCE
CFLAGS+=-Wall -Wextra -g -std=c99 -O3 -pedantic
PREFIX?=/usr/local
MANDIR?=$(PREFIX)/share/man
BINDIR?=$(PREFIX)/bin

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=${INSTALL} -m 644

all: fzy fzytest

fzytest: fzytest.o match.o choices.o
	$(CC) $(CFLAGS) $(CCFLAGS) -o $@ $^

test: fzytest
	-./fzytest

fzy: fzy.o match.o tty.o choices.o
	$(CC) $(CFLAGS) $(CCFLAGS) -o $@ $^

%.o: %.c config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

install: fzy
	$(INSTALL_PROGRAM) fzy $(DESTDIR)$(BINDIR)/fzy
	$(INSTALL_PROGRAM) -d $(DESTDIR)$(MANDIR)/man1
	$(INSTALL_PROGRAM) fzy.1 $(DESTDIR)$(MANDIR)/man1/fzy.1

clean:
	$(RM) fzy fzytest *.o

.PHONY: test all clean install
