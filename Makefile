VERSION=1.0

CPPFLAGS=-DVERSION=\"${VERSION}\" -D_GNU_SOURCE
CFLAGS+=-Wall -Wextra -g -std=c99 -O3 -pedantic -Ideps
PREFIX?=/usr/local
MANDIR?=$(PREFIX)/share/man
BINDIR?=$(PREFIX)/bin
DEBUGGER?=

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=${INSTALL} -m 644

LIBS=-lpthread
OBJECTS=src/fzy.o src/match.o src/tty.o src/choices.o src/options.o src/tty_interface.o
THEFTDEPS = deps/theft/theft.o deps/theft/theft_bloom.o deps/theft/theft_mt.o deps/theft/theft_hash.o
TESTOBJECTS=test/fzytest.c test/test_properties.c test/test_choices.c test/test_match.c src/match.o src/choices.o src/options.o $(THEFTDEPS)

all: fzy

test/fzytest: $(TESTOBJECTS)
	$(CC) $(CFLAGS) $(CCFLAGS) -Isrc -o $@ $(TESTOBJECTS) $(LIBS)

acceptance: fzy
	cd test/acceptance && bundle --quiet && bundle exec ruby acceptance_test.rb

test: check
check: test/fzytest
	$(DEBUGGER) ./test/fzytest

fzy: $(OBJECTS)
	$(CC) $(CFLAGS) $(CCFLAGS) -o $@ $(OBJECTS) $(LIBS)

%.o: %.c config.h %.d
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.d: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM -MT"$@ $(@:.d=.o)" -MP -MF $@ $<

config.h: src/config.def.h
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
	rm -f fzy test/fzytest src/*.o deps/*/*.o
	rm -f $(AUTODEPS)

veryclean: clean
	rm -f config.h

# create a list of auto dependencies
AUTODEPS:= $(patsubst %.o,%.d, $(OBJECTS)) $(patsubst %.o,%.d, $(THEFTDEPS))

# include by auto dependencies
-include $(AUTODEPS)

.PHONY: test check all clean veryclean install fmt acceptance
