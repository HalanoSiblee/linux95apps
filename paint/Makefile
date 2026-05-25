CXX      := g++
CXXFLAGS := -std=c++17 -O3 -march=native -flto -fomit-frame-pointer -DNDEBUG $(shell fltk-config --cxxflags) -s
LDFLAGS  := -O3 -march=native -flto $(shell fltk-config --ldflags --use-images)
TARGET   := paint

.PHONY: all clean run

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) main.cpp -o $(TARGET) $(CXXFLAGS) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
