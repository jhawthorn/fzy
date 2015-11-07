VERSION=0.2

CPPFLAGS=-DVERSION=\"${VERSION}\" -D_GNU_SOURCE
CFLAGS+=-Wall -Wextra -g -std=c99 -O3 -pedantic
PREFIX?=/usr/local
MANDIR?=$(PREFIX)/share/man
BINDIR?=$(PREFIX)/bin
DEBUGGER?=

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=${INSTALL} -m 644

all: fzy fzytest

fzytest: fzytest.o match.o choices.o
	$(CC) $(CFLAGS) $(CCFLAGS) -o $@ $^

test: fzytest
	-$(DEBUGGER) ./fzytest

fzy: fzy.o match.o tty.o choices.o
	$(CC) $(CFLAGS) $(CCFLAGS) -o $@ $^

%.o: %.c config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

config.h:
	cp config.def.h config.h

install: fzy
	mkdir -p $(DESTDIR)$(BINDIR)
	cp fzy $(DESTDIR)$(BINDIR)/
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp fzy.1 $(DESTDIR)$(MANDIR)/man1/

fmt:
	clang-format -i *.c *.h

clean:
	$(RM) fzy fzytest *.o

.PHONY: test all clean install fmt
