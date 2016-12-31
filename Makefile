VERSION=0.7

CPPFLAGS=-DVERSION=\"${VERSION}\" -D_GNU_SOURCE
CFLAGS+=-Wall -Wextra -g -std=c99 -O3 -pedantic
PREFIX?=/usr/local
MANDIR?=$(PREFIX)/share/man
BINDIR?=$(PREFIX)/bin
DEBUGGER?=

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=${INSTALL} -m 644

LIBS=-lpthread
OBJECTS=src/fzy.o src/match.o src/tty.o src/choices.o src/options.o src/tty_interface.o
TESTOBJECTS=test/fzytest.c src/match.o src/choices.o

all: fzy

test/fzytest: $(TESTOBJECTS)
	$(CC) $(CFLAGS) $(CCFLAGS) -Isrc -o $@ $(TESTOBJECTS) $(LIBS)

acceptance: fzy
	cd test/acceptance && bundle && bundle exec ruby acceptance_test.rb

test: check
check: test/fzytest
	$(DEBUGGER) ./test/fzytest

fzy: $(OBJECTS)
	$(CC) $(CFLAGS) $(CCFLAGS) -o $@ $(OBJECTS) $(LIBS)

%.o: %.c config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

config.h:
	cp src/config.def.h config.h

install: fzy
	mkdir -p $(DESTDIR)$(BINDIR)
	cp fzy $(DESTDIR)$(BINDIR)/
	chmod 755 ${DESTDIR}${BINDIR}/fzy
	mkdir -p $(DESTDIR)$(MANDIR)/man1
	cp fzy.1 $(DESTDIR)$(MANDIR)/man1/
	chmod 644 ${DESTDIR}${MANDIR}/man1/fzy.1

fmt:
	clang-format -i src/*.c src/*.h

clean:
	rm -f fzy test/fzytest src/*.o

veryclean: clean
	rm -f config.h

.PHONY: test check all clean veryclean install fmt acceptance
