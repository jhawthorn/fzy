LIBS=
CFLAGS+=-Wall -Wextra -g
TARGET=fzy
OBJECTS=fzy.o

$(TARGET): $(OBJECTS)
	$(CC) $(CCFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(TARGET) *.o
