LIBS=
CFLAGS+=-Wall -Wextra -g
TARGET=fzy
OBJECTS=fzy.o

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(TARGET) *.o
