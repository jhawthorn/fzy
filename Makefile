CFLAGS+=-Wall -Wextra -g -std=c99

all: fzy fzytest

fzytest: fzytest.o match.o
	$(CC) $(CCFLAGS) -o $@ $^

test: fzytest
	-./fzytest

fzy: fzy.o match.o
	$(CC) $(CCFLAGS) -o $@ $^

%.o: %.c fzy.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) fzy fzytest *.o

.PHONY: test all clean
