CC      = g++
CFLAGS  = -O3 -march=native -flto -fomit-frame-pointer $(shell fltk-config --cxxflags)
LDFLAGS = $(shell fltk-config --ldflags) -Wl,-O1 -Wl,--as-needed -Wl,--strip-all -flto

TARGET  = run
SRC     = run.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) $(LDFLAGS)
	@strip -s $(TARGET)
	@echo "Build complete: $(TARGET) (Native Hybrid Clean Build)"

clean:
	rm -f $(TARGET)

.PHONY: all clean
