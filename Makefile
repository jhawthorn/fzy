LIBS=
CFLAGS+=-Wall -Wextra -g
TARGET=hawthfuzz
OBJECTS=hawthfuzz.o

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(TARGET) *.o
