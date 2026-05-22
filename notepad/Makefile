# Makefile
CXX      = g++
CXXFLAGS = $(shell fltk-config --cxxflags) -O3 -march=native
LDFLAGS  = $(shell fltk-config --ldflags)
TARGET   = notepad

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) $(CXXFLAGS) main.cpp $(LDFLAGS) -o $(TARGET)
	strip -s $(TARGET)

clean:
	rm -f $(TARGET)
