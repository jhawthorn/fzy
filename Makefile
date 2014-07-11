LIBS=
CFLAGS+=-Wall -g
TARGET=hawthfuzz
OBJECTS=hawthfuzz.o

$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(TARGET) *.o
