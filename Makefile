VERSION=0.1beta

CPPFLAGS=-DVERSION=\"${VERSION}\"
CFLAGS+=-Wall -Wextra -g -std=c99 -O2
PREFIX?=/usr/local

INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=${INSTALL} -m 644

all: fzy fzytest

fzytest: fzytest.o match.o
	$(CC) $(CCFLAGS) -o $@ $^

test: fzytest
	-./fzytest

fzy: fzy.o match.o tty.o
	$(CC) $(CCFLAGS) -o $@ $^

%.o: %.c fzy.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

install: fzy
	$(INSTALL_PROGRAM) fzy $(DESTDIR)$(PREFIX)/bin/fzy

clean:
	$(RM) fzy fzytest *.o

.PHONY: test all clean install
